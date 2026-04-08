# Star Wars Presentation

Technical reference for AI-assisted development sessions.

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
- `src/MainWindow.cpp/.h` — launcher with mode cards, separate text/stars editor pages, save/load
- `src/CrawlWindow.cpp/.h` — all rendering and animation (6 phases: Intro, Logo, Crawl, Spaceflight, Hyperspace, Outro) + hyperspace effect (Tunnel / Particles)
- `src/CrawlContent.h` — data structs (`CrawlContent`, `Star`, `StarDefinition`)
- `src/StarMapWidget.cpp/.h` — 3D cube star map viewer (projection, drag, keyboard/wheel interaction)
- `src/StarsEditorWidget.cpp/.h` — star list + property form (text, type, colors, radius, position) + StarMapWidget integration
- `src/LineNumberTextEdit.cpp/.h` — QPlainTextEdit subclass with line number gutter and column guide
- `src/TextIO.cpp/.h` — file I/O, section parser, JSON star parser/serializer, `effectiveBodyLineLimit()`
- `resources/text.txt` — crawl content (sections: `[intro]`, `[logo]`, `[title]`, `[subtitle]`, `[body]`, `[planetheader]`, `[finalsentence]`)
- `resources/stars.json` — star/planet definitions

## stars.json contract

Both Live and Video game modes consume this file. Schema fields:

`text`, `position.x/.y/.z`, `colors.core`, `colors.glow`, `radius`, `type` (`"star"` or `"planet"`)

Stars render as spheres with offset radial gradient. Planets render as spheres with Saturn-style rings (QPainterPath subtraction, two-pass clip-rect rendering).

## Editor coordinate system

The star editor shows positions as **0-100%** on all axes. Internally these map to:

- X: -640 to 640 (symmetric)
- Y: -520 to 440 (asymmetric, Y-up = screen-up in both editor and live)
- Z: editor range is **0 to 2360** (the ship starts at raw Z=-1000, but that runway is hidden from the editor)

## Crawl body line limit

Body column width is determined by `effectiveBodyLineLimit()` in TextIO: max line length across body lines, clamped to [30, 40]. Lines are truncated at this limit in the crawl. The text editor shows a dashed vertical guide at this column. Non-last paragraph lines are justified; last lines are left-aligned.

## Hyperspace modes

Selectable in launcher (Tunnel / Particles). Both use 3 sub-phases:

- Charge-up (30 ticks), Jump (45 ticks), Deceleration (60 ticks)
- Constants are in `paintHyperspace` and `tickHyperspace`
- Tunnel: radial gradient tunnel with star streaks, screen shake, white flash
- Particles: spawned particle streams with trails, capped at 500, precomputed trig
- Settle phase (ticks > kHyperspaceEnd) paints starfield as transition to Outro

## Outro sub-phases

Controlled by `m_outroFinalTick`:

- `< 0` — star recap: header text, path with gradient segments, data-driven star/planet rendering from `m_goalStars`, star labels
- `>= 0` — final message: blue italic text, fade in / hold / fade out, then close on next advance

## Logo rendering

Hollow outlined characters via QPainterPath (no fill). Outer glow pass + main stroke. Exponential shrink animation.

## Animation system

- Tick-based at 16 ms intervals (~62.5 fps)
- Phase state machine: Intro -> Logo -> Crawl -> Spaceflight -> Hyperspace -> Outro
- Live mode: deterministic leg-based travel between goal stars, auto-triggers hyperspace 30 ticks after reaching the last star
- Video game mode: free-flight with WASD/arrow controls and HUD
