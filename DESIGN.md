# Design System — Maayavi

## Product Context
- **What this is:** Dice lane defense game with Indian mythology theming
- **Who it's for:** Casual gamers, Indian mythology enthusiasts, web game players
- **Space/industry:** Mobile/web tower defense (Plants vs Zombies, Kingdom Rush, Random Dice)
- **Project type:** SDL3/C game with Emscripten web build, iOS, desktop
- **Memory anchor:** "Indian mythology" — every design decision serves this memory

## Aesthetic Direction
- **Direction:** Dark Sacred — the darkness of a temple at night, not European gothic. The mood of Nataraja's cosmic dance, Kali's fire, Ravana's palace at midnight.
- **Decoration level:** Intentional — mandala-inspired geometric borders on UI panels, lotus motifs at section dividers, temple arch framing on menus
- **Mood:** Supernatural power, not supernatural horror. Stylish darkness. If Raji's warm paintings were viewed at midnight by lamplight.
- **Differentiation:** No competitor combines dark gothic atmosphere with Indian mythology. This is the "Hades for tower defense" positioning.

## Typography
- **Display/Hero:** Teko Bold (Indian Type Foundry, Google Fonts) — tall, condensed, geometric. Has the authority of a temple inscription carved in stone. Supports Devanagari natively. Use with letter-spacing 0.1-0.15em for commanding rhythm.
- **Body:** DM Sans — clean, neutral, good rendering at small sizes
- **UI/Labels:** DM Sans 0.75rem uppercase, letter-spacing 0.08-0.1em
- **Data/HUD:** DM Sans tabular-nums — consistent digit widths for score/wave/HP
- **Code:** JetBrains Mono
- **Loading:** Google Fonts CDN for web shell. In-game: SDL_RenderDebugText (bitmap) for now; upgrade path is stb_truetype or SDL_ttf
- **Scale:** 12px labels / 14px body / 16px UI / 20px subheads / 28px section / 48px hero / 72px splash

## Color
- **Approach:** Balanced — mythology as color language
- **Temple Gold:** #E5AE3A — saffron/sacred. Titles, scores, highlights, primary CTA
- **Vishnu Teal:** #78EFE5 — interactive elements, dice glow, accent, secondary
- **Kali Pink:** #CC4976 — damage, danger, active combat, destructive actions
- **Saffron:** #E8913A — HP, warmth, secondary gold
- **Temple Sanctum:** #1F182F — base dark background, the night sky
- **Panel Dark:** #2A3048 — UI panels, cards, overlays
- **Panel Mid:** #352E4A — hover states, elevated surfaces
- **Steel Blue:** #406287 — borders, structural elements, dividers
- **Neutral Warm:** #75718F — muted text, inactive elements
- **Neutral Deep:** #4A4660 — subtle borders, disabled states
- **Deepest Dark:** #140F20 — shadows, outlines (used as sprite outline color: rgb(31,24,47))
- **Text Cream:** #E2E8D6 — primary body text, like aged parchment
- **Text Muted:** #A09CAA — secondary text, captions
- **Semantic:** success #5A9E6F, warning #E5AE3A (gold), error #CC4976 (pink), info #78EFE5 (teal)
- **Dark mode:** N/A — the game IS dark mode

## Spacing
- **Base unit:** 4px (game runs at 400x700 virtual resolution)
- **Density:** Comfortable
- **Scale:** 2px / 4px / 8px / 12px / 16px / 24px / 32px / 48px / 64px

## Layout
- **Approach:** Game-native
- **Game screen:** HUD strip (40px top), battlefield (5 lanes center), dice tray (bottom)
- **Menu screens:** Centered panel over gothic backdrop, max-width 340px
- **Web shell:** Full-viewport canvas with dark background, centered
- **Border radius:** 0px for game UI (pixel-art aesthetic), 8-12px for web shell overlays only

## Motion
- **Approach:** Intentional
- **Spawn:** Squash/stretch overshoot (0.3s) — already implemented
- **Dice:** Tumble jitter easing out (0.65s) — already implemented
- **Interactive:** Pulse glow on ready dice (sinf wave, 4-5Hz)
- **Banner:** Fade in/out wave announcements (2s total)
- **Screen effects:** Flash on damage (0.25s), shake on hit (if enabled)
- **Easing:** ease-out for spawns/entrances, ease-in for exits/deaths

## Architecture Risks (design-driven)
These are the design changes that would most strengthen the "Indian mythology" memory:

1. **Temple architecture over gothic** — Replace European-gothic spires in draw_gothic_backdrop() with shikhara (curved temple tower) silhouettes, gopuram gateway shapes. render.c:310-344.
2. **Mandala-geometry UI panels** — Add corner ornaments and lotus border patterns to Nuklear panels. ui.c theme setup.
3. **Saffron/gold primary interactive color** — Current primary interactive is teal (#78EFE5). Consider making gold (#E5AE3A) the primary CTA color, teal as secondary accent.

## Existing Color Map (code reference)
These are the colors currently hardcoded in the C source:

| Purpose | Code Location | Current Value | Design Token |
|---------|--------------|---------------|--------------|
| Background | render.c:311 | rgb(44,25,56) | --base-dark variant |
| Panel/Window | ui.c:11 | rgb(31,24,47) | Temple Sanctum |
| Header | ui.c:12 | rgb(42,48,74) | Panel Dark |
| Border | ui.c:13 | rgb(64,98,135) | Steel Blue |
| Button | ui.c:14 | rgb(64,98,135) | Steel Blue |
| Button Hover | ui.c:15 | rgb(92,168,151) | Vishnu Teal (dimmed) |
| Button Active | ui.c:16 | rgb(204,73,118) | Kali Pink |
| Text | ui.c:10 | rgb(226,232,214) | Text Cream |
| Accent | render.c (various) | rgb(120,239,229) | Vishnu Teal |
| Gold | render.c (various) | rgb(229,174,58) | Temple Gold |
| Sprite Outline | sprites.c (all palettes) | rgb(31,24,47) | Deepest Dark |

## Decisions Log
| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-05-16 | Initial design system | Created by /design-consultation. Dark Sacred aesthetic chosen to differentiate from bright/cartoon competitors (Kingdom Rush, PvZ). Indian mythology as memory anchor. |
| 2026-05-16 | Teko display font | Chosen from 9 candidates across 3 rounds. Indian Type Foundry origin, Devanagari support, geometric authority. Paired with DM Sans body. |
| 2026-05-16 | Colors carry Indian identity | User preference: minimal/modern font, let gold/teal/mandala motifs be the cultural signal. Typography steps back, design carries culture. |
