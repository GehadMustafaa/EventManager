# Modern Software Design Philosophies: Agent Reference

Compiled 2026-11-17 from published material by teams that ship mainstream products: Linear, Vercel (Geist), GitHub (Primer), Atlassian, Stripe, Radix, Apple (Liquid Glass), Google (Material 3 Expressive), Figma (UI3).

Purpose: give a coding or design agent concrete, checkable rules taken from real, shipping systems, so output does not default to generic "AI UI".

## How to read this file

Every claim carries an evidence tag. Trust them in this order.

| Tag | Meaning |
|---|---|
| `[P]` | Primary: read directly from the publisher's own site or docs |
| `[R]` | Republished: the publisher's spec, but read via a third-party copy (e.g. a dev.to repost of Vercel's design.md) |
| `[S]` | Secondary: third-party summary or package that mirrors the spec; values are likely right, verify before hard-coding |
| `[M]` | From the compiler's memory, not retrieved this session. Verify before relying on it |
| `[X]` | Synthesis by the compiler (a judgement drawn across sources, not a quote from any one) |

Where a number appears without a tag, it carries the tag of the section's source line.

---

## 0. The ten rules (read these if nothing else)

1. **Rank attention; don't spread it evenly.** Navigation and chrome recede, the work surface leads. (Linear, Apple, Figma) `[P]`
2. **Structure should be felt, not seen.** Prefer spacing, tone shifts and alignment over borders and dividers; when you use borders, soften them. (Linear 2026) `[P]`
3. **Colour is a system of roles, not a palette.** Pick from named steps (background, hover, border, solid, text) rather than picking hex values per component. (Radix, Geist, Primer, Atlassian) `[P][R]`
4. **Generate colour in a perceptual space (OKLCH / LCH / Lab), never by nudging HSL lightness.** Equal lightness then looks equally light across hues, and contrast becomes predictable. (Stripe, Linear) `[P]`
5. **Few inputs, many outputs.** Linear collapsed 98 per-theme variables into three: base, accent, contrast. Build themes by derivation. `[P]`
6. **One spacing grid, named tokens, deliberate optical exceptions.** 4px (Geist, Primer) or 8px with fractional steps (Atlassian). `[P][R]`
7. **Type is tokenized as complete styles** (size + line-height + weight + tracking together), and tracking tightens as size grows. (Geist, Primer, Atlassian) `[R][P]`
8. **Motion answers an action.** Most UI interactions should be near-instant; motion is for showing what changed. (Geist, Apple, M3 for spatial) `[R][P]`
9. **Accessibility is a design input, not an audit.** Contrast, focus rings, target size and reduced-motion/transparency are specified up front. `[P]`
10. **Prune continuously.** Products drift toward clutter one useful feature at a time; scheduled subtraction is part of the design process. (Linear 2026) `[P]`

---

## 1. Task router

| If the task is... | Go to |
|---|---|
| Building a dense productivity / dev-tool / dashboard UI | §3.1 Linear, §3.2 Geist, §3.3 Primer, §3.4 Atlassian |
| Defining a colour system or theme (light/dark/custom/contrast) | §3.5 Stripe, §3.6 Radix, §3.1 Linear (theme generation) |
| Building a native-feeling iOS/macOS UI | §3.7 Apple |
| Building an Android / Material UI, or want physics-based motion | §3.8 Material 3 Expressive |
| Designing a canvas-centric / tool-heavy editor | §3.9 Figma |
| Writing UI copy (buttons, errors, toasts, empty states) | §4.9 |
| Need exact numbers (spacing, type, radii, shadows, timing) | §4 and §7 |
| Reviewing existing UI for quality | §6 and §8 |

---

## 2. Where mainstream systems converge

These show up independently in several shipping systems, so treat them as strong defaults.

| Principle | Who does it | Practical effect |
|---|---|---|
| Content over chrome | Linear, Figma, Apple, Primer | Controls are quieter than content; panels collapse; navigation dims once you've arrived |
| Semantic, role-named tokens | Primer, Atlassian, Geist, Radix | Components reference `color.text.subtle`, never `#6b7280` |
| Three token layers: raw, semantic, component | Primer `[P]` | Raw values never used directly in UI code |
| Perceptually uniform colour | Stripe, Linear, Radix-style scales | Accessible contrast without ugly, muddy colours |
| Tight, restrained shape language | Geist (6/12/16 px), Atlassian (2/4/6 px) | One radius family per view |
| Tone before shadow for depth | Geist, Atlassian, M3 | Elevation by surface tone + subtle layered shadow, not heavy drop shadows |
| Grid-snapped everything | Geist 4px, Primer 4px, Atlassian 8px | Line-heights and sizes land on the grid |
| Respect system accessibility settings | Apple, Linear, Geist, M3 | Increase Contrast, Reduce Motion, Reduce Transparency honoured |
| Opinionated defaults, limited variants | Linear, Geist (max two weights per view) | Fewer, better choices |

