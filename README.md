# StarWarsText

Qt/C++ presentation tool inspired by the Star Wars opening crawl. Features a complete cinematic pipeline (intro → logo → crawl → spaceflight → star showcase → planet finale) plus a free-flight video game mode and an integrated editor.

## App flow

On startup, the app shows a launcher screen with card selectors for:

- `Live mode` — full cinematic presentation
- `Edit mode` — text and star editor
- `Video game mode` — free-flight star-map exploration
- `Current size` / `Full screen` — display style

Toolbar shortcuts:

- `Ctrl+L` → Live
- `Ctrl+E` → Edit
- `Ctrl+G` → Video game
- `Ctrl+1` → Current size
- `Ctrl+2` → Full screen
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
6. **Planet** — camera pans, blue planet approaches, summary text fades in ("Three guiding stars" + all messages)

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
- `TextIO.cpp/.h` — file I/O, section parser, JSON star parser

## Build

Requirements:

- Qt 6.7.3 (MSVC 2022 kit)
- CMake
- C++20 compiler
