#include "EventManager.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {

std::string normalize(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

void requireText(const std::string& value, const char* field) {
    if (value.empty()) throw std::invalid_argument(std::string(field) + " must not be empty");
    if (value.find('|') != std::string::npos || value.find('\n') != std::string::npos)
        throw std::invalid_argument(std::string(field) + " contains an invalid character");
}

}

Event* EventManager::lookupEvent(int eventId) const {
    Event* const* slot = eventById_.find(eventId);
    return slot ? *slot : nullptr;
}

int EventManager::createEvent(const std::string& name, const std::string& date, const std::string& venue, int capacity) {
    requireText(name, "event name");
    requireText(date, "event date");
    requireText(venue, "event venue");
    if (capacity <= 0) throw std::invalid_argument("capacity must be positive");
    if (eventIdByName_.contains(normalize(name))) throw std::invalid_argument("event name already exists");

    int id = nextEventId_++;
    restoreEvent(id, name, date, venue, capacity);
    return id;
}

bool EventManager::deleteEvent(int eventId) {
    Event* event = lookupEvent(eventId);
    if (!event) return false;
    eventIdByName_.erase(normalize(event->name()));
    eventById_.erase(eventId);
    return events_.removeById(eventId);
}

int EventManager::registerAttendee(const std::string& name, const std::string& email) {
    requireText(name, "attendee name");
    requireText(email, "attendee email");
    if (attendeeIdByEmail_.contains(normalize(email))) throw std::invalid_argument("email already registered");

    int id = nextAttendeeId_++;
    restoreAttendee(Attendee{id, name, email});
    return id;
}

Event::BookResult EventManager::book(int eventId, int attendeeId) {
    Event* event = lookupEvent(eventId);
    if (!event) throw std::out_of_range("event not found");
    if (!attendeesById_.contains(attendeeId)) throw std::out_of_range("attendee not found");
    return event->book(attendeeId);
}

Event::CancelResult EventManager::cancel(int eventId, int attendeeId) {
    Event* event = lookupEvent(eventId);
    if (!event) throw std::out_of_range("event not found");
    if (!attendeesById_.contains(attendeeId)) throw std::out_of_range("attendee not found");
    return event->cancel(attendeeId);
}

const Event* EventManager::findEvent(int eventId) const {
    return lookupEvent(eventId);
}

const Event* EventManager::findEventByName(const std::string& name) const {
    const int* id = eventIdByName_.find(normalize(name));
    return id ? lookupEvent(*id) : nullptr;
}

const Attendee* EventManager::findAttendee(int attendeeId) const {
    return attendeesById_.find(attendeeId);
}

const Attendee* EventManager::findAttendeeByEmail(const std::string& email) const {
    const int* id = attendeeIdByEmail_.find(normalize(email));
    return id ? attendeesById_.find(*id) : nullptr;
}

std::vector<const Event*> EventManager::events() const {
    std::vector<const Event*> out;
    events_.forEach([&out](const Event& e) { out.push_back(&e); });
    return out;
}

std::vector<Attendee> EventManager::attendees() const {
    std::vector<Attendee> out;
    attendeesById_.forEach([&out](int, const Attendee& a) { out.push_back(a); });
    std::sort(out.begin(), out.end(), [](const Attendee& a, const Attendee& b) { return a.id < b.id; });
    return out;
}

void EventManager::clear() {
    events_.clear();
    eventById_.clear();
    eventIdByName_.clear();
    attendeesById_.clear();
    attendeeIdByEmail_.clear();
    nextEventId_ = 1;
    nextAttendeeId_ = 1;
}

void EventManager::restoreAttendee(const Attendee& attendee) {
    attendeesById_.insert(attendee.id, attendee);
    attendeeIdByEmail_.insert(normalize(attendee.email), attendee.id);
    nextAttendeeId_ = std::max(nextAttendeeId_, attendee.id + 1);
}

void EventManager::restoreEvent(int id, const std::string& name, const std::string& date, const std::string& venue, int capacity) {
    Event* stored = events_.append(Event(id, name, date, venue, capacity));
    eventById_.insert(id, stored);
    eventIdByName_.insert(normalize(name), id);
    nextEventId_ = std::max(nextEventId_, id + 1);
}

void EventManager::restoreConfirmed(int eventId, int attendeeId) {
    Event* event = lookupEvent(eventId);
    if (!event) throw std::out_of_range("event not found");
    event->restoreConfirmed(attendeeId);
}

void EventManager::restoreWaitlisted(int eventId, int attendeeId) {
    Event* event = lookupEvent(eventId);
    if (!event) throw std::out_of_range("event not found");
    event->restoreWaitlisted(attendeeId);
}
