#pragma once
#include <cstddef>
#include <iterator>
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
    class ConstIterator {
        const Node* cur_;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        explicit ConstIterator(const Node* cur) : cur_(cur) {}

        reference operator*() const { return cur_->value; }
        pointer operator->() const { return &cur_->value; }

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

    ConstIterator begin() const { return ConstIterator(head_); }
    ConstIterator end() const { return ConstIterator(nullptr); }

    template <typename F>
    void forEach(F fn) const {
        for (const T& v : *this) fn(v);
    }
};
