# Warblade Service Release

A decompile of **Warblade 1.34**, created by Edgar M. Vigdal.

Current Warblade SR version: **2.0**.

This repository contains no game assets and as such cannot be used without them. Compiling also requires access to the game assets for the icon file.

This decompilation is possible mainly due to the fact that Edgar released 1.34 as a debug build. This made it much more feasible to convert ~660 compiled functions back into their C++ form, on top of being able to cut out the PTK library (which is not available anymore) and delink it back into an object file. However this also means it's not possible to compile an exact byte-matching .exe to the original version, since it contains some debug-time patching that the compiler didn't do.

As a disclaimer, this project did use a lot of LLM work (shoutout to Claude), particularly for determining the build flags, the grunt work in converting x86 back to C++, and delinking PTK. It also helped quite a bit with documentation and cleaning up the code while preserving function-level byte matching, so please excuse the comment litter.

-------------

This repo contains two branches:
- `decompile` for a function-wise binary-matching version of the code with the original PTK engine and BASS library
- `main`, built on top of the `decompile` branch, removes PTK and replaces it with SDL, additionally with some improvements for modern PCs

Release SR1 is built off of the `main` branch.

### SR branch changes

I intentionally didn't want to change too much about the game without good reason, to preserve the game as it stands. So most changes correspond to the underlying rendering system the game uses, and the others relating to things that don't serve a purpose anymore

- 64bit executable, optimized build
- Uses SDL as the rendering & sound engine. PTK and BASS have been dropped as a result
- Resizable window, letterboxing and scaling is applied automatically
- Fullscreen is now borderless instead of exclusive, and can be toggled while in-game without needing a restart
- Settings not applicable with SDL or modern windows (e.g. 16 bit color, hardware sound mixing) have been removed
- VSync is now an option, enabled by default, toggleable by `Alt + V`
- SDL renderer is now an option that can be cycled through, for options such as DirectX 12, Vulkan, and OpenGL. Default is "auto", meaning that it will pick one that works for your system
- Interpolation is now an option to make the game render at >60FPS, however it has a few caveats (see below)
- Anything relating to online services has been either stubbed out or removed
- Links that the main menu opened now open their archive.org versions instead
- Anti-cheat mechanisms (such as obfuscating score tables by multiplying them by 7, reading the process list for trainer executables, intentionally crashing the game upon finding tampered highscores) have been removed
- Asset loading during startup has been significantly sped up by detatching it from the framerate
- Intro sequence now handles window closing instead of only closing when it reaches the main menu
- Gamepads work (not part of the original game): left stick or d-pad to move, A or right trigger to fire, B, X or left trigger for rockets, Start to pause, Back for the profile key. Controllers with a standard layout (Xbox, PlayStation, Switch Pro, and Steam controllers through Steam Input) are picked up automatically: the first pad plays player 1, the second player 2

#### Cheats

Not part of the original game. During play, type `GALAGA` to toggle cheats (the code only uses letters that aren't in-game hotkeys). While cheats are on, the number keys (top row or numpad) apply to the current player:

| Key | Cheat |
| --- | --- |
| `1` | Extra life |
| `2` | +1000 money (up to the wallet size) |
| `3` | Skip level (warp) |
| `4` | Best weapon: WAR.I.PLASMA with max bullets |
| `5` | Full armour |
| `6` | Max rockets + super autofire |
| `7` | Shield |
| `8` | Smart bomb |
| `9` | God mode on/off |

Scores and profile stats from a cheated game are recorded as normal.

#### Interpolation

Due to the way Warblade intertangles logic ticks and rendering, it's not possible to get true >60FPS rendering without altering the game logic, which isn't want I want to do.

As a next best thing interpolation is used instead, which basically means two things:
- Subpixel positioning is enabled (original Warblade would snap to exact pixel coordinates)
- Draw calls for each 60FPS tick are recorded, and any draws of the same sprite that are detected to be moved from their previous position is able to have in-between frames calculated

The original logic + render ticks are instead ran on a fixed 16.66ms timer, and the "draws" from it aren't actually drawing anything, they're just recorded by the interpolation system to determine what to render when the next frame is requested.

While this does work for quite a few things (notably borders and enemies), there are a few caveats:
- Text can appear less solid & with artifacts around each character (this one is due to the subpixel positioning, as the original code assumes the pixel coordinate snapping)
- Text can jitter and appear slightly corrupted during animation
- Because I can't predict frames in the future without simulating them and irreversably updating global state, the displayed frame has to be in the past. Running at 120FPS, this is about 8ms of added input lag
- Since the game still renders internally at 800x600 which is then scaled to the window size, even with subpixel positioning there's not enough pixel space to represent certain fractional steps the interpolation wants to perform. Meaning that some effects that move slowly across the screen (like background stars) are still going to appear like they "snap" rather than move smoothly like you'd expect.

I may revisit this in the future with a completely new rendering system for Warblade, but for now this is the best I can do I think.

If you don't like how it looks, then you can easily turn it off in the settings. The default value for this option is `auto`, meaning that it's only enabled when your monitor's refresh rate is something other than 60 (since it won't make a difference otherwise) or you have VSync disabled.

### Installing

1. Copy `warblade-sr.exe` to your Warblade 1.34 installation folder.
2. Run the file.

All the dependencies `warblade-sr.exe` needs are built into the file, so you don't need to install anything.

You can also replace `warblade.exe` with this version, but I don't recommend it.


# Building


## `decompile` byte-matching branch

### Requirements

- **Visual Studio 2008** C++ RTM (not SP1) (Express version is fine too)
- A Warblade 1.34 installation

### Building

From a command prompt in the repository:

