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
- `src/CrawlWindow.cpp/.h` — all rendering and animation (6 phases: Intro, Logo, Crawl, Spaceflight, ThreeStars, Ending)
- `src/CrawlContent.h` — data structs (`CrawlContent`, `Star`, `StarDefinition`, `InputState`, `LiveFlightState`)
- `src/TextIO.cpp/.h` — file I/O, section parser (`[intro]`, `[logo]`, `[title]`, `[subtitle]`, `[body]`), JSON star parser
- `resources/text.txt` — crawl content
- `resources/stars.json` — star map (4 stars)

## stars.json contract

Both Live and Video game modes consume this file. Protect this schema:

`text`, `position.x/.y/.z`, `colors.core`, `colors.glow`, `radius`

## Current state

All core features work end-to-end:

- Live mode: Intro, Logo, Crawl, Spaceflight (deterministic leg-based travel), ThreeStars (cinematic per-star showcase), Ending (finale with summary)
- Video game mode: free-flight with HUD
- Launcher + tabbed editor for text.txt and stars.json

## Known code issues

- ThreeStars timing constants and Ending finale text are hardcoded in C++

## Pending decisions

1. Evaluate whether Qt Widgets + QPainter is the right stack long-term, or if QML / a 3D engine would improve iteration on camera, effects, and cinematic polish
2. Consider externalizing hardcoded timing constants and finale text into data files
3. Verify intent: 4th star ("Stay curious. Keep building.") only appears in Spaceflight, not in ThreeStars or Planet summary
