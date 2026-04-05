# status.md

## Project context

This project is a Qt application used to present an internal talk about recent experience with AI.

The opening sequence is intentionally inspired by the classic **Star Wars** intro, but it is **not** meant to replicate it literally. It is an internal, non-public presentation for the team/company, and the current direction is to keep the tone as a **playful sci-fi parody / homage** while using our own project identity.

Current project title to use in the intro:

- **PROJECT RBI**

Current episode card to use:

- **EPISODE AI**
- **A NEW PROMPT**

## Current intro flow

The intro currently follows this narrative structure:

1. A humorous opening line, inspired by the classic "A long time ago..." idea.
2. A PROJECT RBI title reveal.
3. An episode card:
   - `EPISODE AI`
   - `A NEW PROMPT`
4. A perspective text crawl, in the style of a sci-fi opening crawl.
5. The crawl fades out into a small white point / star.

The crawl text already exists and is considered good enough for now. The next implementation step is **what happens after the crawl fade-out**.

## New target direction

We do **not** want to fade into a console or a planet immediately.

Instead, after the crawl fades away, we want to stay fully consistent with the space-travel / sci-fi visual language:

- The crawl fades out.
- A white point remains visible.
- That point is interpreted as a **star**.
- The camera moves toward that star.
- The star grows as the camera approaches it.
- Once near enough, the **first key message** is revealed.
- Then the presentation transitions back to a star field / space navigation feeling.
- The camera moves toward a second star.
- The second message is revealed.
- Then the same for a third star and third message.

So the presentation becomes a **journey through three stars**, each one representing one guiding message.

## Narrative intent

The intro is not just decoration. It should connect directly to the actual talk.

The three messages are not meant to feel like hard conclusions or bullet points from a corporate slide deck. They should feel like:

- orientation,
- guidance,
- three "guiding stars",
- three ideas that help us navigate AI.

This is important: the tone should remain **positive, realistic, reflective**, and slightly poetic, without becoming vague or sentimental.

## Core concept after the crawl

### Working framing

Suggested framing text before the star sequence:

- `Three guiding stars.`

This may be displayed briefly after the crawl, or omitted if the star sequence itself makes the concept obvious enough.

### Visual metaphor

Each message is associated with one star:

1. first star -> prudence / pace
2. second star -> creativity / possibility
3. third star -> people / purpose

The stars should feel like **destinations** or **navigation points**, not random decorative objects.

## The three messages

Current preferred wording:

1. **Move fast. Don't run.**
2. **Create more than before.**
3. **The purpose is people.**

These three are currently the preferred versions.

### Meaning of each message

#### 1) Move fast. Don't run.

Meaning:
- AI adoption is necessary and valuable.
- But we are often moving too fast.
- The message is not "stop".
- The message is: move with intent, judgment and steadiness.
- This idea comes from a previous experience in physical security, where the advice was: move quickly, but do not run, because running creates alarm, instability, and bad decisions.
- The same logic applies to AI adoption.

Tone goal:
- calm
- wise
- realistic
- not alarmist

#### 2) Create more than before.

Meaning:
- This very presentation/application is an example of something that was always desired but was never realistically achievable before.
- AI made it possible to turn an idea into something real.
- This applies not only to creative demos, but also to work: prototypes, tools, drafts, interfaces, experiments, automation, etc.
- The key idea is not only efficiency, but possibility.

Tone goal:
- inspiring
- concrete
- optimistic
- credible

#### 3) The purpose is people.

Meaning:
- The real reason for using AI is not technology for its own sake.
- The real motivation is helping colleagues, people, others.
- In the author's own words, the deepest reason is love: love for colleagues, for people, for others.
- In the presentation itself, this should likely be expressed in a professional but still human way.
- The message should land as: technology matters when it serves people.

Tone goal:
- human
- warm
- sincere
- not overly sentimental

## Visual direction for the three stars

### General rule

The visual transitions must be brief and elegant. The whole presentation has a hard time limit, so these star transitions must support the talk, not dominate it.

The opening "Star Wars"-style intro lasts about **1 minute**.

Each main message section has about **2 minutes maximum**.

The star travel transitions should therefore feel cinematic, but remain concise.

### Recommended approach

For each star:

1. show distant star in the field
2. subtly emphasize it
3. move camera toward it
4. enlarge star progressively
5. when close enough, reveal the corresponding text
6. hold the text cleanly and legibly
7. transition back to a wider field / navigation view
8. select next star

### Suggested pacing

Approximate per-star transition pacing:

- target star becomes distinct: ~0.5 s
- camera movement toward star: ~1.5 to 2.0 s
- message reveal: ~0.5 to 1.0 s
- hold long enough for speaking intro to the section

This should be refined experimentally in the UI.

## Suggested visual personality per star

These are suggestions, not strict requirements.

### Star 1 - Move fast. Don't run.

Possible visual feeling:
- cooler white / blue-white light
- initial motion feels slightly too fast, then stabilizes
- message appears when the movement settles

Symbolic meaning:
- speed without control becomes noise
- controlled speed becomes direction

### Star 2 - Create more than before.

Possible visual feeling:
- brighter, more energetic glow
- richer bloom or trail
- maybe some subtle generative particles or light traces

Symbolic meaning:
- one spark becomes many possibilities
- ideas become buildable things

### Star 3 - The purpose is people.

Possible visual feeling:
- warmer tone
- neighboring lights/stars become visible near it
- perhaps the single point leads visually into a small constellation or surrounding lights

Symbolic meaning:
- the point is not important by itself
- it matters because it connects to others

## Implementation goal for Codex

Codex should continue the current implementation from the end of the crawl and build the **three-star sequence**.

### Main feature to implement

After the crawl fade-out:

- preserve a visible star field
- keep or create a white point that reads as a star
- animate camera motion toward the first star
- reveal message 1
- transition back into navigation/star field space
- animate toward second star
- reveal message 2
- transition again
- animate toward third star
- reveal message 3

### High-level expectations

- Keep the sequence smooth and visually coherent with the existing intro.
- Avoid abrupt stylistic shifts.
- Do not jump to a terminal/consola or other UI metaphor at this stage.
- The "space travel through three stars" is now the preferred direction.
- Maintain readability of the message text.
- Favor simplicity and elegance over heavy visual complexity.

## Message display expectations

The messages should be easy to read and probably centered or near-centered at reveal time.

Current preferred text:

- `Move fast. Don't run.`
- `Create more than before.`
- `The purpose is people.`

If line breaking is needed, preserve readability and keep the wording intact if possible.

## Things explicitly not preferred right now

At this stage, we do **not** want:

- a fade directly into a console
- a realistic planet landing
- a hard switch to a business-slide feeling
- overlong transitions
- excessive visual effects that overshadow the messages

## Tone summary

The whole sequence should feel like:

- playful
- cinematic
- coherent with the intro
- reflective
- positive
- human

It should start as a parody/homage and naturally evolve into a meaningful internal presentation about AI.

## Short summary for immediate implementation

Implement the post-crawl sequence as a journey to **three stars**, each revealing one key message:

1. `Move fast. Don't run.`
2. `Create more than before.`
3. `The purpose is people.`

Keep it visually aligned with the existing sci-fi intro, concise in timing, and emotionally connected to the talk's real content.
