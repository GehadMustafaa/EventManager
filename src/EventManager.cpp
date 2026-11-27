#include "EventManager.h"
#include "DateUtil.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {

std::string normalize(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return out;
}

std::string trim(const std::string &s) {
  std::size_t start = 0;
  while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
    ++start;
  std::size_t end = s.size();
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
    --end;
  return s.substr(start, end - start);
}

void requireText(const std::string &value, const char *field) {
  if (trim(value).empty())
    throw std::invalid_argument(std::string(field) + " must not be empty");
  if (value.find('|') != std::string::npos ||
      value.find('\n') != std::string::npos)
    throw std::invalid_argument(std::string(field) +
                                " contains an invalid character");
}

void requireValidEmail(const std::string &email) {
  requireText(email, "attendee email");
  const std::string trimmed = trim(email);
  const std::size_t at = trimmed.find('@');
  if (at == std::string::npos || at == 0 || at + 1 >= trimmed.size() ||
      trimmed.find(' ') != std::string::npos)
    throw std::invalid_argument(
        "attendee email must look like name@example.com");
} // namespace

} // namespace

Event *EventManager::lookupEvent(int eventId) const {
  Event *const *slot = eventById_.find(eventId);
  return slot ? *slot : nullptr;
}

int EventManager::createEvent(const std::string &name, const std::string &date,
                              const std::string &venue, int capacity,
                              const std::string &description,
                              const std::string &time) {
  const std::string cleanName = trim(name);
  const std::string cleanDate = trim(date);
  const std::string cleanVenue = trim(venue);
  requireText(cleanName, "event name");
  requireText(cleanDate, "event date");
  DateUtil::requireValidDate(cleanDate);
  const std::string cleanTime = DateUtil::requireValidTime(time);
  requireText(cleanVenue, "event venue");
  if (capacity <= 0)
    throw std::invalid_argument("capacity must be positive");
  if (eventIdByName_.contains(normalize(cleanName)))
    throw std::invalid_argument("event name already exists");

  int id = nextEventId_++;
  restoreEvent(id, cleanName, cleanDate, cleanVenue, capacity, description,
               cleanTime);
  return id;
}

std::vector<int> EventManager::updateEvent(int eventId, const std::string &name,
                                           const std::string &date,
                                           const std::string &venue,
                                           int capacity,
                                           const std::string &description,
                                           const std::string &time) {
  Event *event = lookupEvent(eventId);
  if (!event)
    throw std::out_of_range("event not found");
  const std::string cleanName = trim(name);
  const std::string cleanDate = trim(date);
  const std::string cleanVenue = trim(venue);
  requireText(cleanName, "event name");
  requireText(cleanDate, "event date");
  DateUtil::requireValidDate(cleanDate);
  const std::string cleanTime = DateUtil::requireValidTime(time);
  requireText(cleanVenue, "event venue");
  if (capacity <= 0)
    throw std::invalid_argument("capacity must be positive");

  const std::string oldKey = normalize(event->name());
  const std::string newKey = normalize(cleanName);
  if (oldKey != newKey && eventIdByName_.contains(newKey))
    throw std::invalid_argument("event name already exists");

  auto promoted = event->updateDetails(cleanName, cleanDate, cleanVenue,
                                       capacity, description, cleanTime);
  if (oldKey != newKey) {
    eventIdByName_.erase(oldKey);
    eventIdByName_.insert(newKey, eventId);
  }
  return promoted;
}

bool EventManager::deleteEvent(int eventId) {
  Event *event = lookupEvent(eventId);
  if (!event)
    return false;
  eventIdByName_.erase(normalize(event->name()));
  eventById_.erase(eventId);
  return events_.removeById(eventId);
}

int EventManager::registerAttendee(const std::string &name,
                                   const std::string &email) {
  const std::string cleanName = trim(name);
  const std::string cleanEmail = trim(email);
  requireText(cleanName, "attendee name");
  requireValidEmail(cleanEmail);
  if (attendeeIdByEmail_.contains(normalize(cleanEmail)))
    throw std::invalid_argument("email already registered");

  int id = nextAttendeeId_++;
  restoreAttendee(Attendee{id, cleanName, cleanEmail});
  return id;
}

void EventManager::updateAttendee(int attendeeId, const std::string &name,
                                  const std::string &email) {
  Attendee *attendee = attendeesById_.find(attendeeId);
  if (!attendee)
    throw std::out_of_range("attendee not found");
  const std::string cleanName = trim(name);
  const std::string cleanEmail = trim(email);
  requireText(cleanName, "attendee name");
  requireValidEmail(cleanEmail);

  const std::string oldKey = normalize(attendee->email);
  const std::string newKey = normalize(cleanEmail);
  if (oldKey != newKey && attendeeIdByEmail_.contains(newKey))
    throw std::invalid_argument("email already registered");

  attendee->name = cleanName;
  attendee->email = cleanEmail;
  if (oldKey != newKey) {
    attendeeIdByEmail_.erase(oldKey);
    attendeeIdByEmail_.insert(newKey, attendeeId);
  }
}

