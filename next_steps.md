# next_steps.md

## Purpose

Handoff for the next session. The main blockers (traveling, star visuals, final screen) are resolved. The presentation runs end-to-end.

---

# 1. Completed since last handoff

- ~~Fix automatic traveling in Live mode~~ — deterministic leg-based interpolation, exact arrival, no drift
- ~~Improve star visuals~~ — depth-sorted rendering, radial glow, projection scaling, twinkle
- ~~Create the final screen~~ — Planet phase with pan, approach, and summary text reveal
- **New: ThreeStars phase** — cinematic per-star sequence between Spaceflight and Planet

---

# 2. Next tasks

## 2.1 Evaluate language / tool choice

The only remaining item from the previous plan.

Qt Widgets + QPainter has carried the project this far, but the rendering is entirely CPU-side custom 2D. Assess whether:

- the current stack is sufficient for ongoing content iteration
- a GPU-accelerated framework (Qt Quick/QML with shaders, or a lightweight 3D engine) would meaningfully improve iteration speed on camera, effects, and cinematic polish
- a hybrid path is viable (e.g. keep the launcher/editor in Widgets, move the presentation to QML or a different renderer)

Output: a decision documented in this file or `status.md`.

## 2.2 Code cleanup

Minor issues found during review:

- `CrawlWindow` constructor (line 85–104) initializes 3 hardcoded `m_starMessages` entries that are immediately overwritten by `setGoalStars` — remove the dead initialization
- `tickSpaceflight` (line 405) has a duplicate `desired.normalize()` call — remove one
- Windows-style absolute paths in `README.md` links — replace with relative paths

## 2.3 Externalize hardcoded timing and text

Several values are embedded in C++ that could live in data files:

- ThreeStars stage durations (Entry 30, Acquire 170, Travel 118, Reveal 34, Hold 150, Transition 120 ticks)
- Planet phase durations (Pan 150, Travel 130, Reveal 50 ticks)
- Planet finale title ("Three guiding stars") and summary text — could be derived from `stars.json`
- Intro/Logo durations

This is optional polish, not a blocker.

## 2.4 Content tuning

- star size/glow parameters may need per-presentation adjustment
- crawl scroll speed and timeout (45 s) are fixed — consider making configurable
- 4th star ("Stay curious. Keep building.") only appears in Spaceflight, not in ThreeStars or Planet summary — verify this is intentional

---

# 3. Data model status

The star map is externalized in `resources/stars.json`. Both runtime modes consume this file.

Protected contract:

- `text`
- `position.x`, `position.y`, `position.z`
- `colors.core`, `colors.glow`
- `radius`

---

# 4. What not to do next

- do not hardcode new star values in C++
- do not add features before the stack decision is made
- do not break the `stars.json` contract without updating both modes and the editor
