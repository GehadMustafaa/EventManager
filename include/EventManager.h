#pragma once
#include <string>
#include <vector>
#include "Attendee.h"
#include "Event.h"
#include "EventList.h"
#include "HashTable.h"

class EventManager {
public:
    enum class SortField { Id, Name, Date, Capacity, Booked };
    enum class SortOrder { Ascending, Descending };

    int createEvent(const std::string& name, const std::string& date, const std::string& venue, int capacity,
                    const std::string& description = {}, const std::string& time = "09:00 AM");
    std::vector<int> updateEvent(int eventId, const std::string& name, const std::string& date, const std::string& venue,
                                 int capacity, const std::string& description = {}, const std::string& time = "09:00 AM");
    bool deleteEvent(int eventId);

    int registerAttendee(const std::string& name, const std::string& email);
    void updateAttendee(int attendeeId, const std::string& name, const std::string& email);
    bool deleteAttendee(int attendeeId);

    Event::BookResult book(int eventId, int attendeeId);
    Event::CancelResult cancel(int eventId, int attendeeId);

    const Event* findEvent(int eventId) const;
    const Event* findEventByName(const std::string& name) const;
    const Attendee* findAttendee(int attendeeId) const;
    const Attendee* findAttendeeByEmail(const std::string& email) const;

    std::vector<const Event*> events() const;
    std::vector<const Event*> eventsSorted(SortField field, SortOrder order = SortOrder::Ascending) const;
    std::vector<Attendee> attendees() const;

    void clear();

    void restoreAttendee(const Attendee& attendee);
    void restoreEvent(int id, const std::string& name, const std::string& date, const std::string& venue, int capacity,
                      const std::string& description = {}, const std::string& time = "09:00 AM");
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
