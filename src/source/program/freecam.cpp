#include "program/freecam.hpp"
#include "program/controls.hpp"

#include <nn/os/os_tick.hpp>

namespace freecam {

    namespace {

        constexpr fm::Vec3 WorldUp { 0.0f, 1.0f, 0.0f };
        constexpr float MaxPitch = 89.0f * fm::DegToRad;
        constexpr float MaxDeltaTime = 0.1f;   /* clamp after loads / pauses */
        /* If no camera has been built for this long (loading, pause menu, ...), switch off so the game gets its controls back. */
        constexpr float WatchdogSeconds = 0.5f;

        struct State {
            bool enabled = false;
            bool flying = true;

            /* Last camera the game produced while we were off. */
            bool haveGameCamera = false;
            fm::Vec3 gamePos { 0.0f, 0.0f, 0.0f };
            fm::Vec3 gameAt  { 0.0f, 0.0f, 1.0f };

            /* Freecam pose. */
            fm::Vec3 pos { 0.0f, 0.0f, 0.0f };
            float yaw = 0.0f;
            float pitch = 0.0f;
            float roll = 0.0f;
            float lookDistance = 100.0f;
            float speedScale = 1.0f;

            /* Smoothed rates. */
            fm::Vec3 velocity { 0.0f, 0.0f, 0.0f };
            float yawRate = 0.0f;
            float pitchRate = 0.0f;
            float rollRate = 0.0f;

            /* Timing / input edges. */
            s64 lastTick = 0;
            s64 tickFrequency = 0;
            u64 prevButtons = 0;
        };

        s64 TickFrequency(State& s) {
            if (s.tickFrequency <= 0) {
                s.tickFrequency = nn::os::GetSystemTickFrequency();
                if (s.tickFrequency <= 0)
                    s.tickFrequency = 19200000; /* Switch CNTFRQ */
            }
            return s.tickFrequency;
        }

        constinit State g_State {};
        constinit PadReader g_PadReader = nullptr;

        float StepTime(State& s) {
            const s64 now = nn::os::GetSystemTick().GetInt64Value();
            const s64 frequency = TickFrequency(s);

            float dt = 0.0f;
            if (s.lastTick != 0 && now > s.lastTick)
                dt = static_cast<float>(now - s.lastTick) / static_cast<float>(frequency);
            s.lastTick = now;

            return fm::Clamp(dt, 0.0f, MaxDeltaTime);
        }

        fm::Vec3 ForwardFromAngles(float yaw, float pitch) {
            const float cp = fm::Cos(pitch);
            return { fm::Sin(yaw) * cp, fm::Sin(pitch), fm::Cos(yaw) * cp };
        }

        /* Screen-right for a right-handed, Y-up look-at camera. */
        fm::Vec3 RightFromForward(const fm::Vec3& forward, float yaw) {
            const fm::Vec3 fallback { -fm::Cos(yaw), 0.0f, fm::Sin(yaw) };
            return forward.Cross(WorldUp).Normalized(fallback);
        }

        /* Gentle response curve: precise near the centre, full speed at the edge. */
        float ShapeLook(float v) {
            return v * (0.4f + 0.6f * fm::Abs(v));
        }

        bool Held(const PadInput& pad, u64 mask) { return (pad.buttons & mask) != 0; }

        void ResetRates(State& s) {
            s.velocity = { 0.0f, 0.0f, 0.0f };
            s.yawRate = s.pitchRate = s.rollRate = 0.0f;
        }

        void Enable(State& s) {
            const fm::Vec3 delta = s.gameAt - s.gamePos;
            const fm::Vec3 dir = delta.Normalized({ 0.0f, 0.0f, 1.0f });

            s.pos = s.gamePos;
            s.yaw = fm::Atan2(dir.x, dir.z);
            s.pitch = fm::Clamp(fm::Asin(dir.y), -MaxPitch, MaxPitch);
            s.roll = 0.0f;
            s.lookDistance = fm::Clamp(delta.Length(), 1.0f, 5000.0f);
            s.flying = true;
            ResetRates(s);
            s.enabled = true;
        }

