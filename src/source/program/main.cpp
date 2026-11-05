/*
 * BF Freecam - free camera for Super Mario 3D World + Bowser's Fury (v1.1.0)
 * ExLaunch module. Licensed under GPL-2.0 (see LICENSE).
 */

#include "lib.hpp"

#include "program/controls.hpp"
#include "program/freecam.hpp"
#include "program/game.hpp"
#include "program/hid.hpp"

#define FREECAM_LOG(...) Logging.Log("[BFFreecam] " __VA_ARGS__)

namespace {

    using freecam::LookAtCamera;
    using nn::hid::NpadIdType;

    /* ------------------------------------------------------------------ */
    /* Camera hooks                                                        */
    /* ------------------------------------------------------------------ */

    /* Every hooked function has the shape `void makeLookAtCamera(const Poser*, sead::LookAtCamera*)`. */
    template<int Id>
    struct LookAtHook : public exl::hook::impl::TrampolineHook<LookAtHook<Id>> {
        using Base = exl::hook::impl::TrampolineHook<LookAtHook<Id>>;

        static void Callback(const void* poser, LookAtCamera* camera) {
            Base::Orig(poser, camera);
            freecam::OnCameraBuilt(camera);
        }
    };

    HOOK_DEFINE_TRAMPOLINE(RsPoseUpdaterSwitch) {
        static void Callback(void* updater) {
            /* Stop Bowser's Fury from switching camera tickets underneath us. */
            if (!freecam::IsEnabled())
                Orig(updater);
        }
    };

    /* ------------------------------------------------------------------ */
    /* Controller hooks                                                    */
    /* ------------------------------------------------------------------ */

    void NeutraliseStates(nn::hid::NpadCommonState* states, int count) {
        for (int i = 0; i < count; i++) {
            states[i].buttons = 0;
            states[i].analogStickL = { 0, 0 };
            states[i].analogStickR = { 0, 0 };
        }
    }

    template<typename State>
    struct NpadStatesHook : public exl::hook::impl::TrampolineHook<NpadStatesHook<State>> {
        using Base = exl::hook::impl::TrampolineHook<NpadStatesHook<State>>;

        static int Callback(State* out, int count, const NpadIdType& id) {
            const int written = Base::Orig(out, count, id);
            if (freecam::ShouldBlockGameInput() && out != nullptr && written > 0)
                NeutraliseStates(out, written < count ? written : count);
            return written;
        }

        /* Reads the real controller, bypassing the block above. */
        static int ReadUnblocked(State* out, int count, const NpadIdType& id) {
            return Base::Orig(out, count, id);
        }

        static void Install() {
            Base::InstallAtFuncPtr(static_cast<int (*)(State*, int, const NpadIdType&)>(&nn::hid::GetNpadStates));
        }
    };

    /* The game reads every controller style through nn::hid::GetNpadStates (it imports nothing else from hid). */
    template<typename State>
    void InstallNpadHooks() {
        NpadStatesHook<State>::Install();
    }

    /* ------------------------------------------------------------------ */
    /* Pad reading                                                         */
    /* ------------------------------------------------------------------ */

    struct RawPad {
        u64 buttons = 0;
        nn::hid::AnalogStickState left { 0, 0 };
        nn::hid::AnalogStickState right { 0, 0 };
    };

    s64 MagnitudeSq(const nn::hid::AnalogStickState& s) {
        return s64(s.x) * s.x + s64(s.y) * s.y;
    }

    template<typename State>
    void MergePad(RawPad& acc, NpadIdType id) {
        State state {};
        const int written = NpadStatesHook<State>::ReadUnblocked(&state, 1, id);
        if (written < 1 || (state.attributes & nn::hid::Attribute::IsConnected) == 0)
            return;

        acc.buttons |= state.buttons;
        if (MagnitudeSq(state.analogStickL) > MagnitudeSq(acc.left))
            acc.left = state.analogStickL;
        if (MagnitudeSq(state.analogStickR) > MagnitudeSq(acc.right))
            acc.right = state.analogStickR;
    }

    void ApplyDeadzone(const nn::hid::AnalogStickState& raw, float* outX, float* outY) {
        const float scale = 1.0f / static_cast<float>(nn::hid::AnalogStickMax);
        const float x = fm::Clamp(static_cast<float>(raw.x) * scale, -1.0f, 1.0f);
        const float y = fm::Clamp(static_cast<float>(raw.y) * scale, -1.0f, 1.0f);
        const float mag = fm::Sqrt(x * x + y * y);

        if (mag <= freecam::cfg::Deadzone || mag <= 0.0f) {
            *outX = 0.0f;
            *outY = 0.0f;
            return;
        }

        const float scaled = fm::Min((mag - freecam::cfg::Deadzone) / (1.0f - freecam::cfg::Deadzone), 1.0f);
        *outX = x / mag * scaled;
        *outY = y / mag * scaled;
    }

