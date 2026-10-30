#include "EventList.h"

EventList::~EventList() {
    clear();
}

Event* EventList::append(Event&& event) {
    Node* node = new Node(std::move(event));
    if (tail_) tail_->next = node; else head_ = node;
    tail_ = node;
    ++size_;
    return &node->event;
}

Event* EventList::findById(int id) const {
    for (Node* cur = head_; cur; cur = cur->next)
        if (cur->event.id() == id) return &cur->event;
    return nullptr;
}

bool EventList::removeById(int id) {
    Node* prev = nullptr;
    for (Node* cur = head_; cur; prev = cur, cur = cur->next) {
        if (cur->event.id() == id) {
            if (prev) prev->next = cur->next; else head_ = cur->next;
            if (cur == tail_) tail_ = prev;
            delete cur;
            --size_;
            return true;
        }
    }
    return false;
}

void EventList::clear() {
    while (head_) {
        Node* next = head_->next;
        delete head_;
        head_ = next;
    }
    tail_ = nullptr;
    size_ = 0;
}
