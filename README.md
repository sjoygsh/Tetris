# Tetris

A polished Tetris clone built in C with SDL2, featuring an Apple-inspired light theme, smooth animations, and modern gameplay mechanics.

![C11](https://img.shields.io/badge/C11-Standard-blue)
![SDL2](https://img.shields.io/badge/SDL2-Graphics-green)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)

## Features

- **Apple-inspired UI** — Clean white theme with rounded cards, soft shadows, and glossy blocks
- **7-bag randomizer** — Fair piece distribution (every piece guaranteed within each set of 7)
- **Main menu** — Start, Leaderboard, Controls, Quit with fade transitions
- **Hard drop effects** — Vertical trail streaks and particle bursts on landing
- **Line clear animation** — White flash overlay with fade-out
- **Combo system** — Tracks consecutive clears with floating "Combo x3" text
- **Back-to-back Tetris** — Special announcement for consecutive 4-line clears
- **Piece spawn animation** — Smooth fade-in when new pieces appear
- **Lock delay pulse** — Brightness oscillation while piece is about to lock
- **Dynamic background** — Ambient floating particles
- **Ghost piece** — Semi-transparent preview showing where the piece will land
- **Leaderboard** — Persistent top-5 high scores saved to disk
- **Sound effects** — Move, rotate, drop, clear, and game over sounds

## Controls

| Key | Action |
|-----|--------|
| Left / Right | Move piece sideways |
| Up | Rotate piece clockwise |
| Down | Soft drop (fall faster) |
| Space | Hard drop (instant slam) |
| P | Pause / Resume |
| R | Restart game |
| Esc | Return to main menu |

## Building

### Windows

**Prerequisites:** Git, CMake (3.15+), Visual Studio or Build Tools with C/C++ workload.

```batch
build.bat            # Build and run
build.bat --build    # Build only
build.bat --run      # Run only
```

The script automatically installs vcpkg, SDL2 libraries, downloads fonts, and generates sound effects.

### Linux

```bash
chmod +x build.sh    # Make executable (first time only)
./build.sh           # Build and run
./build.sh --build   # Build only
./build.sh --run     # Run only
```

The script automatically installs SDL2 via your package manager (apt/dnf/pacman/zypper/apk), downloads fonts, and generates sound effects.

### Manual Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

You'll need SDL2, SDL2_ttf, and SDL2_mixer development libraries installed, plus font files (`font.ttf`, `font_bold.ttf`, `font_light.ttf`) in the `assets/` directory.

## Project Structure

```
tetris/
  src/
    main.c       Entry point
    defs.h       Constants, enums, structs (central definitions)
    game.c/h     Core logic: init, game loop, gravity, scoring, particles, leaderboard
    piece.c/h    Tetromino shapes (SRS), colors, 7-bag randomizer
    render.c/h   All drawing: board, blocks, menu, sidebar, overlays, effects
    input.c/h    Keyboard handling for menu and gameplay
    board.c/h    Board grid: collision, locking, line clearing
    audio.c/h    Sound effect loading and playback
  assets/
    font.ttf         Nunito Regular
    font_bold.ttf    Nunito Bold
    font_light.ttf   Nunito Light
    *.wav            Generated sound effects
    scores.dat       Leaderboard data (auto-created)
  build.sh           Linux build script (auto-installs deps)
  build.bat          Windows build script (uses vcpkg)
  CMakeLists.txt     CMake build configuration
```

## Technical Details

- **Language:** C11 with CMake build system
- **Graphics:** SDL2 + SDL2_ttf (font rendering) + SDL2_mixer (audio)
- **Rendering:** Custom rounded rectangles via midpoint circle algorithm, glossy block shading with highlight/shadow layers
- **Randomizer:** Fisher-Yates shuffle for uniform 7-bag distribution
- **Particle system:** Background ambient particles (50) + hard-drop burst particles (24 max)
- **Animations:** Spawn fade-in (80ms), line clear flash (300ms), drop trail fade (250ms), floating text scale+drift (1200ms), menu fade transition (400ms)
- **Scoring:** Lines cleared directly added to score, with combo multiplier tracking
- **Persistence:** Binary file leaderboard with sorted insertion (top 5)
- **Frame rate:** ~60 FPS via SDL_Delay with VSync

## License

This project is licensed under the [MIT License](LICENSE). Feel free to use, modify, and distribute.