```bat
set WARBLADE_GAME_DIR=C:\path\to\Warblade
build.bat
```

`WARBLADE_GAME_DIR` defaults to `..\game`. The result is `build\warblade.exe` (a debug-CRT build, like the original).

## `main` modern branch

### Requirements

- **Visual Studio 2026** with **Desktop development with C++** installed
- vcpkg for SDL and its satellite libraries
- A Warblade 1.34 installation

### Building

From a command prompt in the repository:

```bat
set WARBLADE_GAME_DIR=C:\path\to\Warblade
build.bat
```

- `build.bat` (or `build.bat release`) builds `build\vs2026-release\warblade.exe`;
  `build.bat debug` and `build.bat asan` (AddressSanitizer) build into their own folders.
- `WARBLADE_GAME_DIR` defaults to `..\game`. The build folder is runnable: `data\` in it is a
  link to your installation's `data\` folder.
- `run.bat [release|debug|asan]` starts that build from its folder.

### Linux

The `main` branch also builds natively on Linux with GCC or Clang. You still need the data from a Warblade 1.34 installation (its `data/` folder).

```sh
sudo apt install build-essential cmake ninja-build git zlib1g-dev \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev \
    libxtst-dev libasound2-dev libpulse-dev libgl-dev libegl-dev
WARBLADE_GAME_DIR=/path/to/Warblade ./build-linux.sh
cd build/linux-release && ./warblade
```

- `build-linux.sh` builds SDL3, SDL3_image and SDL3_mixer from source into `build/deps` the first time (statically, with the same features as the Windows build), then the game into `build/linux-release` (`build-linux.sh debug`: `build/linux-debug`), with a `data/` link to your installation's.
- Profiles, settings and screenshots go to `~/Documents/warblade` (or `~/warblade` when there is no `Documents` folder).
- The game's Windows paths are translated on the fly (`SysPath` in `src/core/sdl.c`): backslashes become slashes, and file names match regardless of case, as they did on Windows. The Microsoft C runtime calls it uses map onto POSIX in `include/posix/io.h`.

### Browser

The browser game presents at the display's pixel density while keeping the original 800×600 game and input coordinates. Shrinking uses smooth texture filtering to preserve thin text strokes; enlarged output keeps the pixel-art look. The game framebuffer stays at its original resolution and this adds no post-processing passes.

The same code also builds to WebAssembly with [Emscripten](https://emscripten.org). Game assets may be supplied by the server or selected locally; all assets stay in memory for the current page visit. Each player account owns exactly one game profile; profiles, settings and saves are stored on the server. The six high-score tables are shared and merged transactionally on the server, including when players finish at the same time.

```sh
source /path/to/emsdk/emsdk_env.sh
./build-web.sh
```

The page opens directly into a title-art loading view and starts the game automatically. The container serves that artwork from the mounted game archive; it is not bundled in the image. Once loaded, a remembered account opens directly; otherwise the native sign-in screen takes over.

The page needs the account API and a same-origin reverse proxy; opening `build/web/index.html` directly or serving it with `python -m http.server` will not provide saves. The game runs its blocking loop with Asyncify. Touch controls and gamepads work in the browser.

### Docker

`docker compose up --build -d` starts the static game, account API and a local Caddy proxy at `http://localhost:8080`. Put your own Warblade 1.34 assets in `game/data` first. The API database lives in the persistent `api-state` volume. Back it up with SQLite's online backup API or `sqlite3 .backup`, rather than copying a live WAL file alone.

The GitHub workflow is configured to publish `ghcr.io/rkj/warbladesr` (static game) and `ghcr.io/rkj/warbladesr-api` (account API) after these changes reach upstream `main`. Pin the same source commit for both images. The static image accepts only GET and HEAD; route `/api/*` to the API container through a reverse proxy at the same origin. Set `WARBLADE_PUBLIC_ORIGIN` to that exact HTTPS origin in a public deployment, keep the API database private, and set `WARBLADE_TRUSTED_PROXY_IPS` only if the proxy overwrites `X-Real-IP` and its peer address is stable. Set `WARBLADE_SESSION_MODE=cookie` for the browser page: the API uses Argon2id password hashes and a 30-day HttpOnly, Secure, SameSite session cookie; its save paths are account-scoped and checked with revisions.

The browser game's built-in player sign-in/create window authenticates with the server and loads its single profile. It has no second password or profile selector; its existing profile screen remains available for stats, reset, backup and restore. Returning or reloading restores the same server account until its 30-day session expires or you sign out. No password or game saves are stored in browser storage. A single-player run saves automatically when the shop opens; use My Profile → Continue Game to resume its checkpoint. High scores are submitted by the browser, so the shared table prevents lost updates but cannot prove that a score was earned without server-validated gameplay. See `docs/web-security.md` for the security boundary and limits.


# Used libraries

### `decompile` branch

- [PTK](http://ptk.phelios.com)
- [BASS](https://www.un4seen.com) (distributed with your Warblade install)
- [zlib](https://zlib.net), [libpng](http://www.libpng.org/pub/png/libpng.html) and libjpeg, built into PTK

### `main` branch

- [SDL3](https://www.libsdl.org), [SDL3_image](https://github.com/libsdl-org/SDL_image) and [SDL3_mixer](https://github.com/libsdl-org/SDL_mixer)
- [zlib](https://zlib.net), [libpng](http://www.libpng.org/pub/png/libpng.html) and [libjpeg-turbo](https://libjpeg-turbo.org), for images
- [libxmp](https://github.com/libxmp/libxmp) and [dr_mp3](https://github.com/mackron/dr_libs), for music

This software is based in part on the work of the Independent JPEG Group.
