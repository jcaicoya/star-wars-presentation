# StarWarsText

Qt/C++ presentation tool inspired by the Star Wars opening crawl. Features a complete cinematic pipeline (intro → logo → crawl → spaceflight → star showcase → planet finale) plus a free-flight video game mode and an integrated editor.

## App flow

On startup, the app shows a launcher screen with card selectors for:

- `Live mode` — full cinematic presentation
- `Edit mode` — text and star editor
- `Video game mode` — free-flight star-map exploration
- `Full screen` (default) / `Current size` — display style
- `Tunnel` / `Particles` — hyperspace visual mode

Toolbar shortcuts:

- `Ctrl+L` → Live
- `Ctrl+E` → Edit
- `Ctrl+G` → Video game
- `Ctrl+1` → Full screen
- `Ctrl+2` → Current size
- `Ctrl+3` → Tunnel hyperspace
- `Ctrl+4` → Particles hyperspace
- `Ctrl+S` → Save edited files
- `F11` → Toggle fullscreen in the presentation window
- `Esc` → Close presentation window and return to the launcher
- `Space` → Advance to next phase/star (in Live mode)

## Modes

### Live mode

Runs a 6-phase cinematic sequence:

1. **Intro** — italic blue text, fade in/hold/fade out
2. **Logo** — large yellow text that shrinks exponentially
3. **Crawl** — perspective-projected scrolling text over a 3D starfield; auto-transitions after content scrolls past or after 45 s
4. **Spaceflight** — deterministic travel to each goal star with eased interpolation; crawl overlay fades out; `Space` advances to next star after arrival
5. **ThreeStars** — cinematic per-star showcase (camera shift → halo growth → message reveal → hold); auto-advances in Live mode
6. **Ending** — hyperspace jump (Tunnel or Particles mode: charge-up → jump → deceleration → white flash), then logo + summary reveal with star labels

### Video game mode

Skips the crawl and enters the 3D star-map directly.

Controls:

- Arrow keys → move in x/y
- `W` → move forward
- `S` → move backward

Features:

- bounded movement space
- configurable goal stars from `resources/stars.json`
- goal-star labels when close enough
- top-right transparent cube HUD with player position and goal markers
- numeric x/y/z coordinates below the cube

### Edit mode

Edits both runtime data files in a tabbed editor:

- `resources/text.txt`
- `resources/stars.json`

Both saved together with `Ctrl+S`.

## Data files

### text.txt

Section-based text format. Sections: `[intro]`, `[logo]`, `[title]`, `[subtitle]`, `[body]`.

### stars.json

Star configuration in JSON, consumed by both Live and Video game modes.

Schema:

```json
{
  "stars": [
    {
      "text": "Move fast. Don't run.",
      "position": { "x": -180, "y": 90, "z": 520 },
      "colors": { "core": "#e6f2ff", "glow": "#82beff" },
      "radius": 5.0
    }
  ]
}
```

Fields per star: `text`, `position` (x/y/z), `colors.core`, `colors.glow`, `radius`.

## Architecture

- `main.cpp` — app entry point
- `MainWindow.cpp/.h` — launcher, editor, mode switching, save/load
- `CrawlWindow.cpp/.h` — all presentation rendering and animation (phases, starfield, spaceflight, ThreeStars, Planet)
- `CrawlContent.h` — data structs (`CrawlContent`, `Star`, `StarDefinition`)
- `StarMapWidget.cpp/.h` — 3D cube star map viewer with projection and interaction
- `StarsEditorWidget.cpp/.h` — star list, property form (0–100% coordinates), StarMapWidget integration
- `TextIO.cpp/.h` — file I/O, section parser, JSON star parser/serializer

## Build

Requirements:

- Qt 6.7.3+ (MSVC 2022 on Windows, GCC/clang on Linux)
- CMake
- C++20 compiler
