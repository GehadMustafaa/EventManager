#pragma once
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <utility>
#include <vector>

template <typename K, typename V>
class HashTable {
    using Entry = std::pair<K, V>;
    using BucketList = std::vector<std::list<Entry>>;

    static constexpr double kMaxLoadFactor = 0.75;

    BucketList buckets_;
    std::size_t size_ = 0;

    std::size_t indexFor(const K& key, std::size_t bucketCount) const {
        return std::hash<K>{}(key) % bucketCount;
    }

    void rehash(std::size_t newBucketCount) {
        BucketList next(newBucketCount);
        for (auto& bucket : buckets_)
            for (auto& entry : bucket)
                next[indexFor(entry.first, newBucketCount)].push_back(std::move(entry));
        buckets_ = std::move(next);
    }

public:
    class ConstIterator {
        using BucketIter = typename BucketList::const_iterator;
        using EntryIter = typename std::list<Entry>::const_iterator;

        BucketIter bucket_;
        BucketIter bucketEnd_;
        EntryIter entry_;

        void advanceToValid() {
            while (bucket_ != bucketEnd_ && entry_ == bucket_->end()) {
                ++bucket_;
                if (bucket_ != bucketEnd_) entry_ = bucket_->begin();
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Entry;
        using difference_type = std::ptrdiff_t;
        using pointer = const Entry*;
        using reference = const Entry&;

        ConstIterator(BucketIter bucket, BucketIter bucketEnd, EntryIter entry)
            : bucket_(bucket), bucketEnd_(bucketEnd), entry_(entry) {
            advanceToValid();
        }

        reference operator*() const { return *entry_; }
        pointer operator->() const { return &(*entry_); }

        ConstIterator& operator++() {
            ++entry_;
            advanceToValid();
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const ConstIterator& other) const {
            if (bucket_ != other.bucket_) return false;
            return bucket_ == bucketEnd_ || entry_ == other.entry_;
        }

        bool operator!=(const ConstIterator& other) const { return !(*this == other); }
    };

    explicit HashTable(std::size_t initialBuckets = 16) : buckets_(initialBuckets ? initialBuckets : 16) {}

    bool insert(const K& key, const V& value) {
        if (contains(key)) return false;
        if (static_cast<double>(size_ + 1) / static_cast<double>(buckets_.size()) > kMaxLoadFactor)
            rehash(buckets_.size() * 2);
        buckets_[indexFor(key, buckets_.size())].emplace_back(key, value);
        ++size_;
        return true;
    }

    bool insertOrAssign(const K& key, const V& value) {
        if (V* existing = find(key)) {
            *existing = value;
            return false;
        }
        insert(key, value);
        return true;
    }

    V& operator[](const K& key) {
        if (V* existing = find(key)) return *existing;
        if (static_cast<double>(size_ + 1) / static_cast<double>(buckets_.size()) > kMaxLoadFactor)
            rehash(buckets_.size() * 2);
        auto& bucket = buckets_[indexFor(key, buckets_.size())];
        bucket.emplace_back(key, V{});
        ++size_;
        return bucket.back().second;
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

    ConstIterator begin() const {
        return ConstIterator(buckets_.begin(), buckets_.end(),
                             buckets_.empty() ? typename std::list<Entry>::const_iterator{} : buckets_.front().begin());
    }

    ConstIterator end() const {
        return ConstIterator(buckets_.end(), buckets_.end(), typename std::list<Entry>::const_iterator{});
    }

    template <typename F>
    void forEach(F fn) const {
        for (const auto& entry : *this) fn(entry.first, entry.second);
    }
};
