#include <cstdio>
#include <fstream>
#include "EventManager.h"
#include "Storage.h"
#include "TestHarness.h"

TEST(storage_round_trip_preserves_state_and_waitlist_order) {
    const std::string path = "sem_roundtrip.tmp";
    EventManager original;
    int e = original.createEvent("Show", "2026-06-01", "Arena", 1);
    int a = original.registerAttendee("A", "a@x.com");
    int b = original.registerAttendee("B", "b@x.com");
    int c = original.registerAttendee("C", "c@x.com");
    original.book(e, a);
    original.book(e, b);
    original.book(e, c);
    Storage::save(original, path);

    EventManager restored;
    Storage::load(restored, path);
    std::remove(path.c_str());

    const Event* ev = restored.findEvent(e);
    CHECK(ev != nullptr);
    CHECK(ev->isConfirmed(a));
    CHECK(ev->waitlistIds() == std::vector<int>({b, c}));
    CHECK(restored.findAttendeeByEmail("c@x.com")->id == c);
    CHECK(restored.registerAttendee("D", "d@x.com") == c + 1);
}

TEST(storage_load_missing_file_throws) {
    EventManager m;
    CHECK_THROWS(Storage::load(m, "does_not_exist.tmp"));
}

TEST(storage_load_malformed_file_throws_and_clears) {
    const std::string path = "sem_bad.tmp";
    {
        std::ofstream f(path);
        f << "A|1|Name\n";
    }
    EventManager m;
    CHECK_THROWS(Storage::load(m, path));
    std::remove(path.c_str());
    CHECK(m.attendees().empty());
}