    freecam::PadInput ReadPad() {
        RawPad raw {};
        /* Pro Controller / single player 1, handheld, and dual Joy-Con on player 1. */
        MergePad<nn::hid::NpadFullKeyState>(raw, nn::hid::NpadId::No1);
        MergePad<nn::hid::NpadHandheldState>(raw, nn::hid::NpadId::Handheld);
        MergePad<nn::hid::NpadJoyDualState>(raw, nn::hid::NpadId::No1);

        freecam::PadInput pad {};
        pad.buttons = raw.buttons;
        ApplyDeadzone(raw.left, &pad.lx, &pad.ly);
        ApplyDeadzone(raw.right, &pad.rx, &pad.ry);
        return pad;
    }

    /* ------------------------------------------------------------------ */
    /* Version check                                                       */
    /* ------------------------------------------------------------------ */

    /* Same approach Atmosphère's creport uses: the GNU build-id note sits at the end of .rodata. */
    bool FindMainBuildId(u8 (&out)[16]) {
        const auto& rodata = exl::util::GetMainModuleInfo().m_Rodata;
        constexpr size_t NoteTail = 4 + sizeof(out); /* "GNU\0" + id */
        constexpr size_t SearchWindow = 0x2000;

        if (rodata.m_Start == 0 || rodata.m_Size < NoteTail)
            return false;

        const uintptr_t end = rodata.GetEnd();
        const uintptr_t lowest = rodata.m_Size > SearchWindow ? end - SearchWindow : rodata.m_Start;

        for (uintptr_t p = end - NoteTail;; p--) {
            const u8* b = reinterpret_cast<const u8*>(p);
            if (b[0] == 'G' && b[1] == 'N' && b[2] == 'U' && b[3] == '\0') {
                for (size_t i = 0; i < sizeof(out); i++)
                    out[i] = b[4 + i];
                return true;
            }
            if (p == lowest)
                break;
        }
        return false;
    }

    const game::GameVersion* DetectGameVersion() {
        u8 id[16] = {};
        if (!FindMainBuildId(id)) {
            FREECAM_LOG("Could not read the game's build ID.");
            return nullptr;
        }

        FREECAM_LOG("Game build ID: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7],
                    id[8], id[9], id[10], id[11], id[12], id[13], id[14], id[15]);

        for (const game::GameVersion& version : game::SupportedVersions) {
            bool match = true;
            for (size_t i = 0; i < sizeof(id); i++) {
                if (id[i] != version.buildId[i]) {
                    match = false;
                    break;
                }
            }
            if (match)
                return &version;
        }
        return nullptr;
    }

    /* ------------------------------------------------------------------ */
    /* Installation                                                        */
    /* ------------------------------------------------------------------ */

    void InstallCameraHooks(const game::CameraOffsets& off) {
        LookAtHook<0>::InstallAtOffset(off.RsMakeLookAtCamera);
        RsPoseUpdaterSwitch::InstallAtOffset(off.RsPoseUpdaterSwitch);

        LookAtHook<1>::InstallAtOffset(off.PoserAnim);
        LookAtHook<2>::InstallAtOffset(off.PoserEntrance);
        LookAtHook<3>::InstallAtOffset(off.PoserFixedAll);
        LookAtHook<4>::InstallAtOffset(off.PoserFixedPoint);
        LookAtHook<5>::InstallAtOffset(off.PoserKinopioBrigade);
        LookAtHook<6>::InstallAtOffset(off.PoserParallel);
        LookAtHook<7>::InstallAtOffset(off.PoserParallelOnRail);
        LookAtHook<8>::InstallAtOffset(off.PoserProgramable);
        LookAtHook<9>::InstallAtOffset(off.PoserRail);
        LookAtHook<10>::InstallAtOffset(off.PoserRailTower);
        LookAtHook<11>::InstallAtOffset(off.PoserTower);
        LookAtHook<12>::InstallAtOffset(off.PoserZoom);
    }

    void InstallInputHooks() {
        InstallNpadHooks<nn::hid::NpadFullKeyState>();
        InstallNpadHooks<nn::hid::NpadHandheldState>();
        InstallNpadHooks<nn::hid::NpadJoyDualState>();
        InstallNpadHooks<nn::hid::NpadJoyLeftState>();
        InstallNpadHooks<nn::hid::NpadJoyRightState>();
    }
}

extern "C" void exl_main(void* x0, void* x1) {
    EXL_UNUSED(x0, x1);

    exl::hook::Initialize();

    const game::GameVersion* version = DetectGameVersion();
    if (version == nullptr) {
        if (!freecam::cfg::SkipVersionCheck) {
            FREECAM_LOG("Unsupported game version - freecam disabled.");
            return;
        }
        /* Forced: assume the first (newest) table entry. */
        version = &game::SupportedVersions[0];
    }

    /* Input hooks first: the camera hooks read the pad through their trampolines. */
    InstallInputHooks();
    freecam::SetPadReader(ReadPad);
    InstallCameraHooks(version->camera);

    FREECAM_LOG("Loaded for game v%s. Hold L and click R3 to toggle the free camera.", version->name);
}

extern "C" NORETURN void exl_exception_entry() {
    /* Only used for applets/sysmodules. */
    EXL_ABORT("Default exception handler called!");
}
