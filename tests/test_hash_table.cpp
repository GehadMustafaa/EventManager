#include <string>
#include "HashTable.h"
#include "TestHarness.h"

TEST(hash_insert_find_erase) {
    HashTable<int, std::string> t;
    CHECK(t.insert(1, "a"));
    CHECK(!t.insert(1, "b"));
    CHECK(*t.find(1) == "a");
    CHECK(t.erase(1));
    CHECK(!t.erase(1));
    CHECK(t.find(1) == nullptr);
    CHECK(t.empty());
}

TEST(hash_rehash_preserves_entries) {
    HashTable<int, int> t(4);
    for (int i = 0; i < 1000; ++i) CHECK(t.insert(i, i * 2));
    CHECK(t.size() == 1000);
    CHECK(t.bucketCount() > 4);
    for (int i = 0; i < 1000; ++i) CHECK(*t.find(i) == i * 2);
}

TEST(hash_clear_resets_size) {
    HashTable<std::string, int> t;
    t.insert("x", 1);
    t.insert("y", 2);
    t.clear();
    CHECK(t.size() == 0);
    CHECK(!t.contains("x"));
}
