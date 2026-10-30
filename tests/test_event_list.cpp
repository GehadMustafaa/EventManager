#include "EventList.h"
#include "TestHarness.h"

TEST(event_list_append_find_remove) {
    EventList list;
    list.append(Event(1, "A", "2026-01-01", "V", 10));
    list.append(Event(2, "B", "2026-01-02", "V", 10));
    list.append(Event(3, "C", "2026-01-03", "V", 10));
    CHECK(list.size() == 3);
    CHECK(list.findById(2)->name() == "B");
    CHECK(list.removeById(3));
    CHECK(list.removeById(1));
    CHECK(!list.removeById(1));
    CHECK(list.size() == 1);
    list.append(Event(4, "D", "2026-01-04", "V", 10));
    CHECK(list.findById(4) != nullptr);
    CHECK(list.findById(3) == nullptr);
}

TEST(event_list_clear) {
    EventList list;
    list.append(Event(1, "A", "2026-01-01", "V", 10));
    list.clear();
    CHECK(list.empty());
    CHECK(list.findById(1) == nullptr);
}
