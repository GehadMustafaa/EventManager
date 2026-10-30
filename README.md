# Smart Event Management System

C++17 console application for managing events, attendee registration, ticket bookings, and waiting lists.

## Scope

In scope:
- Create, list, search, and delete events
- Register attendees with unique emails
- Book tickets with automatic waiting list when an event is full
- Cancel bookings with automatic promotion of the first waitlisted attendee
- Search events by name or ID and attendees by email or ID
- Save and load all data to a text file
- Unit tests, benchmark, and complexity report

Out of scope: networking, authentication, payments, database engines, multi-threading, attendee deletion or editing, event editing.

## Tech Stack

- Language: C++17, standard library only
- Build: CMake 3.16+
- Compilers: GCC 9+, Clang 10+, MSVC 2019+
- Testing: in-repo minimal harness (`tests/TestHarness.h`) run through CTest
- Persistence: pipe-delimited text file
- Interface: console menu and Qt Widgets GUI (Qt 6, Qt 5 also supported)

## Build and Run

    cmake -S . -B build
    cmake --build build
    ./build/sem [data-file]
    ./build/sem_tests
    ./build/sem_bench
    ./build/sem_gui [data-file]

`sem_gui` is built only when Qt is found; pass `-DSEM_BUILD_GUI=OFF` to skip it. Without a path argument the GUI stores data in the per-user application data folder. Both executables read the same file format, so pass the same path to share data.

Default data file: `data/events.db`.

## Layout

| Path | Purpose |
|---|---|
| `include/Attendee.h` | Attendee record |
| `include/Queue.h` | Template linked-list queue (waiting list) |
| `include/HashTable.h` | Template chained hash table (indexes, confirmed set) |
| `include/Event.h`, `src/Event.cpp` | Event entity, seat accounting, booking and cancellation rules |
| `include/EventList.h`, `src/EventList.cpp` | Singly linked list of events |
| `include/EventManager.h`, `src/EventManager.cpp` | Application service: validation, indexes, orchestration |
| `include/Storage.h`, `src/Storage.cpp` | File persistence |
| `include/Menu.h`, `src/Menu.cpp` | Console interface |
| `src/main.cpp` | Console entry point |
| `gui/MainWindow.h/.cpp` | Main GUI window: events, attendees, booking, waiting list view, save/load |
| `gui/EventDialog.h/.cpp` | New-event form |
| `gui/main_gui.cpp` | GUI entry point |
| `tests/` | Unit tests per component |
| `bench/benchmark.cpp` | Empirical Big-O measurements |
| `docs/REPORT.md` | Data structure and complexity report |
