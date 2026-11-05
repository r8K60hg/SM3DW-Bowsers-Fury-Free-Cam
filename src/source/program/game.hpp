#pragma once

/*
 * Super Mario 3D World + Bowser's Fury (0100 2860 0EBD A000), version 1.1.0.
 *
 * Offsets are relative to the start of the main executable and were taken from
 * fruityloops1/bf-multiplayer (GPL-2.0), which uses them for its debug camera.
 * They are only valid for the build ID below; the mod refuses to patch anything else.
 */

#include <common.hpp>

namespace game {

    /* First 16 bytes of the main NSO's GNU build ID for v1.1.0. */
    constexpr u8 BuildId110[16] = {
        0x89, 0x16, 0x87, 0xf0, 0x16, 0xa1, 0x8f, 0x17,
        0x73, 0xd4, 0xa8, 0x8e, 0xbf, 0x8a, 0x97, 0x3c,
    };

    namespace offsets {
        /* Bowser's Fury camera system: final look-at camera build, (poser, sead::LookAtCamera*). */
        constexpr ptrdiff_t RsMakeLookAtCamera = 0x00923170;
        /* Bowser's Fury camera pose updater step that switches cameras; skipped while the freecam is on. */
        constexpr ptrdiff_t RsPoseUpdaterSwitch = 0x00923c90;

        /* Classic al::CameraPoser::makeLookAtCamera overrides (3D World courses, cutscenes, etc). */
        constexpr ptrdiff_t PoserAnim            = 0x00906010;
        constexpr ptrdiff_t PoserEntrance        = 0x00906500;
        constexpr ptrdiff_t PoserFixedAll        = 0x00906a80;
        constexpr ptrdiff_t PoserFixedPoint      = 0x00907060;
        constexpr ptrdiff_t PoserKinopioBrigade  = 0x00907960;
        constexpr ptrdiff_t PoserParallel        = 0x0090cc10;
        constexpr ptrdiff_t PoserParallelOnRail  = 0x0090e510;
        constexpr ptrdiff_t PoserProgramable     = 0x0090ec00;
        constexpr ptrdiff_t PoserRail            = 0x0090f180;
        constexpr ptrdiff_t PoserRailTower       = 0x0090f9a0;
        constexpr ptrdiff_t PoserTower           = 0x00910580;
        constexpr ptrdiff_t PoserZoom            = 0x00912130;
    }
}