---

## 3. System profiles

### 3.1 Linear (issue tracking / product development)

**Philosophy.** Calm, dense, fast, opinionated. The interface is designed so users look at their work, not the tool. Sources: 2024 redesign write-up, 2025 mobile post, March 2026 refresh post. `[P]`

**Principles stated by Linear (2026 refresh):**
- *Don't compete for attention you haven't earned.* Parts that orient and navigate must recede; the task area leads. Concretely: sidebar made a few notches dimmer, tabs made compact (rounded, smaller icons and text, some icon-only), fewer icons overall and smaller, coloured backgrounds behind team icons removed. `[P]`
- *Structure should be felt, not seen.* Borders and separators had multiplied without reason; they were rounded and made lower-contrast. `[P]`

**Colour / theme system (2024 redesign):** `[P]`
- Theme generation uses the LCH colour space rather than HSL, because equal lightness values look equally light across hues.
- Each theme is defined by **three variables: base colour, accent colour, contrast.** This replaced 98 hand-defined variables per theme. The same system now produces the default light and dark themes and user custom themes.
- The contrast variable lets the system emit very-high-contrast themes automatically for accessibility.
- Elevation (background, foreground, panels, dialogs, modals) is derived from the same generation rules.
- To feel more neutral and timeless they limited how much of the chrome hue (blue) feeds into the calculations, and made text and neutral icons darker in light mode and lighter in dark mode.
- 2026: default neutrals moved from a cool, bluish grey to a slightly warmer, less saturated grey; they warn that going too warm turns muddy. They built an internal tool exposing hue/chroma/lightness per token to tune this quickly. `[P]`

**Typography:** Inter Display for headings (more expression), regular Inter for everything else. `[P]`

**Layout / chrome:** An inverted-L global chrome (sidebar + header) controls the main view. They aligned labels, icons and buttons optically across sidebar and tabs; the effect is felt rather than noticed. Navigation (back/forward/history/tabs) is designed to be removable so the same UI works in a browser and in a desktop shell. `[P]`

**Mobile (2025):** They rebuilt the nav with a custom frosted material inspired by Apple's Liquid Glass, but kept control of the material to support customizable navigation. They deliberately did not reproduce refraction, and the material mirrors Increase Contrast behaviour for accessibility. `[P]`

**Process lessons:**
- Do redesigns fast (about six weeks) to avoid blocking other work and accumulating design debt. `[P]`
- Ship behind a feature flag, test internally with a dev-toolbar toggle to compare old/new in one click, then roll out to a percentage of workspaces daily. `[P]`
- Test a direction against every major view before committing (stress tests), organized by view type (list, board, split). `[P]`

**Use when:** dense work tools, keyboard-heavy apps, anything where users stay in the UI for hours.
**Avoid when:** consumer/marketing surfaces needing expressive brand moments.

---

### 3.2 Vercel Geist (developer platform)

**Philosophy.** Minimal, high-contrast, content on near-neutral surfaces. Colour signals state or hierarchy, never decoration. Hierarchy comes from tonal surfaces and borders first, shadows stay subtle. `[R]` (Vercel's published design.md, dark theme, read via a repost; Vercel's own docs state each page is also available as Markdown by appending `.md`. `[P]` Light-theme values were **not** retrieved.)

**Colour scale semantics (the most reusable idea).** Each non-background hue has 10 steps (100–1000), and the step number encodes the role: `[R]`
| Step | Role |
|---|---|
| 100 / 200 / 300 | Component background: default / hover / active |
| 400 / 500 / 600 | Border: default / hover / active |
| 700 / 800 | Solid fill (high contrast) / its hover |
| 900 | Secondary text and icons |
| 1000 | Primary text and icons |

- Two background tokens: `background-100` (primary page + card surface) and `background-200` (subtle separation only; not a general fill).
- Translucent `gray-alpha-*` for borders, dividers, overlays and hover (layers over any background); solid `gray-*` for text and opaque fills. Don't swap gray and background scales.
- Accent hues carry meaning: blue = links / focus / success-ish, red = error, amber = warning, plus green, teal, purple, pink. Every accent has an `oklch()` P3 variant with an sRGB fallback.
- Dark theme anchors (reference only): page `#000000`, gray-1000 text `#ededed`, gray-900 secondary `#a0a0a0`, gray-100 `#1a1a1a`, gray-400 border `#2e2e2e`, blue-700 `#006efe`, red-800 `#e2162a`.

