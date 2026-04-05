# status.md

## Current snapshot

This project is no longer just a crawl prototype.

It now contains:

- a launcher screen
- an editor mode
- a video-game-like 3D star-map mode
- a live presentation mode that attempts to reuse the same 3D star-map

The project is clearly moving toward a reusable "presentation world" rather than a one-off crawl animation.

## What is implemented

### 1. Launcher / mode selection

The app starts on a launcher page instead of jumping directly into show mode.

Available selectors:

- `Live mode`
- `Edit mode`
- `Video game mode`
- `Current size`
- `Full screen`

There is also a toolbar with mode/display shortcuts.

### 2. Edit mode

Edit mode can now edit both data sources used by the presentation:

- [resources/text.txt](C:/Users/caico/Workspaces/C++/StarWarsText/resources/text.txt)
- [resources/stars.json](C:/Users/caico/Workspaces/C++/StarWarsText/resources/stars.json)

This means the star map is no longer hardcoded in C++.

### 3. Star-map data externalization

The goal stars are now loaded from JSON.

This is the right direction because star configuration is structured data, not free text.

The file currently supports:

- `text`
- `position`
- `colors.core`
- `colors.glow`
- `radius`

### 4. Video game mode

This mode is the most coherent part of the project at the moment.

It already has:

- bounded 3D-ish movement
- projected star field
- goal stars
- cube HUD
- goal-star markers in the cube
- coordinate readout

This mode proves that the current "star map + ship position" model is viable.

### 5. Live mode reuse of the 3D engine

Live mode now starts from the same star-map world instead of switching to a disconnected post-crawl effect.

Current intent:

- intro
- logo
- crawl
- same mapped stars visible in the background
- fade from crawl into travel toward the first configured star

Conceptually, this is the correct direction.

## What is working well

- external star configuration through `stars.json`
- launcher/editor structure
- first leg of the live-mode travel is close to the intended feeling
- video-game navigation demonstrates the right rendering model
- initial deep-space look improved after increasing starfield density and shrinking stars

## What is not working yet

### 1. Automatic traveling in live mode

This is the main blocker.

Observed problem:

- the first star approach feels acceptable
- later approaches become incorrect
- the system either overshoots, reaches the wrong framing, or accumulates route errors

Current conclusion:

- this is a pathing / sequencing problem, not simply a star-position problem

### 2. Star look and feel

The visuals are improved, but still not finished.

Remaining issues:

- important stars can still feel too large or too synthetic depending on context
- star travel still needs more polish
- the overall cinematic quality is not finished yet

### 3. End state / final screen

There is still no proper final screen after the last star.

The old planet/final-state experiments are not the current answer.

We need a clean final screen, currently described informally as the "game over one".

### 4. Technology risk

The project is pushing Qt Widgets + custom 2D rendering quite far.

This does not mean the current approach is wrong, but it does mean we should explicitly evaluate whether another language or tool would accelerate iteration for:

- camera movement
- 3D feeling
- cinematic sequencing
- shader/effects work

## Current recommendation

Do not keep expanding features blindly until the live-mode automatic traveling is corrected.

The next session should focus on:

1. correct live-mode automatic travel
2. polish star visuals
3. design the final screen
4. evaluate whether the current stack is still the best one
