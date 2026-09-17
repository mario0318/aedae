# House of Human Capability: deterministic implementation brief

## Status and authority

**PROPOSED. Not an approved design-system replacement.** This document is a handoff specification for the next approved public-site redesign. It does not amend `01-information-architecture.md`, `02-design-system.md`, or the claim registry until the human owner explicitly approves it.

When approved, it supersedes the visual rules in `02-design-system.md` for the public site only. It does not alter the site's claim, privacy, static-build, or deployment constraints.

## Non-negotiable outcome

Build aeDae as a complete editorial destination called **House of Human Capability**. It must feel like a cultured public institution and journal about practical human agency, not a startup landing page, a security dashboard, a technical status page, or a developer-docs site.

The authenticator is one concrete project inside the House. Its unshipped state is visible only in the technical-project module and on `aedae.tech`; it must never consume the hero or become the site’s main visual motif.

## Agent execution rules

1. Implement only the values, sections, text, colors, spacing, and layout rules written here. Do not add a visual motif, a color, a font, an icon set, an illustration, a section, or a call to action not specified below.
2. Do not use image generation, stock photography, external assets, icon libraries, web fonts, gradients, glass effects, parallax, carousels, animated counters, canvas, video, or JavaScript.
3. Use semantic HTML and CSS only. The existing `node build.mjs` must remain dependency-free and must generate exactly 15 pages across the existing five surfaces.
4. Preserve the current claim-status system exactly: `verified`, `specified`, `research`, `horizon`, and `practice`; each rendered status remains paired with its existing source. Never render `built`.
5. Preserve the existing public-site guarantees: no forms, account creation, downloads, schedules, cookies, analytics, local storage, third-party embeds, outbound network calls, DNS changes, or new deployment configuration.
6. If a requirement is absent or conflicts with a repository source of truth, stop and report the gap. Do not make a design decision.

## Brand model

The House has five named rooms. These map directly to the existing surfaces and are not new brands:

| Room | Surface | Role | Fixed label |
|---|---|---|---|
| Workshop | `aedae.tech` | concrete tools and technical work | `WORKSHOP / tools made accountable` |
| Observatory | `aedae.world` | arguments and research horizon | `OBSERVATORY / ideas under a wider sky` |
| Commons | `aedae.life` | practical capability and care | `COMMONS / practices kept close` |
| Journal | `aedae.live` | field notes and honest work in motion | `JOURNAL / notes from the work` |
| Foyer | `aedae.online` | entry point and directory | `FOYER / find your way in` |

The master wordmark is exactly `æDæ` in lower case. Do not add a slogan to the wordmark. The primary statement is exactly:

> A house for practical human capability.

The supporting sentence is exactly:

> Tools, ideas, and practices for keeping more of your life in your own hands.

These are positioning statements, not claims about an implemented product. They must not receive a status chip.

## Global visual tokens

Use only this token set. Values are fixed.

```css
:root {
  --paper: #F3F0E8;
  --paper-deep: #E6E0D3;
  --ink: #1B1B19;
  --ink-soft: #4B4B45;
  --line: #B8B1A3;
  --night: #1D2925;
  --night-line: #69736C;
  --cream: #FAF7F0;
  --workshop: #A04932;
  --observatory: #566DA2;
  --commons: #54704D;
  --journal: #9E6A3D;
  --foyer: #765877;
  --focus: #184D8E;
  --sans: Georgia, "Times New Roman", serif;
  --mono: "Cascadia Mono", "Cascadia Code", Consolas, monospace;
}
```

Use `--sans` only for display headings, section headings, pull quotes, and feature-link titles. Use `--mono` for body text, navigation, labels, sources, metadata, chips, and buttons. Never introduce a third font family.

There are no gradients. Paper texture is one CSS background layer only:

```css
background-image: radial-gradient(rgba(27,27,25,.07) .7px, transparent .7px);
background-size: 7px 7px;
```

This is the sole approved gradient syntax and must be used only on the outer page background.

## Master layout

### Desktop: 1440 px reference viewport

- Page background: `--paper`; texture applied to `body`.
- Content frame: maximum width `1280px`, centered, `24px` left/right page margin.
- Frame internal border: `1px solid var(--line)` on left and right only.
- Column grid: 12 columns, `72px` each, `16px` gutter. The usable frame width is `1056px`; remaining frame width is blank breathing room, not an extra column.
- Every major section starts on a `1px` horizontal rule and has `72px` top and bottom padding.
- Never round a card, chip, image frame, or button. Border radius is `0` everywhere.
- Do not use shadows.

