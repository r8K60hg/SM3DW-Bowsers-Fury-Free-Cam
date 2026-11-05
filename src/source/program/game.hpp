#pragma once

/*
 * Super Mario 3D World + Bowser's Fury (0100 2860 0EBD A000).
 *
 * Each supported game version is identified by the first 16 bytes of the main
 * executable's GNU build ID. The mod only patches a build listed here.
 *
 * v1.1.0 offsets come from fruityloops1/bf-multiplayer (GPL-2.0), which uses them
 * for its debug camera. v1.0.0 offsets were mapped from those against a 1.0.0 dump.
 */

#include <common.hpp>

namespace game {

    struct CameraOffsets {
        /* al::CameraPoser_RS::makeLookAtCameraPrev(sead::LookAtCamera*) const — Bowser's Fury camera build. */
        ptrdiff_t RsMakeLookAtCamera;
        /* al::CameraPoseUpdater::trySwitchCamera() — skipped while the freecam is on. */
        ptrdiff_t RsPoseUpdaterSwitch;

        /* Classic al::CameraPoser::makeLookAtCamera overrides (3D World courses, cutscenes, ...). */
        ptrdiff_t PoserAnim;
        ptrdiff_t PoserEntrance;
        ptrdiff_t PoserFixedAll;
        ptrdiff_t PoserFixedPoint;
        ptrdiff_t PoserKinopioBrigade;
        ptrdiff_t PoserParallel;
        ptrdiff_t PoserParallelOnRail;
        ptrdiff_t PoserProgramable;
        ptrdiff_t PoserRail;
        ptrdiff_t PoserRailTower;
        ptrdiff_t PoserTower;
        ptrdiff_t PoserZoom;
    };

    struct GameVersion {
        const char* name;
        u8 buildId[16];
        CameraOffsets camera;
    };

    constexpr GameVersion SupportedVersions[] = {
        {
            "1.1.0",
            { 0x89, 0x16, 0x87, 0xf0, 0x16, 0xa1, 0x8f, 0x17, 0x73, 0xd4, 0xa8, 0x8e, 0xbf, 0x8a, 0x97, 0x3c },
            {
                .RsMakeLookAtCamera  = 0x00923170,
                .RsPoseUpdaterSwitch = 0x00923c90,
                .PoserAnim           = 0x00906010,
                .PoserEntrance       = 0x00906500,
                .PoserFixedAll       = 0x00906a80,
                .PoserFixedPoint     = 0x00907060,
                .PoserKinopioBrigade = 0x00907960,
                .PoserParallel       = 0x0090cc10,
                .PoserParallelOnRail = 0x0090e510,
                .PoserProgramable    = 0x0090ec00,
                .PoserRail           = 0x0090f180,
                .PoserRailTower      = 0x0090f9a0,
                .PoserTower          = 0x00910580,
                .PoserZoom           = 0x00912130,
            },
        },
        /*
         * 1.0.0: located in the 1.0.0 executable. The camera code is unchanged between versions,
         * only shifted: classic posers by -0x6860, Bowser's Fury (RS) camera by -0x6800.
         * Verified: every pose function is slot 6 of its own vtable, every address is a function
         * start, and the named 1.1.0 functions on both sides of each one shift by the same amount.
         */
        {
            "1.0.0",
            { 0x9f, 0x7e, 0xfc, 0x2f, 0xb9, 0x65, 0x3e, 0x5c, 0xde, 0x03, 0x03, 0x04, 0x78, 0xf2, 0x3e, 0xda },
            {
                .RsMakeLookAtCamera  = 0x0091c970,
                .RsPoseUpdaterSwitch = 0x0091d490,
                .PoserAnim           = 0x008ff7b0,
                .PoserEntrance       = 0x008ffca0,
                .PoserFixedAll       = 0x00900220,
                .PoserFixedPoint     = 0x00900800,
                .PoserKinopioBrigade = 0x00901100,
                .PoserParallel       = 0x009063b0,
                .PoserParallelOnRail = 0x00907cb0,
                .PoserProgramable    = 0x009083a0,
                .PoserRail           = 0x00908920,
                .PoserRailTower      = 0x00909140,
                .PoserTower          = 0x00909d20,
                .PoserZoom           = 0x0090b8d0,
            },
        },
    };
}
