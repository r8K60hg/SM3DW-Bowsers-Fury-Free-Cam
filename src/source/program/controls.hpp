#pragma once

/*
 * ===================== FREECAM SETTINGS =====================
 * Edit the values below and rebuild to customise the freecam.
 * Button names follow the Switch layout (nn::hid::Button::*).
 * ============================================================
 */

#include "program/hid.hpp"

namespace freecam::cfg {

    namespace Btn = nn::hid::Button;

    /* --- Toggle: hold the first button, then press the second. --- */
    constexpr u64 ToggleHold  = Btn::L;
    constexpr u64 TogglePress = Btn::StickR;     /* L + R3 */

    /* --- While the freecam is on --- */
    constexpr u64 ParkToggle  = Btn::StickL;     /* L3: park camera & give control back to Mario */
    constexpr u64 Rise        = Btn::R;
    constexpr u64 Fall        = Btn::L;
    constexpr u64 Fast        = Btn::ZR;         /* hold */
    constexpr u64 Slow        = Btn::ZL;         /* hold */
    constexpr u64 SpeedUp     = Btn::Up;         /* D-pad, press */
    constexpr u64 SpeedDown   = Btn::Down;       /* D-pad, press */
    constexpr u64 RollLeft    = Btn::Left;       /* D-pad, hold */
    constexpr u64 RollRight   = Btn::Right;      /* D-pad, hold */
    constexpr u64 ResetRoll   = Btn::Minus;      /* resets roll and speed */

    /* --- Feel --- */
    constexpr float MoveSpeed      = 800.0f;  /* world units per second (one block is ~100 units) */
    constexpr float LookSpeedDeg   = 110.0f;  /* degrees per second at full stick */
    constexpr float RollSpeedDeg   = 45.0f;   /* degrees per second */
    constexpr float FastMultiplier = 4.0f;
    constexpr float SlowMultiplier = 0.2f;
    constexpr float SpeedStep      = 1.5f;    /* D-pad up/down multiplies/divides by this */
    constexpr float Deadzone       = 0.15f;   /* 0..1 */
    constexpr float Smoothing      = 0.10f;   /* seconds; 0 = instant, higher = more cinematic */

    constexpr bool  InvertLookX    = false;
    constexpr bool  InvertLookY    = false;

    /* Stop Mario from reacting to the controller while you fly the camera. */
    constexpr bool  BlockGameInputWhileFlying = true;

    /* Set to true only if you know the camera offsets match your game build. */
    constexpr bool  SkipVersionCheck = false;
}
