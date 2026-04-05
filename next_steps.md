# next_steps.md

## Purpose

This file is the handoff for the next session.

The codebase has progressed meaningfully, but the key presentation experience is still not correct enough to call finished.

The next session should focus on a smaller set of high-value fixes instead of adding more new features.

---

# 1. Immediate priority

Fix the **automatic travel in Live mode**.

This is now the main blocker.

Current situation:

- the first automatic leg can look good
- later legs degrade
- the issue is not solved by only moving the star positions in `stars.json`
- the problem is in how the camera/ship is moved from one goal star to the next

Required outcome:

- each automatic leg should end exactly on the intended target
- no overshoot
- no accumulated drift from previous legs
- predictable framing for star 2, star 3, star 4

---

# 2. Specific next tasks

## 2.1 Correct automatic traveling

Investigate and fix the live-mode pathing logic in [CrawlWindow.cpp](C:/Users/caico/Workspaces/C++/StarWarsText/CrawlWindow.cpp).

Important:

- the problem appears after the first leg
- the path should be deterministic
- the arrival should be exact
- pressing `Space` should continue cleanly to the next configured star

Questions to resolve:

- should live mode use deterministic interpolation per leg?
- should it use a look-at / target-framing model before translation?
- should arrival snap to a known camera pose relative to the target star?

Do not keep trying random numeric tweaks only. The model itself may need to be simplified.

## 2.2 Improve star look and feel

Once travel is correct, improve the visual quality of stars.

Current targets:

- distant stars should read as tiny points
- main stars should start small and grow convincingly
- depth ordering must remain correct
- overall style should stay "inspired by", not "copy of", Star Wars

Potential work:

- tune goal-star size formula
- tune glow radius and alpha
- tune background star density by depth bucket
- optionally add subtle depth-layer differentiation

## 2.3 Create the final screen

We need a proper ending screen after the final star.

For now, think of it as the **final screen / game over one**.

Requirements:

- clean
- intentional
- consistent with the presentation tone
- not just an abrupt stop

Do not reuse the previous planet ending by default.

## 2.4 Evaluate language / tool choice

We should explicitly assess whether continuing in the current stack is the best choice.

This does **not** mean immediate migration.

It means:

- evaluate if Qt Widgets + custom painter code is still efficient enough
- compare with alternatives better suited for 3D / camera / cinematic work
- consider whether another language/tool could accelerate iteration

Possible outputs for next session:

- stay with current stack and justify it
- identify a better option for a rewrite/prototype
- define a hybrid path if needed

---

# 3. Data model status

The star map is now externalized and editable through:

- [resources/stars.json](C:/Users/caico/Workspaces/C++/StarWarsText/resources/stars.json)

This is correct and should be preserved.

Both runtime modes should continue to consume this file instead of hardcoded star values.

If changes are made, protect this contract:

- `text`
- `position.x`
- `position.y`
- `position.z`
- `colors.core`
- `colors.glow`
- `radius`

---

# 4. What not to do next

Avoid these traps in the next session:

- do not keep adding new UI features before travel is fixed
- do not hardcode new star values in C++
- do not treat star-position tweaks as the main fix for broken travel
- do not bring back the planet ending as the default without reconsidering the design

---

# 5. Short brief for next session

The next session should do these four things:

1. correct automatic traveling in Live mode
2. improve star visuals
3. create the final screen
4. evaluate whether another language or tool would be better for this project