bool EventManager::deleteAttendee(int attendeeId) {
  Attendee *attendee = attendeesById_.find(attendeeId);
  if (!attendee)
    return false;

  for (const Event &e : events_) {
    Event *mutableEvent = lookupEvent(e.id());
    if (mutableEvent)
      mutableEvent->cancel(attendeeId);
  }
  attendeeIdByEmail_.erase(normalize(attendee->email));
  return attendeesById_.erase(attendeeId);
}

Event::BookResult EventManager::book(int eventId, int attendeeId) {
  Event *event = lookupEvent(eventId);
  if (!event)
    throw std::out_of_range("event not found");
  if (!attendeesById_.contains(attendeeId))
    throw std::out_of_range("attendee not found");
  return event->book(attendeeId);
}

Event::CancelResult EventManager::cancel(int eventId, int attendeeId) {
  Event *event = lookupEvent(eventId);
  if (!event)
    throw std::out_of_range("event not found");
  if (!attendeesById_.contains(attendeeId))
    throw std::out_of_range("attendee not found");
  return event->cancel(attendeeId);
}

const Event *EventManager::findEvent(int eventId) const {
  return lookupEvent(eventId);
}

const Event *EventManager::findEventByName(const std::string &name) const {
  const int *id = eventIdByName_.find(normalize(name));
  return id ? lookupEvent(*id) : nullptr;
}

const Attendee *EventManager::findAttendee(int attendeeId) const {
  return attendeesById_.find(attendeeId);
}

const Attendee *
EventManager::findAttendeeByEmail(const std::string &email) const {
  const int *id = attendeeIdByEmail_.find(normalize(email));
  return id ? attendeesById_.find(*id) : nullptr;
}

std::vector<const Event *> EventManager::events() const {
  std::vector<const Event *> out;
  for (const Event &e : events_)
    out.push_back(&e);
  return out;
}

std::vector<const Event *> EventManager::eventsSorted(SortField field,
                                                      SortOrder order) const {
  std::vector<const Event *> out = events();
  auto cmpLess = [field](const Event *a, const Event *b) {
    switch (field) {
    case SortField::Id:
      return a->id() < b->id();
    case SortField::Name:
      return normalize(a->name()) < normalize(b->name());
    case SortField::Date:
      if (a->date() != b->date())
        return a->date() < b->date();
      return DateUtil::timeToMinutes(a->time()) <
             DateUtil::timeToMinutes(b->time());
    case SortField::Capacity:
      return a->capacity() < b->capacity();
    case SortField::Booked:
      return a->confirmedCount() < b->confirmedCount();
    }
    return false;
  };
  std::stable_sort(out.begin(), out.end(),
                   [order, &cmpLess](const Event *a, const Event *b) {
                     return order == SortOrder::Ascending ? cmpLess(a, b)
                                                          : cmpLess(b, a);
                   });
  return out;
}

std::vector<Attendee> EventManager::attendees() const {
  std::vector<Attendee> out;
  attendeesById_.forEach([&out](int, const Attendee &a) { out.push_back(a); });
  std::sort(out.begin(), out.end(),
            [](const Attendee &a, const Attendee &b) { return a.id < b.id; });
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

void EventManager::restoreAttendee(const Attendee &attendee) {
  attendeesById_.insert(attendee.id, attendee);
  attendeeIdByEmail_.insert(normalize(attendee.email), attendee.id);
  nextAttendeeId_ = std::max(nextAttendeeId_, attendee.id + 1);
}

void EventManager::restoreEvent(int id, const std::string &name,
                                const std::string &date,
                                const std::string &venue, int capacity,
                                const std::string &description,
                                const std::string &time) {
  Event *stored =
      events_.append(Event(id, name, date, venue, capacity, description, time));
  eventById_.insert(id, stored);
  eventIdByName_.insert(normalize(name), id);
  nextEventId_ = std::max(nextEventId_, id + 1);
}

void EventManager::restoreConfirmed(int eventId, int attendeeId) {
  Event *event = lookupEvent(eventId);
  if (!event)
    throw std::out_of_range("event not found");
  event->restoreConfirmed(attendeeId);
}

void EventManager::restoreWaitlisted(int eventId, int attendeeId) {
  Event *event = lookupEvent(eventId);
  if (!event)
    throw std::out_of_range("event not found");
  event->restoreWaitlisted(attendeeId);
}
