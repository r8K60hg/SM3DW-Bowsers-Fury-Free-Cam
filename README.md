# BF Freecam

A free-flying camera for **Super Mario 3D World + Bowser's Fury** (Switch), for emulators (Ryujinx / Ryubing, Eden, Citron and other Yuzu forks) and Atmosphère.

Toggle it on, fly anywhere with the sticks, park the camera and keep playing Mario from that angle, roll the horizon for photos, then toggle off to get the normal camera back.

> **Game version: 1.1.0 only.** The 1.2.x updates (the Switch 2 patches) move the game's code around. On any other version the mod detects the mismatch at boot and does nothing, so it can't crash your game; it just won't activate.

---

## Controls

| Action | Button |
|---|---|
| **Toggle free camera on/off** | Hold **L**, click **R3** (right stick) |
| Move (forward/back, strafe) | Left stick |
| Look | Right stick |
| Rise / descend | **R** / **L** |
| Fast / slow (hold) | **ZR** / **ZL** |
| Base speed up / down | D-pad **Up** / **Down** |
| Roll (tilt) camera | D-pad **Left** / **Right** (hold) |
| Reset roll + speed | **Minus** |
| Park camera & control Mario | **L3** (click left stick); press again to fly |

While you're flying, Mario ignores the controller. In **parked** mode the camera stays where you left it and the controller goes back to the game, which is great for filming Mario from a fixed angle.

**Photo tip:** press D-pad Down for the game's Snapshot mode (freezes time), then turn on the freecam and fly around the frozen scene. Use your emulator's screenshot key, or park the camera (L3) to give the controller back to the Snapshot UI.

---

## Install

You need two files, `subsdk9` and `main.npdm`, from a build (see **Getting the files** below).

### Ryujinx / Ryubing
1. Right-click the game → **Open Mods Directory**.
2. Copy the `BFFreecam` folder in there so you end up with
   `…/mods/contents/010028600ebda000/BFFreecam/exefs/subsdk9` and `main.npdm`.
3. Launch the game. Make sure the **1.1.0** update is the one selected under *Manage Title Updates*.

### Eden / Citron / other Yuzu forks
1. Right-click the game → **Open Mod Data Location**.
2. Copy the `BFFreecam` folder in there: `…/load/010028600EBDA000/BFFreecam/exefs/subsdk9` and `main.npdm`.
3. Check it's enabled under the game's **Properties → Add-Ons**, and that update **1.1.0** is installed.

### Modded Switch (Atmosphère)
Copy the `atmosphere` folder from the Atmosphère package to the root of your SD card.

**Conflicts:** only one code mod can use `subsdk9` + `main.npdm` at a time, so disable other ExLaunch-based mods (e.g. the Bowser's Fury co-op or online mods) while using this one. Texture/model (romfs) mods are fine.

**Checking it loaded:** with debug logging on, the emulator log shows `[BFFreecam] Loaded…`, or `[BFFreecam] Unsupported game version…` if you're not on 1.1.0.

---

## Getting the files (building)

The mod is C++ and has to be compiled with Nintendo Switch homebrew tools. Pick whichever is easiest:

**Option A: GitHub (no installs).**
Create a new GitHub repository, upload this whole folder (including the hidden `.github` folder), then open the **Actions** tab. The *Build BF Freecam* workflow runs automatically; open the finished run and download **BFFreecam-emulator** (or **BFFreecam-atmosphere**).

**Option B: Docker (Windows, macOS, Linux).**
Install Git and Docker Desktop, then run:
- Windows (PowerShell): `powershell -ExecutionPolicy Bypass -File .\build.ps1`
- macOS / Linux: `./build.sh`

The results land in `dist/`.

**Option C: local devkitPro.** With devkitPro (devkitA64 + libnx + switch-tools) installed and `DEVKITPRO` set: `./build.sh --local`.

The build pins ExLaunch to commit `f698816d` and the devkitA64 image digest that the Bowser's Fury co-op mod builds with, so it stays reproducible.

---

## Customising

Everything you'd want to tweak is in [`src/source/program/controls.hpp`](src/source/program/controls.hpp): button mapping, move/look/roll speed, deadzone, smoothing (cinematic glide), Y/X inversion, and whether Mario is frozen while flying. Edit, rebuild, reinstall.

---

## How it works

- **ExLaunch module** (`subsdk9`) loaded next to the game's own code.
- **Camera:** hooks the final look-at camera of Bowser's Fury's camera system plus the `makeLookAtCamera` of 12 classic camera types used in 3D World courses. After the game builds its camera, the mod overwrites position / target / up when the freecam is on. While it's on, Bowser's Fury's camera-switch step is paused so the game doesn't fight it.
- **Input:** reads the controller directly through `nn::hid` (Pro Controller, handheld, or dual Joy-Con on player 1). While flying it hands the game a neutral controller so Mario stands still.
- **Timing:** motion uses the system tick, so speed is the same at 30 or 60 fps.
- **Watchdog:** if the game stops building cameras for half a second (pause menu, loading screen), the freecam switches itself off so you never get stuck with a frozen controller. Toggle it back on afterwards.
- **Safety:** at boot it reads the game's build ID from the executable and only installs hooks on `891687f016a18f17…` (v1.1.0).

Source layout (`src/` is copied over a clean ExLaunch checkout at build time):

```
src/config.json, config.mk          NPDM + build config for title 010028600EBDA000
src/source/program/main.cpp         hooks, controller reading, version check
src/source/program/freecam.cpp      camera state machine and flight model
src/source/program/controls.hpp     ← your settings
src/source/program/game.hpp         v1.1.0 offsets + build ID
src/source/program/hid.hpp          minimal nn::hid declarations
src/source/program/fmath.hpp        small libm-free math
```

## Known limitations

- **v1.1.0 only.** Supporting 1.2.x means finding the same functions in that executable; the offsets live in `game.hpp`.
- **3D World courses:** the mod covers the 12 classic camera types the Bowser's Fury modding community has mapped. If a particular course or cutscene ignores the freecam, it uses a camera type that isn't hooked yet.
- **Player 1 only.** Other players' controllers aren't blocked while flying.
- Field of view isn't adjustable (it lives in the projection, not the look-at camera).

## Credits & license

- Camera hook offsets for v1.1.0: [fruityloops1/bf-multiplayer](https://github.com/fruityloops1/bf-multiplayer) (GPL-2.0), from its debug camera tool.
- Hooking framework: [shadowninja108/exlaunch](https://github.com/shadowninja108/exlaunch) (GPL-2.0).
- NPDM configuration and pinned toolchain image: [Gudui/BowsersFuryLocalCoop](https://github.com/Gudui/BowsersFuryLocalCoop) (GPL-2.0).
- Build-ID lookup follows the approach used by Atmosphère's crash reporter and LibHakkun.

Licensed under **GPL-2.0** (see `LICENSE`). Unofficial fan project; not affiliated with Nintendo. Contains no game code or assets. Use your own legally dumped copy of the game.
