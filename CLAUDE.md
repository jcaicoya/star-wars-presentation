# Star Wars Presentation

## Build

```bash
# Configure (clang++ on Linux)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++

# Build
cmake --build build -j$(nproc)

# Run
./build/StarWarsText
```

On Windows: open in CLion with MSVC, no extra flags needed.

## Cross-platform

`CMakeLists.txt` sets `CMAKE_PREFIX_PATH` per platform:

- **Windows:** `C:/Qt/6.7.3/msvc2022_64` (MSVC 2022)
- **Linux:** `$HOME/Qt/6.10.1/gcc_64` (clang++ 18)

## Git

- Remote uses SSH alias `github.com-personal` (personal account `jcaicoya`)
- The company account `ns-jcaicoya` does NOT have push access

## Architecture

- `src/main.cpp` — entry point
- `src/MainWindow.cpp/.h` — launcher, editor, mode switching, save/load
- `src/CrawlWindow.cpp/.h` — all rendering and animation (6 phases: Intro, Logo, Crawl, Spaceflight, ThreeStars, Ending) + hyperspace effect (Tunnel / Particles modes)
- `src/CrawlContent.h` — data structs (`CrawlContent`, `Star`, `StarDefinition`, `InputState`, `LiveFlightState`)
- `src/StarMapWidget.cpp/.h` — 3D cube star map viewer (projection, drag, keyboard/wheel interaction)
- `src/StarsEditorWidget.cpp/.h` — star list + property form + StarMapWidget integration
- `src/TextIO.cpp/.h` — file I/O, section parser (`[intro]`, `[logo]`, `[title]`, `[subtitle]`, `[body]`), JSON star parser/serializer
- `resources/text.txt` — crawl content
- `resources/stars.json` — star map (4 stars)

## stars.json contract

Both Live and Video game modes consume this file. Protect this schema:

`text`, `position.x/.y/.z`, `colors.core`, `colors.glow`, `radius`

## Editor coordinate system

The star editor shows positions as **0–100%** on all axes. Internally these map to:

- X: -640 to 640 (symmetric)
- Y: -520 to 440 (asymmetric, Y-up = screen-up in both editor and live)
- Z: editor range is **0 to 2360** (the ship starts at raw Z=-1000, but that runway is hidden from the editor)

## Hyperspace modes

The Ending phase starts with a hyperspace jump (selectable in launcher):

- **Tunnel** — radial gradient tunnel with star streaks, screen shake, white flash
- **Particles** — spawned particle streams with trails, capped at 500 particles, precomputed trig

Both use 3 sub-phases: Charge-up (30 ticks), Jump (45 ticks), Deceleration (60 ticks). Constants are in `paintEnding` and `tickEnding`.

## Current state

All core features work end-to-end:

- Live mode: Intro, Logo, Crawl, Spaceflight (deterministic leg-based travel), ThreeStars (cinematic per-star showcase), Ending (hyperspace + logo/summary reveal)
- Video game mode: free-flight with HUD
- Launcher: mode cards + fullscreen (default) / current size + hyperspace mode selector
- Tabbed editor for text.txt and stars.json (3D cube map with 0–100% coordinates, 6-direction dotted projection lines)

## Known code issues

- ThreeStars timing constants and Ending finale text are hardcoded in C++
- Particles hyperspace mode: performance is acceptable but could be better on low-end hardware

## Next steps (priority order)

1. Mouse button as spacebar in live mode (advance phases/stars)
2. Remove F11 fullscreen toggle
3. Improve star look and feel (surface rendering, visual options)
4. Auto-trigger hyperspace after reaching last star (currently manual)
5. Tweak hyperspace timing and visuals
6. Improve transition to Planet phase (post-hyperspace)
7. Define header text for Planet phase
8. Improve Planet phase element appearance/animation
9. Add final sequence after Planet: closing message, thank you / hearts, then close
10. Add go-back navigation option
11. Clean unused code and dead elements
12. Code refactoring pass
13. Sound options

## Pending decisions

1. Evaluate whether Qt Widgets + QPainter is the right stack long-term, or if QML / a 3D engine would improve iteration on camera, effects, and cinematic polish
2. Consider externalizing hardcoded timing constants and finale text into data files
3. Verify intent: 4th star ("Stay curious. Keep building.") only appears in Spaceflight, not in ThreeStars or Planet summary
