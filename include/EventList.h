#pragma once
#include <cstddef>
#include <iterator>
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
    class ConstIterator {
        const Node* cur_;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Event;
        using difference_type = std::ptrdiff_t;
        using pointer = const Event*;
        using reference = const Event&;

        explicit ConstIterator(const Node* cur) : cur_(cur) {}

        reference operator*() const { return cur_->event; }
        pointer operator->() const { return &cur_->event; }

        ConstIterator& operator++() {
            cur_ = cur_->next;
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            cur_ = cur_->next;
            return tmp;
        }

        bool operator==(const ConstIterator& other) const { return cur_ == other.cur_; }
        bool operator!=(const ConstIterator& other) const { return cur_ != other.cur_; }
    };

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

    ConstIterator begin() const { return ConstIterator(head_); }
    ConstIterator end() const { return ConstIterator(nullptr); }

    template <typename F>
    void forEach(F fn) const {
        for (const Event& e : *this) fn(e);
    }
};
