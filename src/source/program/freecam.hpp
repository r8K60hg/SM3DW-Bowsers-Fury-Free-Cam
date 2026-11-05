#pragma once

#include <common.hpp>
#include <cstddef>
#include "program/fmath.hpp"

namespace freecam {

    /* Mirrors sead::LookAtCamera (vtable + 3x4 view matrix + pos/at/up). */
    struct LookAtCamera {
        void* vtable;
        float viewMtx[3][4];
        fm::Vec3 pos;
        fm::Vec3 at;
        fm::Vec3 up;
    };
    static_assert(offsetof(LookAtCamera, pos) == 0x38);
    static_assert(offsetof(LookAtCamera, at)  == 0x44);
    static_assert(offsetof(LookAtCamera, up)  == 0x50);

    struct PadInput {
        u64 buttons;
        float lx, ly;   /* -1..1, deadzone already applied */
        float rx, ry;
    };

    /* Reads the controller without being affected by the input block. */
    using PadReader = PadInput (*)();

    void SetPadReader(PadReader reader);

    /* Called after the game has produced a camera; overrides it while the freecam is on. */
    void OnCameraBuilt(LookAtCamera* camera);

    bool IsEnabled();

    /* True while the player is flying the camera and the game should see a neutral controller. */
    bool ShouldBlockGameInput();
}