**Type scale (Geist Sans for UI/prose, Geist Mono for code/data/tabular figures):** `[R]`
| Style | Size / line-height | Weight | Tracking |
|---|---|---|---|
| heading-72 | 72 / 72 | 600 | −4.32px (−0.06em) |
| heading-48 | 48 / 56 | 600 | −2.88px (−0.06em) |
| heading-32 | 32 / 40 | 600 | −1.28px (−0.04em) |
| heading-24 | 24 / 32 | 600 | −0.96px (−0.04em) |
| heading-20 | 20 / 26 | 600 | −0.4px (−0.02em) |
| heading-16 | 16 / 24 | 600 | −0.32px (−0.02em) |
| heading-14 | 14 / 20 | 600 | −0.28px (−0.02em) |
| copy-16 / copy-14 / copy-13 | 16/24, 14/20, 13/18 | 400 | 0 |
| label-14 / label-12 | 14/20, 12/16 | 400 | 0 |
| button-14 / button-16 | 14/20, 16/20 | 500 | 0 |

- Naming convention: *heading* = titles, *label* = single-line scannable text (nav, form labels, table headers), *copy* = multi-line body, *button* = control labels. `copy-14` and `label-14` cover most text.
- Max two font weights per view.

**Spacing and layout:** `[R]`
- Scale: 4, 8, 12, 16, 24, 32, 40, 64, 96 px (base 4).
- Rhythm: 8px inside a group, 16px between groups, 32–40px between sections. Card padding 24px (16 compact, 32 hero).
- Content column 1200px. Breakpoints: 401 / 601 / 961 / 1200 / 1400 px.
- Control heights: 32 (small), 40 (default), 48 (large).

**Shape:** radius 6px everyday surfaces and controls, 12px menus and modals, 16px fullscreen surfaces, 9999px pills/avatars. One radius family per view; never mix rounded and sharp corners. `[R]`

**Elevation (dark theme):** raised cards `0 1px 2px rgba(0,0,0,.16)`; popovers `0 1px 1px rgba(0,0,0,.02), 0 4px 8px -4px rgba(0,0,0,.04), 0 16px 24px -8px rgba(0,0,0,.06)`; modals use a slightly larger version of the same layering. Always multi-layer and low opacity. `[R]`

**Motion:** `[R]`
- Default stance: many interactions should be instant (0ms is often best).
- When used: ~150ms state changes, ~200ms popovers/tooltips, ~300ms overlays/modals.
- Easing: `cubic-bezier(0.175, 0.885, 0.32, 1.1)`.
- Drop non-essential motion under `prefers-reduced-motion`.

**Components and states:** `[R]`
- Primary button is solid gray-1000 with a background-coloured label; reserve for the single most important action in a view. Secondary has a translucent border; tertiary is transparent and tints on hover; destructive uses solid red-800 with white text.
- Hover and active step up the scale (100→200→300 for fills, 400→500→600 for borders).
- Disabled: gray-100 fill, gray-700 text, not-allowed cursor.
- Focus ring: 2px gap in the surface colour, then a 2px blue ring (`box-shadow: 0 0 0 2px #000, 0 0 0 4px #47a8ff` on dark). Show on every interactive element at `:focus-visible`.

**Use when:** developer tools, dashboards, docs, anything needing "engineered, not decorated".
**Avoid when:** warm consumer products; Geist reads austere by design.

---

### 3.3 GitHub Primer

**Philosophy.** Functional density: system fonts, explicit borders, compact spacing, quiet chrome. Primer is built to be consumed by agents too (it publishes an MCP server for correct usage). `[P]`

**Token layering:** `[P]`
1. **Base** tokens map to raw values and are used only as references for other tokens. They do not respect colour modes and must never be used directly in UI.
2. **Functional** tokens express purpose (e.g. `borderColor-*`, `fgColor-*`) and respond to light/dark modes.
3. **Component/pattern** tokens are scoped to specific components.

**Typography rules:** `[P]`
- Tokens use `rem` so browser zoom works; line-heights are unitless and aligned to a 4px grid.
- Shorthand tokens set size, family, weight and line-height in one declaration.
- Don't use colour as the primary means of emphasis.
- Keep lines around 80 characters or fewer.
- Never change semantic heading order to get a visual result; fix it with styling.

