#include "EventManager.h"
#include "TestHarness.h"

TEST(manager_create_and_find_event) {
    EventManager m;
    int id = m.createEvent("Conf", "2026-05-01", "Hall", 5);
    CHECK(m.findEvent(id) != nullptr);
    CHECK(m.findEventByName("conf")->id() == id);
    CHECK(m.findEventByName("missing") == nullptr);
}

TEST(manager_rejects_invalid_input) {
    EventManager m;
    CHECK_THROWS(m.createEvent("", "2026-05-01", "Hall", 5));
    CHECK_THROWS(m.createEvent("A|B", "2026-05-01", "Hall", 5));
    CHECK_THROWS(m.createEvent("A", "2026-05-01", "Hall", 0));
    m.createEvent("A", "2026-05-01", "Hall", 1);
    CHECK_THROWS(m.createEvent("a", "2026-05-02", "Hall", 1));
}

TEST(manager_register_attendee_unique_email) {
    EventManager m;
    int id = m.registerAttendee("Ann", "ann@x.com");
    CHECK(m.findAttendeeByEmail("ANN@x.com")->id == id);
    CHECK_THROWS(m.registerAttendee("Ann2", "ann@x.com"));
}

TEST(manager_booking_flow_with_waitlist_promotion) {
    EventManager m;
    int e = m.createEvent("Show", "2026-06-01", "Arena", 1);
    int a = m.registerAttendee("A", "a@x.com");
    int b = m.registerAttendee("B", "b@x.com");
    CHECK(m.book(e, a) == Event::BookResult::Confirmed);
    CHECK(m.book(e, b) == Event::BookResult::Waitlisted);
    Event::CancelResult r = m.cancel(e, a);
    CHECK(r.promotedAttendeeId.has_value());
    CHECK(*r.promotedAttendeeId == b);
    CHECK(m.findEvent(e)->isConfirmed(b));
}

TEST(manager_unknown_ids_throw) {
    EventManager m;
    int e = m.createEvent("Show", "2026-06-01", "Arena", 1);
    CHECK_THROWS(m.book(999, 1));
    CHECK_THROWS(m.book(e, 999));
    CHECK_THROWS(m.cancel(e, 999));
}

TEST(manager_delete_event_frees_name) {
    EventManager m;
    int e = m.createEvent("Show", "2026-06-01", "Arena", 1);
    CHECK(m.deleteEvent(e));
    CHECK(!m.deleteEvent(e));
    CHECK(m.findEventByName("Show") == nullptr);
    CHECK(m.events().empty());
    m.createEvent("Show", "2026-06-02", "Arena", 1);
}
