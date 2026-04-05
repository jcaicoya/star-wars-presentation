# StarWarsText

Qt/C++ presentation prototype inspired by the Star Wars opening crawl, but intentionally not a literal replica.

The project now has two main runtime modes plus an edit mode:

- `Live`
- `Video game`
- `Edit`

## Current app flow

On startup, the app shows an initial launcher screen with large selectors for:

- `Live mode`
- `Edit mode`
- `Video game mode`
- `Current size`
- `Full screen`

There is also a toolbar with shortcuts:

- `Ctrl+L` -> Live
- `Ctrl+E` -> Edit
- `Ctrl+G` -> Video game
- `Ctrl+1` -> Current size
- `Ctrl+2` -> Full screen
- `Ctrl+S` -> Save edited files
- `F11` -> Toggle fullscreen in the presentation window
- `Esc` -> Close presentation window and return to the main app

## Modes

### Live mode

Current intent:

- intro text
- logo
- crawl
- 3D star-map background visible from the beginning
- after the crawl, automatic travel to configured stars

Current reality:

- the first travel leg is close to the intended result
- later automatic travel legs are still incorrect
- the automatic travel logic still needs to be corrected before this mode can be considered stable

### Video game mode

This mode skips the crawl and enters the 3D star-map directly.

Current controls:

- arrow keys -> move in `x/y`
- `W` -> move forward
- `S` -> move backward

Current features:

- bounded movement space
- configurable goal stars loaded from `resources/stars.json`
- goal-star labels when close enough
- top-right transparent cube map
- cube map shows player position
- cube map also shows goal-star markers
- numeric `x/y/z` coordinates below the cube

### Edit mode

Edit mode now allows editing both runtime data files:

- [resources/text.txt](C:/Users/caico/Workspaces/C++/StarWarsText/resources/text.txt)
- [resources/stars.json](C:/Users/caico/Workspaces/C++/StarWarsText/resources/stars.json)

Both are presented as tabs in the editor UI and saved together with `Ctrl+S`.

## Data files

### text.txt

The presentation text is still stored in the section-based text format used by the current parser.

### stars.json

Star configuration is now externalized in JSON and used by both `Live` and `Video game` modes.

Current schema:

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

Editable per star:

- text
- position
- core color
- glow color
- radius

## Current technical direction

The project currently mixes two approaches:

- older scripted 2D / perspective-crawl code
- newer 3D-ish star-map navigation code used by the video-game mode and now partially reused by live mode

The newer star-map approach is the more promising one for the next iteration.

## Known issues

- automatic live-mode travel is still wrong after the first star
- live-mode pathing needs to stop correctly on each target
- star appearance still needs polish
- there is no proper final screen yet

## Build

Requirements:

- Qt 6.7.3 for MSVC 2022 in `C:/Qt/6.7.3/msvc2022_64`
- CMake
- MSVC with C++20 support

Main files:

- [MainWindow.cpp](C:/Users/caico/Workspaces/C++/StarWarsText/MainWindow.cpp)
- [CrawlWindow.cpp](C:/Users/caico/Workspaces/C++/StarWarsText/CrawlWindow.cpp)
- [TextIO.cpp](C:/Users/caico/Workspaces/C++/StarWarsText/TextIO.cpp)
- [resources/text.txt](C:/Users/caico/Workspaces/C++/StarWarsText/resources/text.txt)
- [resources/stars.json](C:/Users/caico/Workspaces/C++/StarWarsText/resources/stars.json)
