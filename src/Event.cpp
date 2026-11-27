#include "Event.h"

Event::Event(int id, std::string name, std::string date, std::string venue, int capacity,
             std::string description, std::string time)
    : id_(id),
      name_(std::move(name)),
      date_(std::move(date)),
      time_(std::move(time)),
      venue_(std::move(venue)),
      capacity_(capacity),
      description_(std::move(description)) {}

int Event::seatsAvailable() const {
  return capacity_ - static_cast<int>(confirmed_.size());
}

bool Event::isConfirmed(int attendeeId) const {
  return confirmed_.contains(attendeeId);
}

bool Event::isWaitlisted(int attendeeId) const {
  return waitlistSet_.contains(attendeeId);
}

Event::BookResult Event::book(int attendeeId) {
  if (isConfirmed(attendeeId) || isWaitlisted(attendeeId))
    return BookResult::Duplicate;
  if (seatsAvailable() > 0) {
    confirmed_.insert(attendeeId, true);
    return BookResult::Confirmed;
  }
  waitlist_.enqueue(attendeeId);
  waitlistSet_.insert(attendeeId, true);
  return BookResult::Waitlisted;
}

Event::CancelResult Event::cancel(int attendeeId) {
  if (confirmed_.erase(attendeeId)) {
    if (!waitlist_.empty()) {
      int promoted = waitlist_.dequeue();
      waitlistSet_.erase(promoted);
      confirmed_.insert(promoted, true);
      return {true, promoted};
    }
    return {true, std::nullopt};
  }
  if (waitlist_.remove(attendeeId)) {
    waitlistSet_.erase(attendeeId);
    return {true, std::nullopt};
  }
  return {false, std::nullopt};
}

std::vector<int> Event::updateDetails(std::string name, std::string date,
                                      std::string venue, int capacity,
                                      std::string description, std::string time) {
    if (capacity < static_cast<int>(confirmed_.size()))
        throw std::invalid_argument("capacity cannot be less than confirmed bookings");
    name_ = std::move(name);
    date_ = std::move(date);
    time_ = std::move(time);
    venue_ = std::move(venue);
    capacity_ = capacity;
    description_ = std::move(description);

  std::vector<int> promoted;
  while (seatsAvailable() > 0 && !waitlist_.empty()) {
    int id = waitlist_.dequeue();
    waitlistSet_.erase(id);
    confirmed_.insert(id, true);
    promoted.push_back(id);
  }
  return promoted;
}

std::vector<int> Event::confirmedIds() const {
  std::vector<int> ids;
  confirmed_.forEach([&ids](int id, bool) { ids.push_back(id); });
  return ids;
}

std::vector<int> Event::waitlistIds() const {
  std::vector<int> ids;
  waitlist_.forEach([&ids](int id) { ids.push_back(id); });
  return ids;
}

void Event::restoreConfirmed(int attendeeId) {
  confirmed_.insert(attendeeId, true);
}

void Event::restoreWaitlisted(int attendeeId) {
  waitlist_.enqueue(attendeeId);
  waitlistSet_.insert(attendeeId, true);
}
