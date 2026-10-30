#pragma once
#include <cstddef>
#include "Event.h"

class EventList {
    struct Node {
        Event event;
        Node* next;
        explicit Node(Event&& e) : event(std::move(e)), next(nullptr) {}
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;

public:
    EventList() = default;
    EventList(const EventList&) = delete;
    EventList& operator=(const EventList&) = delete;
    ~EventList();

    Event* append(Event&& event);
    Event* findById(int id) const;
    bool removeById(int id);
    void clear();

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    template <typename F>
    void forEach(F fn) const {
        for (Node* cur = head_; cur; cur = cur->next) fn(static_cast<const Event&>(cur->event));
    }
};
