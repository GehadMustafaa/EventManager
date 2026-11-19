# GUI Design Notes

The Qt GUI follows `design-philosophies.md` (Linear, Geist, Primer, Atlassian, Stripe, Radix). Everything visual lives in `gui/Theme.cpp`; widgets only set properties (`variant`, `chip`, `role`, `invalid`, `state`).

| Rule (section of the reference) | What the GUI does |
|---|---|
| Rank attention (§0.1) | The booking inspector leads. Lists use quiet panels; IDs, dates, venues and emails use secondary text. |
| Felt, not seen (§0.2) | No grid lines. Rows are bands (hover and selected tone). One soft panel outline only. |
| Colour as roles (§0.3, §3.6) | 12-step neutral and accent scales plus success, warning and danger. The stylesheet only references roles such as `@{text-2}`. |
| Perceptual colour (§0.4) | Palette generated in OKLCH at one lightness per step across hues, then contrast-checked. |
| Grid and shape (§0.6) | 4px spacing tokens, 36px rows, 32px controls. Radii: 6 for controls, 12 for panels. |
| Type (§0.7) | 13px body, 12px captions, 20px titles with -0.02em tracking. Two weights (400, 600). Inter, then the system font. |
| Motion (§0.8) | None. Every interaction is instant. |
| Accessibility (§0.9) | Text at least 4.5:1, input borders at least 3:1 (checked in both modes). Keyboard focus rings only. State is always also text ("Full", "Almost Full"). |
| Single primary (§3.2) | "Book Attendee" is the only solid button in the main window. Dialog submit buttons are primary inside their own dialog. |
| Copy (§4.9) | Verb + noun buttons, errors say what to do next, empty states point to the first action, toasts name the item and have no trailing period. |

## Behaviour changes (GUI only, core untouched)
- Register Attendee and New Event use dialogs with inline validation. Duplicate name or email keeps the dialog open.
- Book and Cancel moved next to the event they change, and are disabled when they don't apply (hint text says why).
- View > Dark Mode (Ctrl+Shift+D); follows the system until chosen, then remembered.

## Not verified
Written without a Qt toolchain: needs a first build and visual pass. Check the QDateEdit drop-down arrow and calendar popup, and the QMessageBox button row.
