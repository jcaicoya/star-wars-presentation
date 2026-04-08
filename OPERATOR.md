# Operator Guide

## Launcher

On startup, the app shows a launcher with card selectors:

| Row | Options | Default |
|-----|---------|---------|
| Mode | Live, Video game | Live |
| Display | Full screen, Current size | Full screen |
| Hyperspace | Tunnel, Particles | Tunnel |

Toolbar buttons open the editors or launch the presentation.

### Launcher shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+L` | Select Live mode |
| `Ctrl+G` | Select Video game mode |
| `Ctrl+1` | Full screen |
| `Ctrl+2` | Current size |
| `Ctrl+3` | Tunnel hyperspace |
| `Ctrl+4` | Particles hyperspace |
| `Ctrl+E` | Open text editor |
| `Ctrl+S` | Save edited files |

## Text editor

A structured form with fields for each section of `resources/text.txt`:

- **Left column:** Intro, Logo, Title (single line), Subtitle (single line)
- **Center column:** Body (multi-line with line numbers)
- **Right column:** Planet header, Final sentence

The body field shows a dashed vertical guide at the effective column limit. This limit is the longest body line length, clamped to [30, 40] characters. Text beyond this limit is not rendered in the crawl.

Body text is justified in the crawl. The last line of each paragraph is left-aligned.

## Stars editor

A split view: star list + property form on the left, 3D cube map on the right.

**Property form fields:** Text, Type (Star/Planet), Core color, Glow color, Radius, Position X/Y/Z (shown as 0-100%).

Stars can be dragged in the 3D cube map. Selection syncs between list and map.

Stars render as spheres. Planets render as spheres with Saturn-style rings.

## Show controls (presentation window)

### Live mode

Runs a 6-phase cinematic sequence: Intro, Logo, Crawl, Spaceflight, Hyperspace, Outro.

| Input | Action |
|-------|--------|
| Left click / `Right arrow` | Advance to next phase or star |
| Right click / `Left arrow` | Go back to previous phase |
| `Esc` | Close and return to launcher |
| `1` / `I` | Jump to Intro |
| `2` / `S` | Jump to Spaceflight |
| `3` / `H` | Jump to Hyperspace |
| `4` / `O` | Jump to Outro |

In the Outro, the first advance transitions from star recap to the final message. The second advance closes the show.

The spaceflight auto-triggers hyperspace ~0.5 seconds after reaching the last star.

### Video game mode

Enters the 3D starfield directly with free-flight controls.

| Input | Action |
|-------|--------|
| Arrow keys | Move left/right/up/down |
| `W` | Move forward |
| `S` | Move backward |
| `Esc` | Close and return to launcher |

Displays a transparent cube HUD (top-right) with player position and goal markers, plus numeric x/y/z coordinates.

## Data files

### resources/text.txt

Section-based text format. Each `[section]` header introduces a block that runs until the next header or EOF. Leading/trailing blank lines within each block are stripped.

Sections: `[intro]`, `[logo]`, `[title]`, `[subtitle]`, `[body]`, `[planetheader]`, `[finalsentence]`.

### resources/stars.json

JSON file defining the goal stars/planets.

```json
{
  "stars": [
    {
      "text": "Message text",
      "position": { "x": -180, "y": 90, "z": 520 },
      "colors": { "core": "#ffffff", "glow": "#99ccff" },
      "radius": 5.0,
      "type": "star"
    }
  ]
}
```

| Field | Description |
|-------|-------------|
| `text` | Label shown near the star during spaceflight and in the outro recap |
| `position` | 3D coordinates (X: -640..640, Y: -520..440, Z: -1000..2360) |
| `colors.core` | Hex color for the star's bright center |
| `colors.glow` | Hex color for the halo and outer gradient |
| `radius` | Visual size of the star |
| `type` | `"star"` (sphere) or `"planet"` (sphere + Saturn ring) |
