#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T>
class Queue {
    struct Node {
        T value;
        Node* next;
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;

public:
    Queue() = default;
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(Queue&& other) noexcept : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = other.tail_ = nullptr;
        other.size_ = 0;
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~Queue() { clear(); }

    void enqueue(const T& value) {
        Node* node = new Node{value, nullptr};
        if (tail_) tail_->next = node; else head_ = node;
        tail_ = node;
        ++size_;
    }

    T dequeue() {
        if (!head_) throw std::underflow_error("queue is empty");
        Node* node = head_;
        T value = std::move(node->value);
        head_ = node->next;
        if (!head_) tail_ = nullptr;
        delete node;
        --size_;
        return value;
    }

    const T& front() const {
        if (!head_) throw std::underflow_error("queue is empty");
        return head_->value;
    }

    bool remove(const T& value) {
        Node* prev = nullptr;
        for (Node* cur = head_; cur; prev = cur, cur = cur->next) {
            if (cur->value == value) {
                if (prev) prev->next = cur->next; else head_ = cur->next;
                if (cur == tail_) tail_ = prev;
                delete cur;
                --size_;
                return true;
            }
        }
        return false;
    }

    bool contains(const T& value) const {
        for (Node* cur = head_; cur; cur = cur->next)
            if (cur->value == value) return true;
        return false;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        while (head_) {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    template <typename F>
    void forEach(F fn) const {
        for (Node* cur = head_; cur; cur = cur->next) fn(cur->value);
    }
};