**Reference values (via third-party mirror, verify):** `[S]` system-ui font stack for the whole product, monospace stack for code; base unit 4px with steps 4/8/12/16/20/24/32/48; button radius 6px; accent blue `#0969da`, success green `#1a7f37`; page max width about 1280px; dense list rows (about 16px horizontal, 12px vertical padding); hover feedback ~80ms, menu open ~200ms.

**Use when:** information-dense, text-heavy, list-and-table product surfaces.
**Avoid when:** you want a distinctive brand typeface; Primer deliberately has none.

---

### 3.4 Atlassian Design System (Jira, Confluence)

**Philosophy.** Predictable, named, theme-resolved tokens; a consistent spacing system produces a "more harmonious experience" and optical adjustment is allowed on top of it. `[P]` (spacing page)

**Spacing:** `[P]`
- Base unit 8px; tokens are named as a **percentage of the base**: `space.100` = 8px, `space.200` = 16px, `space.025` = 2px. Range 0 to 80px.
- Small range (0–8px, `space.0`–`space.100`): compact pieces such as icon-to-text gaps, badge and table-cell padding.
- Large range (32–80px, `space.400`–`space.1000`): layout and large separations between content.
- Visual weight can justify deviating from the pattern: adjust by eye, using scale values, to get balance (optical adjustment). Prefer the Bleed primitive over negative-space tokens.
- Intermediate steps (reported by a third-party writeup): `space.075` = 6, `space.150` = 12, `space.250` = 20. `[S]`

**Elevation model:** Sunken (e.g. Kanban column wells), Default, Raised (movable cards), Overlay (modals, dropdowns, popovers), plus an overflow shadow for scrolled content; tokens are `elevation.surface.*` and `elevation.shadow.*`. `[S]` Overlay surfaces pair with an overlay shadow; don't thicken every card's border instead.

**Other reported values:** radius xsmall 2 / small 4 / medium 6 px; type scale of heading, body and metric styles from 12 to 32 px; semantic colour tokens such as `color.background.brand.bold`, `color.text`, `color.text.subtle`. `[S]`

**Use when:** enterprise tools with many states, boards, and multi-level surfaces.

---

### 3.5 Stripe: accessible colour from the ground up

**Philosophy.** Accessible colour and good-looking colour aren't a trade-off if you work in a perceptually uniform space. `[P]`

**What they did:**
- Rebuilt the colour system in CIELAB (Lab), where lightness corresponds to how humans actually perceive it, unlike HSL.
- Built an internal visualization tool to see lightness and contrast across all hues at once and iterate against it.
- Result: text and icon colours at each step have uniform contrast across hues; they pass WCAG 2.0 contrast on white **and** on the lightest tint of any hue, so tinted backgrounds are safe.
- They flag the constraint: perceptual models expose that certain colours (for example, a very bright dark yellow) simply can't exist, so you design around physical limits. `[P][S]`

**Agent rule `[X]`:** define each palette step by target lightness (and chroma cap) in OKLCH/LCH, then solve hue; verify contrast programmatically for text-on-surface pairs, including text-on-tinted-surface.

---

### 3.6 Radix Colors (12-step role scales)

**Philosophy.** Every step in a scale has a defined job, so you never "pick a shade". `[P]`

| Step | Use |
|---|---|
| 1 | App background |
| 2 | Subtle background (cards, code blocks, sidebars, canvas areas) |
| 3 | Component background, normal |
| 4 | Component background, hover |
| 5 | Component background, pressed/selected |
| 6 | Subtle borders and separators (non-interactive: sidebars, headers, cards, alerts) |
| 7 | Component border and focus rings |
| 8 | Component border, hover (strong borders) |
| 9 | Solid backgrounds, the purest/highest-chroma step (buttons, badges) |
| 10 | Hovered solid backgrounds |
| 11 | Low-contrast text |
| 12 | High-contrast text |

- Step 9 is designed for white text on top; a handful of light hues (sky, mint, lime, yellow, amber) are designed for black text on 9 and 10.
- If a transparent component needs a hover, use step 3.
- For call-to-action emphasis you may bump component backgrounds up to 4/5/6.
- Dark mode uses the same step meanings with different values.

**Compare with Geist:** Geist's 10-step scale encodes the same ideas (bg, hover, border, solid, text). The convergence is the lesson: define roles by step. `[X]`

---

### 3.7 Apple: Liquid Glass (iOS/macOS 26 era)

**Philosophy.** A dynamic material combining the optical behaviour of glass with fluidity. Apple states its design principles for this era as **hierarchy, harmony, consistency**. `[P]`

