# Smart Event Management System

C++17 event, attendee, booking, and waiting-list manager with an interactive console menu (`sem`) and a Qt Widgets desktop application (`sem_gui`).

## Scope

In scope:
- Create, edit, list, sort, search, filter, and delete events with validated calendar dates (`YYYY-MM-DD`), 12-hour times (`hh:mm AM/PM`), venues, seat capacities, and optional multi-line notes
- Register, edit, search, list, and delete attendees with unique case-insensitive email enforcement
- Book attendees into events with automatic FIFO waiting lists when full, and auto-promote waitlisted attendees on cancellation or capacity increases
- Search and sort events (`id`, `name`, `date`, `capacity`, `booked`), filter by availability (`All`, `Open`, `Almost Full`, `Full`), and export snapshots to CSV
- Snapshot-based undo, atomic persistence (`V|2` pipe-delimited format, legacy v1 compatible) with automatic `.bak` backups and corrupt-file recovery mode
- Qt Widgets desktop interface featuring a Command Palette (`Ctrl+K`), inline form validation, confirmation dialogs with dependency counts, Light/Dark and High-Contrast OKLCH themes, keyboard navigation, and Windows UI Automation metadata
- Unit tests, empirical Big-O benchmark, and complexity report

Out of scope: networking, authentication, payments, external database engines, and multi-threading.

## Tech Stack

- Language: C++17, standard library only for `sem_core`
- Build: CMake 3.16+ (`-DSEM_BUILD_GUI=ON|OFF`, `-DSEM_SANITIZERS=ON|OFF`)
- Compilers: GCC 9+, Clang 10+, MSVC 2019+
- Testing: minimal in-repo harness (`tests/TestHarness.h`) registered with CTest (`sem_tests`; optional `sem_gui_tests` if `tests/test_gui.cpp` and `QtTest` are present)
- Persistence: versioned pipe-delimited text file (`V|2`, base64-encoded notes) with `.bak` backup and CSV export
- Interface: 17-option console menu (`sem`) and Qt Widgets desktop GUI (`sem_gui`, Qt 6 or Qt 5)

## Build and Run

    cmake -S . -B build
    cmake --build build
    ./build/sem [data-file]
    ./build/sem_tests
    ./build/sem_bench
    ./build/sem_gui [data-file]

`sem_gui` is built only when Qt Widgets is found; pass `-DSEM_BUILD_GUI=OFF` to skip it. Without a path argument the CLI defaults to `data/events.db`, while the GUI stores data in the per-user application data folder. Both executables share the same file format, so pass the same path to open the same dataset.

## Layout

| Path | Purpose |
|---|---|
| `include/Attendee.h` | Attendee record (`id`, `name`, `email`) |
| `include/DateUtil.h` | Calendar date (`YYYY-MM-DD`, leap years) and 12-hour time (`hh:mm AM/PM`) validation and formatting |
| `include/Queue.h` | Template linked-list queue (waiting list) |
| `include/HashTable.h` | Template chained hash table (indexes, confirmed set) |
| `include/Event.h`, `src/Event.cpp` | Event entity, seat accounting, booking, cancellation, and capacity-increase promotions |
| `include/EventList.h`, `src/EventList.cpp` | Singly linked list of events |
| `include/EventManager.h`, `src/EventManager.cpp` | Application service: validation, indexes, sorting, cascading deletes, orchestration |
| `include/Snapshot.h` | State snapshot stack for undo |
| `include/Storage.h`, `src/Storage.cpp` | Versioned save/load, atomic `.tmp` replace, `.bak` restore, and CSV export |
| `include/Menu.h`, `src/Menu.cpp` | Console interface (`17` actions + exit) with optional auto-save |
| `src/main.cpp` | Console entry point |
| `gui/MainWindow.h/.cpp` | Main GUI window: events/attendees tables, detail inspector, quick booking, waitlist, status filters, sorting, search, recovery banner |
| `gui/EventDialog.h/.cpp` | Create/edit event dialog (name, calendar date picker, 12-hour time, venue, capacity, notes) |
| `gui/AttendeeDialog.h/.cpp` | Register/edit attendee dialog |
| `gui/ConfirmDialog.h/.cpp` | Confirmation and alert dialogs (destructive confirmations default to the safe cancel button and report booking/waitlist impact) |
| `gui/CommandRegistry.h/.cpp`, `gui/CommandPalette.h/.cpp` | Fuzzy command registry and `Ctrl+K` Command Palette (`Quit`, navigation, editing, booking, recovery actions) |
| `gui/ActionToast.h/.cpp` | Transient toast notifications with inline `Undo` (`Ctrl+Z`) |
| `gui/Delegates.h/.cpp`, `gui/FormField.h` | Table/booking row painting (`Qt::ElideMiddle`, seat meters, hover remove button) and validated form field wrapper |
| `gui/Theme.h/.cpp`, `gui/ThemeTokens.inc` | Light, Dark, and High-Contrast themes generated from OKLCH role tokens, keyboard-only focus rings, and QSS styling |
| `scripts/generate_tokens.py` | OKLCH token generator for `gui/ThemeTokens.inc` |
| `gui/main_gui.cpp` | GUI entry point |
| `tests/` | Unit tests per component (`TestHarness.h`, run via `sem_tests` / `ctest`) |
| `bench/benchmark.cpp` | Empirical Big-O measurements |
| `docs/DESIGN.md`, `docs/REPORT.md` | GUI design notes and data structure complexity report |
