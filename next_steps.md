# next_steps.md

## Purpose

This file is intended as a practical handoff for Codex so it can continue implementation of the **post-crawl sequence** in the current Qt application.

It complements `status.md`, which explains the narrative and product context.

This file focuses on **implementation direction**, scene structure, animation flow, and design constraints.

---

# 1. Immediate goal

Implement the sequence that starts **after the crawl fades out**.

The new sequence should:

1. remain in the same visual universe as the intro
2. keep the star field visible
3. select a first star
4. move the camera toward it
5. reveal message 1
6. return to a broader star-navigation state
7. repeat for star 2
8. repeat for star 3
9. leave the app in a clean state for the next presentation section

Messages to reveal:

1. `Move fast. Don't run.`
2. `Create more than before.`
3. `The purpose is people.`

---

# 2. Preferred product behavior

This sequence should feel like a continuation of the intro, not a new mode.

That means:

- same visual language
- same rendering style
- same timing philosophy
- no abrupt switch to a terminal, planet landing, or corporate slide

The user wants the post-crawl sequence to feel like a **journey through three stars**.

---

# 3. Recommended implementation strategy

## 3.1 Treat the post-crawl as its own scene/state

Recommended approach:

Create a dedicated presentation state / scene for the post-crawl sequence, instead of trying to fold the whole behavior into the crawl logic itself.

For example:

- `IntroOpeningState`
- `ProjectTitleState`
- `EpisodeCardState`
- `CrawlState`
- `ThreeStarsState`
- `ConclusionState` or `NextSectionState`

Exact naming is flexible, but the key idea is:

> The three-star sequence should be implemented as a **separate, self-contained state**.

Advantages:

- easier to reason about
- easier to tune animation timing
- easier to debug
- easier to extend later

---

# 4. Suggested internal structure for ThreeStarsState

Recommended conceptual phases:

1. **Entry from crawl**
2. **Star field stabilization**
3. **Star 1 acquisition**
4. **Travel to star 1**
5. **Reveal message 1**
6. **Return / widen / transition**
7. **Star 2 acquisition**
8. **Travel to star 2**
9. **Reveal message 2**
10. **Return / widen / transition**
11. **Star 3 acquisition**
12. **Travel to star 3**
13. **Reveal message 3**
14. **Exit to next presentation section**

This can be implemented using either:

- a small explicit finite state machine
- a timeline/sequence animation controller
- a chained signal/slot animation sequence
- a custom update loop with time-based sub-phases

Preferred choice:
- whatever best matches the current app architecture

If current code already has a phase-based animation model, reuse it.

---

# 5. Suggested rendering model

## 5.1 Keep the star field alive

Do not let the background become empty black after the crawl.

The crawl should fade away, but a sparse star field should remain.

At minimum, maintain:

- static or subtly animated distant stars
- one candidate target star
- optional depth/parallax feeling

This is important because the three-star journey depends on continuity.

## 5.2 The first white point should become the first star

If the current implementation already ends the crawl with a small white point, reuse it as the first star when possible.

That is narratively elegant and visually efficient.

Possible flow:

- crawl opacity goes to zero
- white point remains
- surrounding stars slowly reassert visibility
- that point becomes the active target
- camera starts moving toward it

---

# 6. Camera / motion model

## 6.1 General rule

The camera should feel like it is **moving through space toward a destination**.

This does not require a full 3D engine.

A convincing effect can likely be created with 2D transforms and layered stars:

- scale
- translation
- radial growth
- brightness changes
- parallax layers

## 6.2 Simple implementation option

If the app is mostly 2D, a good illusion may be achieved by:

- designating one star as target
- keeping it near the center or slightly off-center
- scaling the target star up over time
- moving background stars outward relative to the target
- increasing target brightness / bloom
- optionally reducing visibility of non-target stars as the target dominates

This would simulate forward travel.

## 6.3 Alternative implementation option

If current code already has some concept of 3D/perspective or virtual camera distance, the target star could be treated as a point the camera approaches, while background stars respond accordingly.

But do not overengineer unless the current architecture already supports it.

---

# 7. Recommended per-star animation flow

For each of the three stars:

## Phase A - Acquisition
- star field visible
- one star becomes subtly brighter / more prominent
- this should tell the viewer: this is the next destination

Duration guideline:
- ~0.3 to 0.7 s

## Phase B - Travel
- camera advances toward target star
- target grows
- surrounding field shifts to suggest movement
- keep motion smooth and controlled

Duration guideline:
- ~1.5 to 2.0 s

## Phase C - Arrival / settle
- movement eases out
- target is now large/near
- scene stabilizes briefly before text appears

Duration guideline:
- ~0.2 to 0.5 s

## Phase D - Message reveal
- reveal the corresponding message
- simple fade-in is fine
- readability is more important than effect complexity

Duration guideline:
- ~0.4 to 1.0 s

## Phase E - Hold
- hold message cleanly while speaker begins the section
- keep background alive but calm

Duration guideline:
- enough for the presentation rhythm; may need to wait for user input or a timed step depending on current app design

## Phase F - Transition out
- hide message
- reduce target dominance
- return to wider star field or begin transition to next target

Duration guideline:
- ~0.5 to 1.0 s

---

