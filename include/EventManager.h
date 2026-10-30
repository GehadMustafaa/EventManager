#pragma once
#include <string>
#include <vector>
#include "Attendee.h"
#include "Event.h"
#include "EventList.h"
#include "HashTable.h"

class EventManager {
public:
    int createEvent(const std::string& name, const std::string& date, const std::string& venue, int capacity);
    bool deleteEvent(int eventId);

    int registerAttendee(const std::string& name, const std::string& email);

    Event::BookResult book(int eventId, int attendeeId);
    Event::CancelResult cancel(int eventId, int attendeeId);

    const Event* findEvent(int eventId) const;
    const Event* findEventByName(const std::string& name) const;
    const Attendee* findAttendee(int attendeeId) const;
    const Attendee* findAttendeeByEmail(const std::string& email) const;

    std::vector<const Event*> events() const;
    std::vector<Attendee> attendees() const;

    void clear();

    void restoreAttendee(const Attendee& attendee);
    void restoreEvent(int id, const std::string& name, const std::string& date, const std::string& venue, int capacity);
    void restoreConfirmed(int eventId, int attendeeId);
    void restoreWaitlisted(int eventId, int attendeeId);

private:
    Event* lookupEvent(int eventId) const;

    EventList events_;
    HashTable<int, Event*> eventById_;
    HashTable<std::string, int> eventIdByName_;
    HashTable<int, Attendee> attendeesById_;
    HashTable<std::string, int> attendeeIdByEmail_;
    int nextEventId_ = 1;
    int nextAttendeeId_ = 1;
};
