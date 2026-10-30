# Smart Event Management System: Data Structures and Complexity Report

## 1. Data Structure Mapping

| Structure | Implementation | Holds | Reason |
|---|---|---|---|
| Singly linked list | `EventList` (head and tail pointers) | All events in creation order | O(1) append, no resizing, cheap removal once a node is located |
| Queue (linked) | `Queue<int>` inside each `Event` | Waiting list of attendee IDs | FIFO guarantees first-come promotion; O(1) enqueue and dequeue |
| Hash table (separate chaining) | `HashTable<K,V>` | Event ID index, event name index, attendee table, attendee email index, per-event confirmed set | O(1) average lookup, insertion, and deletion |

## 2. Operation Complexity

n = number of events, a = number of attendees, c = confirmed attendees in an event, w = waiting list size of an event.

| Operation | Time (average) | Time (worst) | Notes |
|---|---|---|---|
| Create event | O(1) | O(n) on rehash (amortized O(1)) | Append to list, insert into two indexes |
| Find event by ID or name | O(1) | O(n) | Hash index |
| Find event by ID in list only | O(n) | O(n) | Used for benchmark comparison |
| Delete event | O(n) | O(n) | List node removal requires a scan |
| Register attendee | O(1) | O(a) on rehash (amortized O(1)) | Two hash inserts |
| Find attendee by ID or email | O(1) | O(a) | Hash index |
| Book ticket | O(w) | O(w + c) | Duplicate check scans waiting list; confirmed check is O(1) |
| Cancel confirmed booking and promote | O(1) | O(c) | Hash erase plus queue dequeue |
| Cancel waitlisted booking | O(w) | O(w) | Linear removal from queue |
| List events | O(n) | O(n) | Traversal |
| Save / load | O(n + a + bookings) | same | Single pass |

Space: O(n + a + bookings).

## 3. Benchmark

Run `./build/sem_bench` and paste the output table here. Expected shape: list find grows linearly with N, hash find stays flat, queue enqueue/dequeue stays flat.

## 4. Test Results

Run `ctest --test-dir build --output-on-failure` or `./build/sem_tests` and paste the output here.

## 5. Design Decisions

- Event name and attendee email are unique and matched case-insensitively.
- Attendees are never removed, so booking records cannot reference a missing attendee.
- `|` and newline are rejected in text fields because the storage format uses `|` as a delimiter.
- `Event` and its containers are move-only, which prevents accidental duplication of seat state.