# 8. Message presentation recommendations

## 8.1 Preferred wording

Use exactly:

1. `Move fast. Don't run.`
2. `Create more than before.`
3. `The purpose is people.`

## 8.2 Text layout

Preferred:
- centered or near-centered
- large enough to read instantly
- stable, no fancy motion during reading
- avoid perspective distortion on these lines

This is important:
The crawl is perspective text.
The key messages should **not** be perspective text.
They should feel like clear, present, readable statements.

## 8.3 Suggested reveal style

Any of these are acceptable:

- opacity fade-in
- slight scale + fade-in
- glow fade resolving into crisp text

Preferred choice:
- keep it simple and elegant

Avoid:
- typewriter effect
- overly technical UI overlay
- aggressive glitch effects

---

# 9. Visual personality per message

These are optional enhancements. Good if easy to implement. Not required if they add too much complexity.

## 9.1 Star 1 - Move fast. Don't run.

Possible cues:
- slightly faster initial movement
- then a controlled ease-out
- cooler white or blue-white tone

Goal:
- visually support the idea of speed becoming steadiness

## 9.2 Star 2 - Create more than before.

Possible cues:
- richer bloom
- subtle particle traces
- stronger glow or generative feeling

Goal:
- suggest possibility, expansion, making

## 9.3 Star 3 - The purpose is people.

Possible cues:
- warmer light
- nearby faint stars become visible
- subtle constellation-like feeling

Goal:
- suggest connection, relation, others

Again: these are optional. The basic sequence matters more than extra polish.

---

# 10. Timing constraints

The presentation has strict time limits.

Known constraints:

- the opening Star-Wars-inspired intro lasts about **1 minute**
- each main idea should take roughly **2 minutes maximum**
- there will be a conclusion after the three main ideas

So the transitions must be concise.

Recommendation:
- do not let this three-star sequence become a second long intro
- keep animation beautiful but efficient

---

# 11. Input / control behavior

Codex should inspect the current app flow and choose the control model that best fits.

Possible models:

## Option A - Timed autoplay
Each star sequence progresses automatically after defined durations.

Pros:
- cinematic
Cons:
- less speaker control

## Option B - Step/advance controlled
Each message holds until presenter input advances to the next star.

Pros:
- better for live presenting
Cons:
- slightly more implementation complexity

## Option C - Hybrid (recommended if feasible)
- acquisition/travel/reveal are animated automatically
- once the message is visible, wait for presenter action to continue

This is probably the best presentation behavior.

If there is already a "next" interaction in the app, reuse it.

---

# 12. Exit behavior after third message

After the third message, the app should transition into whatever comes next in the presentation.

This part is not yet fully defined, so implementation should ideally leave a clean hook.

Recommended design:
- after message 3 hold, emit a signal / set a state / call a transition callback for next section
- do not hardcode a console or unrelated next scene unless it already exists in the app and is explicitly intended

A flexible handoff is preferred.

For example:
- `threeStarsFinished()`
- `requestAdvanceToNextSection()`
- or similar

---

# 13. Maintainability expectations

Codex should prefer code that is:

- easy to tune
- easy to read
- parameterized where useful

Useful parameters to expose:

- star travel duration
- message reveal duration
- background star count
- target star brightness multiplier
- hold duration (if timed)
- text font size / color / glow
- per-star visual style parameters

Even if not all are surfaced in UI, keep them centralized in code if possible.

---

# 14. Minimum viable implementation

If time is limited, implement this in the simplest acceptable form:

1. after crawl fade, show star field
2. select star 1
3. animate scale/approach illusion
4. fade in message 1
5. on advance, reset to wider field
6. repeat for stars 2 and 3
7. expose clean completion point

This is enough to satisfy current direction.

Extra polish can come later.

---

# 15. Nice-to-have polish ideas

Optional only:

- subtle bloom around target stars
- star twinkle shader/effect
- very light parallax depth layers
- message glow matching star tone
- different star sizes/positions to avoid repetition
- slight drift before each acquisition
- tiny easing differences per star

Do not block core implementation on these.

---

# 16. Things to avoid

Please avoid the following unless explicitly requested later:

- immediate switch to terminal/consola scene
- realistic planet approach sequence
- overly long dead time between stars
- excessive particle noise
- unreadable text due to bloom or motion
- business-slide style overlays
- heavy stylistic break from the intro

---

# 17. Immediate TODO list for Codex

1. Inspect current state/scene architecture.
2. Locate end-of-crawl transition point.
3. Add or extract a dedicated `ThreeStarsState` (or equivalent).
4. Reuse the final white point from the crawl as first target star if feasible.
5. Implement star field continuity after crawl fade-out.
6. Implement star acquisition + travel + message reveal for star 1.
7. Generalize the sequence for star 2 and star 3.
8. Support presenter-controlled advance if practical.
9. Add clean handoff after the third message.
10. Keep timing configurable.

---

# 18. Final short brief

The desired sequence after the crawl is:

- stay in space
- travel to star 1 -> reveal `Move fast. Don't run.`
- travel to star 2 -> reveal `Create more than before.`
- travel to star 3 -> reveal `The purpose is people.`

This should feel like **three guiding stars**, fully connected to the intro's sci-fi journey.
