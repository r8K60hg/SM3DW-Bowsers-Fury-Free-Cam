#pragma once

/*
 * Minimal nn::hid declarations used by the freecam.
 *
 * These are resolved at load time against the game's nnSdk module. Every
 * function declared here exists in the SDK that ships with
 * Super Mario 3D World + Bowser's Fury 1.1.0.
 */

#include <common.hpp>
#include <cstddef>

namespace nn::hid {

    /* nn::hid::NpadIdType is a plain u32 (mangled as "j"). */
    using NpadIdType = unsigned int;

    namespace NpadId {
        constexpr NpadIdType No1      = 0;
        constexpr NpadIdType Handheld = 0x20;
    }

    struct AnalogStickState {
        s32 x;
        s32 y;
    };

    /* Shared layout of every "standard" Npad state (FullKey, Handheld, JoyDual, JoyLeft, JoyRight). */
    struct NpadCommonState {
        s64 samplingNumber;
        u64 buttons;              /* nn::hid::NpadButtonSet */
        AnalogStickState analogStickL;
        AnalogStickState analogStickR;
        u32 attributes;           /* nn::hid::NpadAttributesSet */
        u32 reserved;
    };
    static_assert(sizeof(NpadCommonState) == 0x28);
    static_assert(offsetof(NpadCommonState, buttons) == 0x08);
    static_assert(offsetof(NpadCommonState, analogStickL) == 0x10);
    static_assert(offsetof(NpadCommonState, analogStickR) == 0x18);
    static_assert(offsetof(NpadCommonState, attributes) == 0x20);

    struct NpadFullKeyState  : NpadCommonState {};
    struct NpadHandheldState : NpadCommonState {};
    struct NpadJoyDualState  : NpadCommonState {};
    struct NpadJoyLeftState  : NpadCommonState {};
    struct NpadJoyRightState : NpadCommonState {};

    /* Latest single state. */
    void GetNpadState(NpadFullKeyState* out,  const NpadIdType& id);
    void GetNpadState(NpadHandheldState* out, const NpadIdType& id);
    void GetNpadState(NpadJoyDualState* out,  const NpadIdType& id);
    void GetNpadState(NpadJoyLeftState* out,  const NpadIdType& id);
    void GetNpadState(NpadJoyRightState* out, const NpadIdType& id);

    /* Ring-buffer history, newest first. Returns the number of entries written. */
    int GetNpadStates(NpadFullKeyState* out,  int count, const NpadIdType& id);
    int GetNpadStates(NpadHandheldState* out, int count, const NpadIdType& id);
    int GetNpadStates(NpadJoyDualState* out,  int count, const NpadIdType& id);
    int GetNpadStates(NpadJoyLeftState* out,  int count, const NpadIdType& id);
    int GetNpadStates(NpadJoyRightState* out, int count, const NpadIdType& id);

    /* nn::hid::NpadButton bit indices. */
    namespace Button {
        constexpr u64 Bit(int i) { return u64(1) << i; }

        constexpr u64 A      = Bit(0);
        constexpr u64 B      = Bit(1);
        constexpr u64 X      = Bit(2);
        constexpr u64 Y      = Bit(3);
        constexpr u64 StickL = Bit(4);   /* L3: click left stick  */
        constexpr u64 StickR = Bit(5);   /* R3: click right stick */
        constexpr u64 L      = Bit(6);
        constexpr u64 R      = Bit(7);
        constexpr u64 ZL     = Bit(8);
        constexpr u64 ZR     = Bit(9);
        constexpr u64 Plus   = Bit(10);
        constexpr u64 Minus  = Bit(11);
        constexpr u64 Left   = Bit(12);  /* D-pad */
        constexpr u64 Up     = Bit(13);
        constexpr u64 Right  = Bit(14);
        constexpr u64 Down   = Bit(15);
    }

    namespace Attribute {
        constexpr u32 IsConnected = 1u << 0;
    }

    constexpr s32 AnalogStickMax = 0x7FFF;
}