        void Fly(State& s, const PadInput& pad, u64 pressed, float dt) {
            /* Speed adjustments. */
            if (pressed & cfg::SpeedUp)
                s.speedScale = fm::Min(s.speedScale * cfg::SpeedStep, 64.0f);
            if (pressed & cfg::SpeedDown)
                s.speedScale = fm::Max(s.speedScale / cfg::SpeedStep, 1.0f / 64.0f);
            if (pressed & cfg::ResetRoll) {
                s.roll = 0.0f;
                s.rollRate = 0.0f;
                s.speedScale = 1.0f;
            }

            const float alpha = fm::SmoothAlpha(dt, cfg::Smoothing);

            /* Rotation. */
            const float look = cfg::LookSpeedDeg * fm::DegToRad;
            const float targetYawRate   = -ShapeLook(pad.rx) * look * (cfg::InvertLookX ? -1.0f : 1.0f);
            const float targetPitchRate =  ShapeLook(pad.ry) * look * (cfg::InvertLookY ? -1.0f : 1.0f);
            const float rollDir = (Held(pad, cfg::RollRight) ? 1.0f : 0.0f) - (Held(pad, cfg::RollLeft) ? 1.0f : 0.0f);
            const float targetRollRate = rollDir * cfg::RollSpeedDeg * fm::DegToRad;

            s.yawRate   = fm::Lerp(s.yawRate, targetYawRate, alpha);
            s.pitchRate = fm::Lerp(s.pitchRate, targetPitchRate, alpha);
            s.rollRate  = fm::Lerp(s.rollRate, targetRollRate, alpha);

            s.yaw   = fm::WrapPi(s.yaw + s.yawRate * dt);
            s.pitch = fm::Clamp(s.pitch + s.pitchRate * dt, -MaxPitch, MaxPitch);
            s.roll  = fm::WrapPi(s.roll + s.rollRate * dt);

            /* Translation (flies along the view direction, like a drone). */
            float speed = cfg::MoveSpeed * s.speedScale;
            if (Held(pad, cfg::Fast))
                speed *= cfg::FastMultiplier;
            if (Held(pad, cfg::Slow))
                speed *= cfg::SlowMultiplier;

            const fm::Vec3 forward = ForwardFromAngles(s.yaw, s.pitch);
            const fm::Vec3 right = RightFromForward(forward, s.yaw);
            const float vertical = (Held(pad, cfg::Rise) ? 1.0f : 0.0f) - (Held(pad, cfg::Fall) ? 1.0f : 0.0f);

            const fm::Vec3 targetVelocity = forward * (pad.ly * speed) + right * (pad.lx * speed) + WorldUp * (vertical * speed);

            s.velocity.x = fm::Lerp(s.velocity.x, targetVelocity.x, alpha);
            s.velocity.y = fm::Lerp(s.velocity.y, targetVelocity.y, alpha);
            s.velocity.z = fm::Lerp(s.velocity.z, targetVelocity.z, alpha);

            s.pos += s.velocity * dt;

            /* Never write garbage into the game's camera. */
            if (!s.pos.IsFinite()) {
                s.pos = s.gamePos;
                ResetRates(s);
            }
        }

        void WriteCamera(const State& s, LookAtCamera* camera) {
            const fm::Vec3 forward = ForwardFromAngles(s.yaw, s.pitch);
            const fm::Vec3 right = RightFromForward(forward, s.yaw);
            const fm::Vec3 levelUp = right.Cross(forward);
            const fm::Vec3 up = levelUp * fm::Cos(s.roll) + right * fm::Sin(s.roll);

            camera->pos = s.pos;
            camera->at = s.pos + forward * s.lookDistance;
            camera->up = up.Normalized(WorldUp);
        }
    }

    void SetPadReader(PadReader reader) {
        g_PadReader = reader;
    }

    bool IsEnabled() {
        State& s = g_State;
        if (!s.enabled)
            return false;

        /* Watchdog: the camera hooks have gone quiet, so hand everything back to the game. */
        const s64 now = nn::os::GetSystemTick().GetInt64Value();
        const s64 limit = static_cast<s64>(WatchdogSeconds * static_cast<float>(TickFrequency(s)));
        if (s.lastTick != 0 && now - s.lastTick > limit) {
            s.enabled = false;
            ResetRates(s);
            return false;
        }
        return true;
    }

    bool ShouldBlockGameInput() {
        return cfg::BlockGameInputWhileFlying && IsEnabled() && g_State.flying;
    }

    void OnCameraBuilt(LookAtCamera* camera) {
        if (camera == nullptr || g_PadReader == nullptr)
            return;

        State& s = g_State;

        /* Several cameras can be built per frame; time-based steps keep motion correct regardless. */
        const float dt = StepTime(s);
        const PadInput pad = g_PadReader();
        const u64 pressed = pad.buttons & ~s.prevButtons;
        s.prevButtons = pad.buttons;

        const bool toggle = Held(pad, cfg::ToggleHold) && (pressed & cfg::TogglePress) != 0;

        if (!s.enabled) {
            if (camera->pos.IsFinite() && camera->at.IsFinite()) {
                s.gamePos = camera->pos;
                s.gameAt = camera->at;
                s.haveGameCamera = true;
            }

            if (!toggle || !s.haveGameCamera)
                return; /* Leave the game's camera untouched. */

            Enable(s);
        } else if (toggle) {
            s.enabled = false;
            ResetRates(s);
            return; /* Hand the camera straight back to the game. */
        } else {
            if (pressed & cfg::ParkToggle) {
                s.flying = !s.flying;
                ResetRates(s);
            }

            if (s.flying)
                Fly(s, pad, pressed, dt);
        }

        WriteCamera(s, camera);
    }
}
