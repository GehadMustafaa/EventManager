# Smart Event Management System

Desktop and CLI event manager written in C++17 and Qt Widgets for scheduling events, tracking attendees, managing bookings, and automating FIFO waiting lists.

---

## Highlights

- **Event Management** — Create, edit, search, sort (`id`, `name`, `date`, `capacity`, `booked`), filter (`All`, `Open`, `Almost Full`, `Full`), and delete events with calendar date (`YYYY-MM-DD`), 12-hour time (`hh:mm AM/PM`), venue, capacity, and multi-line notes.
- **Attendees & Waitlists** — Register, edit, search, and remove attendees with case-insensitive unique email checks. Bookings automatically move to a FIFO waitlist when capacity is reached and auto-promote when a seat frees up or capacity grows.
- **Desktop GUI (`sem_gui`)** — Master-detail workspace with inline form validation, a fuzzy `Ctrl+K` Command Palette, `Ctrl+Z` snapshot undo with toast notifications, safe confirmation dialogs, Light/Dark and High-Contrast OKLCH themes, keyboard-only focus rings, and UI Automation metadata.
- **Safe Persistence (`V|2`)** — Writes atomically via `.tmp` + `.bak` backup, reads legacy v1 files, exports to CSV, and switches to recovery mode instead of overwriting a corrupt data file.
- **Interactive CLI (`sem`)** — 17-action console menu sharing the same core engine and file format as the GUI.

## Quick Start

**Requirements:** C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+), CMake 3.16+, and Qt 6 (or Qt 5) Widgets for the GUI.

```bash
cmake -S . -B build
cmake --build build
```

Run either interface (passing an optional file path lets both read the same database):

```bash
./build/sem_gui [data-file]   # Qt desktop app (defaults to per-user app data dir)
./build/sem [data-file]       # Console menu (defaults to data/events.db)
```

Run tests and benchmarks:

```bash
ctest --test-dir build --output-on-failure
./build/sem_bench
```

**CMake options:**
- `-DSEM_BUILD_GUI=OFF` — skip `sem_gui` and build core/CLI targets only
- `-DSEM_SANITIZERS=ON` — enable ASan/UBSan on `sem_core` (GCC/Clang)

## Keyboard Shortcuts (`sem_gui`)

| Shortcut | Action |
|---|---|
| `Ctrl+K` | Open Command Palette |
| `Ctrl+1` / `Ctrl+2` | Switch between Events and Attendees |
| `/` | Focus search/filter bar |
| `Ctrl+N` / `Ctrl+Shift+N` | New event / Register attendee |
| `Ctrl+E` / `Ctrl+Shift+E` | Edit selected event / attendee |
| `Del` / `Ctrl+Shift+Backspace` | Delete selected event / attendee |
| `Ctrl+B` / `Ctrl+Shift+B` | Book / Cancel quick booking |
| `Ctrl+Z` / `Ctrl+S` | Undo / Save |
| `Ctrl+Shift+D` / `Ctrl+Shift+H` | Toggle Dark Mode / High Contrast |

## Directory Structure

```text
include/   Core headers (Event, EventManager, EventList, HashTable, Queue, DateUtil, Snapshot, Storage, Menu)
src/       Core implementation and CLI entry point
gui/       Qt Widgets UI, Command Palette, OKLCH themes, delegates, and dialogs
scripts/   generate_tokens.py (regenerates gui/ThemeTokens.inc)
tests/     Unit tests (TestHarness.h via CTest)
bench/     Empirical complexity benchmark
docs/      DESIGN.md and REPORT.md
```
