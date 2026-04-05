# status.md

## Current snapshot

The project is a fully functional Star Wars-inspired presentation tool with three modes, a launcher, an editor, and a complete cinematic pipeline from intro to finale.

All previously identified blockers (automatic traveling, star visuals, final screen) are resolved.

## What is implemented

### 1. Launcher / mode selection

The app starts on a launcher page with large card selectors for mode and display style, plus a toolbar with keyboard shortcuts.

### 2. Edit mode

Edits both data sources used by the presentation:

- `resources/text.txt` (section-based crawl content)
- `resources/stars.json` (star map configuration)

Both are presented as tabs and saved together with `Ctrl+S`.

### 3. Star-map data externalization

Goal stars loaded from JSON. Schema: `text`, `position` (x/y/z), `colors.core`, `colors.glow`, `radius`.

Currently 4 configured stars.

### 4. Video game mode

Free-flight 3D-ish navigation with bounded movement, projected starfield, goal stars with labels, cube HUD with markers, and coordinate readout.

### 5. Live mode — full cinematic pipeline

Live mode runs a complete sequence of 6 phases:

1. **Intro** — italic blue text, fade in/hold/fade out (~4.8 s)
2. **Logo** — large yellow text that shrinks exponentially and fades (~6.7 s)
3. **Crawl** — perspective-projected scrolling text with 3D starfield background, auto-transitions after content scrolls past or after 45 s timeout
4. **Spaceflight** — deterministic leg-based travel to each goal star using `easeInOutSine` interpolation; crawl overlay fades out during the first leg; Space advances to the next star after arrival
5. **ThreeStars** — cinematic per-star sequence with 6 sub-stages (Entry → Acquire → Travel → Reveal → Hold → Transition); camera shifts, star halo grows, message text fades in; auto-advances in Live mode
6. **Planet** — camera pans right, blue planet approaches with atmosphere glow, then reveals summary text ("Three guiding stars" + all star messages)

### 6. Automatic traveling (resolved)

The live-mode pathing system uses:

- leg-based interpolation: each leg has an explicit start, end, tick counter, and duration
- duration proportional to distance (`distance / 5.5f`, min 120 ticks)
- `easeInOutSine` easing for smooth acceleration/deceleration
- exact snap to target on arrival (`m_shipPosition = m_liveLegEnd`)
- no accumulated drift — each new leg starts from current position

### 7. Star visuals (resolved)

- goal stars rendered back-to-front (depth-sorted)
- radial gradient glow with distance-based emphasis
- projection-based scale
- background space stars with twinkle animation and spatial recycling
- ThreeStars phase adds cinematic halo growth, vignette, and neighbor satellites

### 8. Final screen (resolved)

Planet phase provides a clean ending:

- three-stage animation: pan (150 ticks) → approach (130 ticks) → text reveal (50 ticks)
- blue planet with layered ellipses and atmospheric glow
- summary text fades in with all star messages

## What is working well

- complete end-to-end Live mode presentation
- deterministic, drift-free automatic traveling
- externalized star configuration consumed by all modes
- launcher/editor structure
- contextual HUD hints per phase and mode
- video-game mode as a standalone exploration tool

## Remaining considerations

### 1. Technology risk

The project pushes Qt Widgets + custom QPainter rendering far beyond typical usage. This works, but iteration speed on camera, 3D, and shader-like effects is limited compared to a real 3D engine or GPU-accelerated framework.

### 2. Content polish

- star size/glow tuning is subjective and may need per-presentation adjustment
- ThreeStars timing constants are hardcoded; may benefit from externalization
- planet finale text is hardcoded; could be driven by `stars.json`

### 3. Minor issues

- `m_starMessages` constructor still has 3 hardcoded fallback entries (overwritten by `setGoalStars` at runtime, but dead code)
- duplicate `desired.normalize()` call in video-game tick (line 405)
- Windows-style paths in some doc links
