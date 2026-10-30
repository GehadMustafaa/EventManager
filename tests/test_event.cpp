#include "Event.h"
#include "TestHarness.h"

TEST(event_books_until_capacity_then_waitlists) {
    Event e(1, "E", "2026-01-01", "V", 2);
    CHECK(e.book(10) == Event::BookResult::Confirmed);
    CHECK(e.book(11) == Event::BookResult::Confirmed);
    CHECK(e.book(12) == Event::BookResult::Waitlisted);
    CHECK(e.seatsAvailable() == 0);
    CHECK(e.waitlistCount() == 1);
}

TEST(event_rejects_duplicate_booking) {
    Event e(1, "E", "2026-01-01", "V", 1);
    e.book(10);
    e.book(11);
    CHECK(e.book(10) == Event::BookResult::Duplicate);
    CHECK(e.book(11) == Event::BookResult::Duplicate);
}

TEST(event_cancel_promotes_first_waitlisted) {
    Event e(1, "E", "2026-01-01", "V", 1);
    e.book(10);
    e.book(11);
    e.book(12);
    Event::CancelResult r = e.cancel(10);
    CHECK(r.cancelled);
    CHECK(r.promotedAttendeeId.has_value());
    CHECK(*r.promotedAttendeeId == 11);
    CHECK(e.isConfirmed(11));
    CHECK(e.isWaitlisted(12));
    CHECK(e.seatsAvailable() == 0);
}

TEST(event_cancel_waitlisted_does_not_promote) {
    Event e(1, "E", "2026-01-01", "V", 1);
    e.book(10);
    e.book(11);
    Event::CancelResult r = e.cancel(11);
    CHECK(r.cancelled);
    CHECK(!r.promotedAttendeeId.has_value());
    CHECK(e.waitlistCount() == 0);
}

TEST(event_cancel_unknown_attendee) {
    Event e(1, "E", "2026-01-01", "V", 1);
    CHECK(!e.cancel(99).cancelled);
}