**Primary-source guidance (Apple's Liquid Glass overview):** `[P]`
- Standard SwiftUI/UIKit/AppKit components pick up the material and behaviour automatically; custom elements can adopt the effect.
- Adopting it does not mean reinventing the app; rebuild with the latest SDK, then fix what looks off.
- Put the most important content in focus via layout and navigation structure.
- Be judicious with colour in controls and navigation so they stay legible and let content show through.
- Make interface elements fit the software and hardware design across devices.
- Use standard iconography and predictable action placement across platforms.
- App icons: simple, bold layers that offer dimensionality across appearances.
- Create edge-to-edge content with the background extension effect, and extend horizontal scroll views under sidebars/inspectors.

**Widely repeated guidance from commentary (verify against HIG before hard-coding):** `[S]`
- Controls float above content on glass and must not hide it; text sits on solid layers rather than directly on glass.
- Corners are concentric with hardware and with nested containers.
- Do not mix Regular and Clear glass variants in one interface.
- Respect Reduce Transparency and Reduce Motion.
- Critics argue legibility and consistency suffer when the material is overused. Treat glass as an accent for floating controls, not a surface for body content. `[X]`

**Longer-standing HIG numbers:** minimum tappable area 44×44pt. `[S]` iOS text styles (Large Title 34, Title 1 28, Title 2 22, Title 3 20, Headline 17 semibold, Body 17, Callout 16, Subheadline 15, Footnote 13, Caption 12/11) `[M]`: verify against the current HIG before use.

**Use when:** native Apple apps. If you must imitate it in web/cross-platform, copy the *behaviours* (content-first hierarchy, floating controls over content, adaptive tint, accessibility fallbacks), not just the blur. Linear's mobile app is a worked example: it built its own material and left out refraction. `[P]`

---

### 3.8 Material 3 Expressive (Google)

**Philosophy.** Personal, adaptive and expressive: dynamic colour from a seed or wallpaper, tonal surfaces instead of shadow-based elevation, richer shape vocabulary, and physics-based motion. `[S]`

**Key shifts `[S]`:**
- Depth from tone and shape, not heavy shadows or borders.
- Colour roles from a seed (via material-color-utilities); three user-selectable contrast levels.
- Shape scale tokens: none 0, extra-small 4, small 8, medium 12, large 16 dp (e.g. chips and snackbars 4, text fields and menus 8, cards 12, FABs and drawers 16). Larger steps exist (20, 28, 32, 48, full) `[M]`: verify.
- Shape morphing between states is a signature effect.

**Motion tokens: spring, not duration.** Values below come from a package that mirrors M3's published springs `[S]`:

| Token | Stiffness | Damping ratio | Feel |
|---|---|---|---|
| standard spatial fast / default / slow | 1400 / 700 / 300 | 0.9 | position, size, layout; small overshoot |
| expressive spatial fast / default / slow | 800 / 380 / 200 | 0.6 / 0.8 / 0.8 | bouncier; for key moments |
| effects fast / default / slow (opacity, colour) | 3800 / 1600 / 800 | 1.0 | no overshoot |

- Use *spatial* springs for movement and size and *effects* springs for opacity/colour, because fading shouldn't bounce.
- Fall back to fades when the system's animation scale or reduce-motion is on.
- Touch target guidance: 48dp. `[S]`

**Use when:** Android/Compose/Flutter products, or any product that wants personal, expressive motion and colour.

---

### 3.9 Figma UI3 (canvas-first tool UI)

**Philosophy.** Shift emphasis from the interface to the work on the canvas, while keeping tools in reach; leave logical places in the hierarchy for future features. `[P]`

**What shipped:** `[P]`
- Resizable and collapsible panels; a "minimize UI" action that collapses navigation and properties panels, and the properties panel reopens temporarily when you select something.
- A slim toolbar moved to the bottom of the canvas, freeing the top and making room for new AI actions.
- Reorganized navigation panel holding file information; property labels for clarity.
- Stated motivation: UI had become unapproachable to newcomers and features were being squeezed in ("playing Tetris"), so the hierarchy was rearranged to leave slots for growth.

**Reception (important counter-evidence):** Forum threads run to hundreds of replies, with users saying the floating bottom toolbar covers canvas, can't be docked elsewhere, and can blend into dark canvases. `[P]` Lesson `[X]`: moving a core tool's position breaks muscle memory and spatial flow; give users a way to dock/restore and test against dark and light canvases. Don't assume "less chrome" lands uniformly with expert users.

---

## 4. Cross-cutting specs (use these when you need numbers)

### 4.1 Colour roles (minimum set)
Define by role, then derive values.
`surface/app`, `surface/subtle`, `surface/raised`, `surface/overlay`, `surface/sunken`;
`fill/neutral` (default, hover, active); `fill/solid` (default, hover);
`border/subtle`, `border/default`, `border/strong`, `border/focus`;
`text/primary`, `text/secondary`, `text/disabled`, `text/on-solid`;
`status/info|success|warning|danger` each with bg, border, solid, text.
Rules:
- Text primary/secondary use step 12/11 (Radix) or 1000/900 (Geist) equivalents. `[P][R]`
- Don't signal state with colour alone; pair it with an icon or text. `[R]`
- Hold WCAG AA (4.5:1 body text; 3:1 large text and UI components). `[R]`
- Limit accent hue in neutrals; Linear removed most chrome hue from its neutrals for a timeless feel. `[P]`
- Keep solid accent for state and the single most important action per view. `[R]`

### 4.2 Typography
- One family for UI + one mono for code/data is the common pattern (Geist, Atlassian, Primer-with-system-fonts). Linear pairs a display cut with a text cut of one family. `[P][R]`
- Tracking tightens as size grows: roughly 0 at body, −0.02em at 14–20px headings, −0.04em at 24–32, −0.06em at 40+. `[R]`
- Line-height snaps to the grid (4px multiples). `[P][R]`
- Body line length ≤ ~80 characters. `[P]`
- Use tabular figures for aligned numbers (Geist Mono or `font-variant-numeric: tabular-nums`). `[R]`
- At most two weights per view. `[R]`

### 4.3 Spacing
- Choose one base (4px or 8px with named fractional steps) and name tokens. `[P][R]`
- Three-step rhythm: tight within a group, medium between groups, large between sections (Geist: 8 / 16 / 32–40). `[R]`
- Compact UI uses the small end of the scale (Atlassian: 0–8px); layout uses the large end (32–80px). `[P]`
- Optical adjustment is legitimate when visual weight demands it; do it with scale values, not arbitrary pixels. `[P]`

### 4.4 Shape
- Small radii for controls, larger for menus/modals/fullscreen surfaces (Geist 6/12/16; Atlassian 2/4/6). `[R][S]`
- Pills (9999px) only for pills, avatars, circular controls. `[R]`
- Nested rounded shapes should have concentric radii (outer = inner + padding). `[S]` (Apple commentary)
- One radius family per view. `[R]`

### 4.5 Elevation and depth
- Tone first (surface step), border second, shadow last. `[R][S]`
- Shadows are multi-layer, low-opacity; large blur with negative spread for popovers and modals. `[R]`
- Named surface levels (sunken / default / raised / overlay) beat ad-hoc z-index shadows. `[S]`

### 4.6 Motion
- Default instant; animate only to explain a change. `[R]`
- Durations: ~150ms state, ~200ms popover/tooltip, ~300ms overlay/modal; hover feedback as short as ~80ms in dense UIs. `[R][S]`
- Prefer spring physics for spatial change if the platform supports it (M3 values in §3.8); use no-overshoot easing for opacity and colour. `[S]`
- Always provide a reduced-motion path. `[R][S]`

### 4.7 Target size
| Standard | Minimum |
|---|---|
| WCAG 2.2 SC 2.5.8 (AA) | 24×24 CSS px, or 24px spacing offset from neighbours |
| WCAG SC 2.5.5 (AAA) | 44×44 CSS px |
| Apple | 44×44 pt |
| Material | 48×48 dp |
`[S]` (multiple consistent secondary sources; W3C draft text read directly for 2.5.8). Practical default: 44px touch targets, pad small icons rather than enlarging them visually; inline text links are exempt.

### 4.8 Focus and states
- Visible two-layer focus ring on every interactive element at `:focus-visible`, never an outline removed without replacement. `[R]`
- Hover/active step up the same scale (bg 100→200→300; border 400→500→600). `[R]`
- Disabled state is explicit and consistent. `[R]`
- Sidebar/nav inactive items use muted text; the active item carries the emphasis. `[P]` (Linear)

### 4.9 Content voice (from Geist's published guidance) `[R]`
- Action labels are verb + noun ("Deploy Project", "Delete Member"); avoid "OK", "Confirm", or a bare verb.
- Errors say what happened and what to do next.
- Toasts name the specific thing that changed; no trailing period; don't say "successfully".
- Empty states point to the first action.
- In-progress states use a present participle with an ellipsis character ("Saving…").
- Numerals, curly quotes, and the ellipsis character; no "please", no marketing superlatives.
- Title Case for labels, buttons, titles and tabs; sentence case for body, helper text and toasts (Geist's convention; match whichever your product already uses).

---

## 5. Choosing a philosophy

| Product situation | Lean on | Because |
|---|---|---|
| Expert daily-driver tool, dense information | Linear + Primer | Quiet chrome, tight spacing, keyboard-first |
| Developer-facing platform and docs | Geist | High contrast, mono-forward, state-coded colour |
| Enterprise product with many surface levels | Atlassian | Named elevations, 8px base with fractional steps |
| Theming / white-label / accessibility-critical | Stripe + Radix + Linear's 3-variable themes | Derive themes from few inputs in a perceptual space |
| Native Apple app | Apple HIG / Liquid Glass | Platform conventions beat custom invention |
| Android/Compose or expressive consumer app | Material 3 Expressive | Dynamic colour, spring motion, shape language |
| Canvas/editor with heavy tooling | Figma UI3 + Linear | Collapsible panels, recede-chrome; but keep tool placement flexible |

Mixing: pick **one** system for structure (spacing, type, shape) and borrow ideas (colour derivation, motion) from others. Don't mix radius families or two elevation models in a screen. `[X]`

---

## 6. Anti-patterns

**Documented in the sources above:**
- Equal visual weight on everything; bright navigation competing with content. (Linear)
- Borders and dividers added "just in case". (Linear)
- Icons everywhere, or coloured icon backgrounds as decoration. (Linear)
- HSL-adjusted shades that look muddy or uneven across hues. (Stripe, Linear)
- Using base/raw colour tokens directly in components. (Primer)
- Colour as the main emphasis mechanism. (Primer)
- Reordering heading levels for looks. (Primer)
- Mixed rounded and sharp corners, or more than two weights in one view. (Geist)
- Using `background-200`-style subtle fills as general fills; swapping text scales for background scales. (Geist)
- Removing focus outlines without replacement. (Geist)
- Long, looping, attention-grabbing animation. (Geist)
- Text directly on glass; glass on content areas. (Apple commentary)
- Heavy drop shadows for hierarchy. (Geist, M3)
- Relocating a core tool without a docking or restore option. (Figma backlash)

**Compiler's synthesis `[X]`:** (what separates real product UI from generic generated UI)
- Real products have a *named token system*; generated UI hard-codes hex values per element.
- Real products dim and shrink secondary UI; generated UI gives every card the same shadow and radius.
- Real products specify hover, active, focus, disabled and loading states; generated UI ships only the default.
- Real products have empty and error states with a next action; generated UI shows blank space or "Something went wrong".
- Real products snap type and spacing to a grid; generated UI uses arbitrary pixel values.
- Real products pick **one** idea to be distinctive (Linear's calm density, Geist's austerity) and keep everything else quiet.

---

## 7. Starter token set (synthesized, `[X]`)

A neutral scaffold built from the structures above. It is not any one company's palette; derive actual colour values with OKLCH per the rules in §3.5 and §4.1.

```css
:root {
  color-scheme: light dark;

  /* Spacing: 4px base (Geist / Primer convention) */
  --space-1: 4px;  --space-2: 8px;  --space-3: 12px; --space-4: 16px;
  --space-6: 24px; --space-8: 32px; --space-10: 40px; --space-16: 64px;

  /* Shape: one family per view */
  --radius-control: 6px;   /* buttons, inputs */
  --radius-surface: 12px;  /* menus, modals, cards */
  --radius-large: 16px;    /* fullscreen sheets */
  --radius-pill: 9999px;

  /* Heights */
  --control-sm: 32px; --control-md: 40px; --control-lg: 48px;

  /* Motion */
  --dur-instant: 0ms;
  --dur-state: 150ms;
  --dur-popover: 200ms;
  --dur-overlay: 300ms;
  --ease-ui: cubic-bezier(0.175, 0.885, 0.32, 1.1); /* Geist */

  /* Type: tokenize as complete styles */
  --font-ui: "Inter", system-ui, sans-serif;     /* swap for the product's family */
  --font-mono: ui-monospace, "SF Mono", Menlo, Consolas, monospace;
  --track-heading-sm: -0.02em;   /* 14-20px */
  --track-heading-md: -0.04em;   /* 24-32px */
  --track-heading-lg: -0.06em;   /* 40px+ */
}

/* Colour: define ROLES; fill values from an OKLCH-generated scale */
:root {
  --surface-app:      /* step 1  */;
  --surface-subtle:   /* step 2  */;
  --fill-neutral:     /* step 3  */;
  --fill-neutral-hover:   /* step 4 */;
  --fill-neutral-active:  /* step 5 */;
  --border-subtle:    /* step 6  */;
  --border-default:   /* step 7  */;
  --border-strong:    /* step 8  */;
  --fill-solid:       /* step 9  */;
  --fill-solid-hover: /* step 10 */;
  --text-secondary:   /* step 11 */;
  --text-primary:     /* step 12 */;
}

@media (prefers-reduced-motion: reduce) {
  * { animation-duration: 0.01ms !important; transition-duration: 0.01ms !important; }
}
:focus-visible {
  outline: none;
  box-shadow: 0 0 0 2px var(--surface-app), 0 0 0 4px var(--border-focus, currentColor);
}
```

Generation recipe (`[X]` combining Stripe, Linear, Radix):
1. Choose base hue and accent hue; set chroma low for neutrals.
2. Fix lightness per step in OKLCH (same L targets for every hue so contrast is uniform).
3. Verify: text-primary and text-secondary against surface-app and surface-subtle ≥ 4.5:1; against tinted status backgrounds too.
4. Offer a contrast control that rescales L spread (Linear's contrast variable).
5. Derive dark mode by re-mapping the same role names, not by inverting.

---

## 8. Pre-ship checklist

- [ ] Every colour in code comes from a role token; none are hard-coded.
- [ ] Text contrast ≥ 4.5:1 (including on tinted surfaces); UI components ≥ 3:1.
- [ ] Hover, active, focus-visible, disabled, loading, empty and error states exist.
- [ ] One radius family; at most two font weights; spacing from the scale.
- [ ] Touch targets ≥ 44px on touch (24px absolute floor on pointer UIs).
- [ ] Secondary chrome is visibly quieter than content; borders only where they explain structure.
- [ ] Motion only explains change; reduced-motion path exists.
- [ ] State isn't communicated by colour alone.
- [ ] Copy: verb + noun actions, errors say what happened and what to do, empty states give a first action.
- [ ] Heading order is semantic; line length ≤ ~80ch.

---

## 9. Sources and confidence

**Primary `[P]`**
- Linear, "How we redesigned the Linear UI (part II)", 2024: https://linear.app/blog/how-we-redesigned-the-linear-ui
- Linear, "A calmer interface for a product in motion", 12 Mar 2026: https://linear.app/blog/behind-the-latest-design-refresh
- Linear, "A Linear spin on Liquid Glass" (mobile), 2025: https://linear.app/blog/linear-liquid-glass
- Vercel Geist index (agent-readable Markdown for docs pages): https://vercel.com/geist
- Apple, Liquid Glass overview: https://developer.apple.com/documentation/technologyoverviews/liquid-glass.md
- GitHub Primer (typography, colour usage, tokens): https://primer.style/foundations/ and https://primer.style/product/getting-started/foundations/typography
- Atlassian spacing: https://atlassian.design/foundations/spacing
- Stripe, "Designing accessible color systems": https://stripe.com/blog/accessible-color-systems
- Radix Colors, "Understanding the scale": https://www.radix-ui.com/colors/docs/palette-composition/understanding-the-scale
- Figma, UI3 posts: https://www.figma.com/blog/behind-our-redesign-ui3/ and https://www.figma.com/blog/our-approach-to-designing-ui3/ ; help article https://help.figma.com/hc/articles/23954856027159

**Republished `[R]`**
- Vercel Geist design.md (dark theme), read via https://dev.to/tene/vercel-designdarkmd-53l. Token values, type scale, motion, elevation, voice rules. Light-theme values not retrieved.

**Secondary `[S]` (verify before hard-coding)**
- Material 3 Expressive spring tokens via the Flutter `motor` package: https://pub.dev/packages/motor
- M3 shape tokens via a community skill file: https://openskillindex.com/skills/hamen-material-3-skill-material-3
- Atlassian elevation/radius/type via a third-party writeup: https://www.superdesign.dev/blog/atlassian-design-system
- Primer reference values via https://open-design.ai/plugins/design-system-github/
- Touch-target figures: https://tetralogical.com/blog/2022/12/20/foundations-target-size and others

**Known gaps (not covered; do not infer):**
- Official Material 3 pages (m3.material.io) could not be fetched this session; M3 values rely on `[S]` mirrors.
- Apple HIG numeric type scale and layout metrics were not retrieved; the iOS text-style numbers in §3.7 are `[M]`.
- Geist light-theme values, Geist materials/grid pages not retrieved.
- Not researched: Notion, Spotify, Airbnb, Slack, Microsoft Fluent, IBM Carbon, Shopify Polaris, Raycast, Arc. Add these before treating this file as comprehensive for consumer apps.
- Figma's own stated rationale was read from its blog; user reaction comes from forum threads and is anecdotal, not usage data.
