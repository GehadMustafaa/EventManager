#pragma once
#include <optional>
#include <string>
#include <vector>
#include "HashTable.h"
#include "Queue.h"

class Event {
public:
    enum class BookResult { Confirmed, Waitlisted, Duplicate };

    struct CancelResult {
        bool cancelled;
        std::optional<int> promotedAttendeeId;
    };

    Event(int id, std::string name, std::string date, std::string venue, int capacity);

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& date() const { return date_; }
    const std::string& venue() const { return venue_; }
    int capacity() const { return capacity_; }

    int seatsAvailable() const;
    std::size_t confirmedCount() const { return confirmed_.size(); }
    std::size_t waitlistCount() const { return waitlist_.size(); }

    bool isConfirmed(int attendeeId) const;
    bool isWaitlisted(int attendeeId) const;

    BookResult book(int attendeeId);
    CancelResult cancel(int attendeeId);

    std::vector<int> confirmedIds() const;
    std::vector<int> waitlistIds() const;

    void restoreConfirmed(int attendeeId);
    void restoreWaitlisted(int attendeeId);

private:
    int id_;
    std::string name_;
    std::string date_;
    std::string venue_;
    int capacity_;
    HashTable<int, bool> confirmed_;
    Queue<int> waitlist_;
};
