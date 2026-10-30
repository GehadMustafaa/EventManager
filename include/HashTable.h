#pragma once
#include <cstddef>
#include <functional>
#include <list>
#include <utility>
#include <vector>

template <typename K, typename V>
class HashTable {
    using Entry = std::pair<K, V>;

    static constexpr double kMaxLoadFactor = 0.75;

    std::vector<std::list<Entry>> buckets_;
    std::size_t size_ = 0;

    std::size_t indexFor(const K& key, std::size_t bucketCount) const {
        return std::hash<K>{}(key) % bucketCount;
    }

    void rehash(std::size_t newBucketCount) {
        std::vector<std::list<Entry>> next(newBucketCount);
        for (auto& bucket : buckets_)
            for (auto& entry : bucket)
                next[indexFor(entry.first, newBucketCount)].push_back(std::move(entry));
        buckets_ = std::move(next);
    }

public:
    explicit HashTable(std::size_t initialBuckets = 16) : buckets_(initialBuckets ? initialBuckets : 16) {}

    bool insert(const K& key, const V& value) {
        if (contains(key)) return false;
        if (static_cast<double>(size_ + 1) / static_cast<double>(buckets_.size()) > kMaxLoadFactor)
            rehash(buckets_.size() * 2);
        buckets_[indexFor(key, buckets_.size())].emplace_back(key, value);
        ++size_;
        return true;
    }

    bool erase(const K& key) {
        auto& bucket = buckets_[indexFor(key, buckets_.size())];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->first == key) {
                bucket.erase(it);
                --size_;
                return true;
            }
        }
        return false;
    }

    V* find(const K& key) {
        for (auto& entry : buckets_[indexFor(key, buckets_.size())])
            if (entry.first == key) return &entry.second;
        return nullptr;
    }

    const V* find(const K& key) const {
        for (const auto& entry : buckets_[indexFor(key, buckets_.size())])
            if (entry.first == key) return &entry.second;
        return nullptr;
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    std::size_t bucketCount() const { return buckets_.size(); }

    void clear() {
        for (auto& bucket : buckets_) bucket.clear();
        size_ = 0;
    }

    template <typename F>
    void forEach(F fn) const {
        for (const auto& bucket : buckets_)
            for (const auto& entry : bucket) fn(entry.first, entry.second);
    }
};