### Tablet: 768–1099 px

- Content frame margin `20px`; internal side padding `28px`.
- Preserve section order. Change all 12-column constructs to a 6-column grid with `16px` gutters.
- Two-column modules become 3/3 columns. Five-room cards become 3 + 2 cards, in source order.

### Mobile: 320–767 px

- Page margin `12px`; internal side padding `20px`.
- One column, no horizontal scrolling.
- Header navigation collapses to one visible row of five abbreviated room links: `TECH`, `WORLD`, `LIFE`, `LIVE`, `ONLINE`. It wraps to two lines only below 390 px.
- All decorative room-map lines remain, but room labels and descriptions stack beneath their marks.
- Keep body copy at 16 px minimum and all interactive targets at least 44 by 44 px.

## Header and footer

### Header

Header height is `88px` desktop, `72px` mobile. It contains exactly three regions:

1. Left: wordmark `æDæ`, `40px` desktop / `32px` mobile, `--sans`, linked to the current surface root.
2. Center: fixed phrase `HOUSE OF HUMAN CAPABILITY`, uppercase `11px`, `0.14em` letter spacing, `--mono`. Hide this region below 768 px.
3. Right: the five room links in a single row. Uppercase 11 px. Current room uses its room accent as a 3 px bottom rule; no filled tab, pill, or hover animation.

Header has a bottom rule only. It is not sticky.

### Footer

Footer uses `--night` background and `--cream` text. It contains three fixed columns desktop, stacked mobile:

- `æDæ` plus the fixed statement `A house for practical human capability.`
- Five room links, one per line, in the table order above.
- Existing privacy statement verbatim: `This site sets no cookies, uses no analytics, and has no third-party embeds.`

Footer padding: `48px` desktop, `32px 20px` mobile. No social icons, email sign-up, copyright-year interpolation, or contact CTA.

## Homepage: exact module order

Every surface uses the same component sequence. Only the room label, accent color, existing title, lead, facts, links, and status sources change. This resolves cohesion without adding unverified content.

### 1. Hero: `HOUSE / ROOM`

- Height: `min(720px, calc(100vh - 88px))`, never below `600px` desktop; content-led on mobile.
- Layout: left text block 7 columns; right House Map 5 columns.
- Above heading: fixed room label from the table, 11 px uppercase in the room accent.
- H1: existing surface title, `clamp(56px, 6.2vw, 92px)`, serif, 0.95 line-height, maximum 4 lines.
- Lead: existing surface lead, 18 px / 1.55, max width 34ch.
- Below lead: one text link, `ENTER THE [ROOM NAME]`, pointing to the first existing continuation link for that surface. No button treatment.

#### House Map

Build in HTML/CSS, no SVG or image. It is a `420px × 420px` square desktop, `min(100%, 340px)` mobile. It has:

- An outer square 1 px `--ink` border.
- A central 112 px square, 1 px `--ink` border, containing `æDæ` at 48 px serif.
- Five fixed room markers: small 12 px filled circles with 1 px outlines, connected to the central square by 1 px lines.
- Marker positions relative to square: Workshop `(18%, 23%)`; Observatory `(76%, 17%)`; Commons `(83%, 67%)`; Journal `(46%, 84%)`; Foyer `(17%, 70%)`.
- Only the current room marker is filled with the room accent. Other markers are `--paper` fill with `--ink` outline.
- Each marker has a fixed room name in 10 px uppercase mono, placed 12 px toward the nearest outer edge.
- Add no animation and no hover behavior.

### 2. House statement band

- Full frame width, `--night` background, `--cream` text; 72 px vertical padding.
- First line exactly: `CAPABILITY IS NOT A LUXURY.`
- Second line exactly: `It is the ability to understand, choose, recover, make, and remain your own.`
- First line: 12 px uppercase mono, letter spacing .12em. Second: 34 px serif, max width 26ch.
- No status chips in this band.

### 3. Five rooms

- Heading exactly `FIVE ROOMS, ONE HOUSE.`
- Use a horizontal grid of five equal cards on desktop, 3 + 2 tablet, one-column mobile.
- Each card height 300 px desktop. Top 72% is a CSS geometry panel; bottom 28% contains fixed room label and role from the table.
- Geometry panels are exact: Workshop = 3 offset vertical bars; Observatory = 3 concentric circles plus one orbit line; Commons = 5 rounded? **No** rounded geometry: use 5 overlapping 48 px squares; Journal = one 1 px vertical line with five short horizontal ticks; Foyer = one open 96 px square with a 16 px square at the center.
- Current surface card has its accent as the geometry stroke/fill. Others use `--ink` strokes only. Cards link to their respective domain roots.
- Card borders: right only between cards; no surrounding card boxes.

### 4. Current surface feature

- Heading: existing surface title.
- Two columns: copy 5 columns; three existing facts 7 columns.
- Copy side: existing lead, then its exact existing state chip and source.
- Fact side: render exactly three existing facts. Each fact has its existing heading, body, status chip, and source. Separate facts with horizontal rules. Do not invent an editorial headline or a fourth card.

### 5. The record

- Background `--paper-deep`.
- Heading exactly `THE RECORD STAYS OPEN.`
- Two fixed paragraphs:
  1. `æDæ records what is true now, what is only specified, what remains under study, and what belongs to practice.`
  2. `A page is allowed to have limits. A claim is allowed to be unfinished. The work is to make both legible.`
- To the right, show the five approved status labels in a vertical list, each using the existing chip style and no source line. This is taxonomy explanation, not a factual claim.

### 6. Continue

- Heading exactly `KEEP WALKING.`
- Render only the current surface’s existing continuation links.
- Each link is a full-width 1 px top rule, label on the left, fixed arrow `→` on the right. Height 68 px. No cards or buttons.

## Non-homepage routes

Existing non-homepage routes retain their current sourced copy. Give each this fixed page template:

1. Compact title block: room label, page title, existing lead or first paragraph.
2. One 1 px rule.
3. Main content at max 720 px width.
4. A right-side `HOUSE MAP / YOU ARE HERE` marker on desktop only; hide it below 768 px.
5. Continue module using the exact existing continuation links.

No route gains copy, claims, statistics, testimonials, pricing, product UI, feature comparisons, or calls to action beyond its current source content.

## Status-chip specification

- Typeface: `--mono`, 10 px, uppercase, `0.08em` letter spacing.
- Border: `1px solid currentColor`; padding `4px 7px`; no radius.
- Colors: `verified #2D6648`, `specified #8B572A`, `research #566DA2`, `horizon #765877`, `practice #54704D`.
- Source line directly beneath: 11 px mono, `--ink-soft`, 8 px top margin.
- Chip order is always chip then source. Do not place a chip alone except in the fixed taxonomy list in “The record.”

## Accessibility and interaction

- Use a visible `Skip to content` link.
- All navigation is ordinary anchor navigation; no JavaScript menus.
- Every focus state: `3px solid var(--focus)` with 3 px offset.
- Keep text contrast at WCAG AA or greater. Do not use accent color for body text on `--paper` if it fails contrast.
- House Map is decorative when it is not the current-page indicator: use `aria-hidden="true"`. On a current surface, give it `role="img"` and the exact label `House map. Current room: [Room Name].`
- Honor `prefers-reduced-motion` by having no motion to disable.

## Files, ownership, and completion

Implementation scope is restricted to:

- `web/src/site.mjs`
- `web/src/styles.css`
- `web/spec/02-design-system.md` only after human approval of this brief
- `web/spec/07-house-of-human-capability-implementation-brief.md`

Do not modify `web/build.mjs`, deployment files, repo-root specifications, authenticator code, tests, packaging, or claims unless a separate task authorizes it.

### Required verification

1. Run `node build.mjs` in `web/`; it must report `Built 15 pages across 5 surfaces`.
2. Run `git diff --check`.
3. Confirm generated HTML contains no `<script`, `data-status="built"`, prohibited handoff citation, external font URL, `http://` or `https://` asset URL, `localStorage`, `cookie`, or analytics term.
4. Capture screenshots at 1440 × 1100, 1024 × 1100, and 390 × 844 for `tech`, `world`, and `online` roots. Compare against every fixed rule in this brief; no subjective substitutions are allowed.
5. Report any rule that cannot be implemented without a decision. Do not mark complete until that rule is resolved by the human owner.
