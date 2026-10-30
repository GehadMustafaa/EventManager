#include "Queue.h"
#include "TestHarness.h"

TEST(queue_fifo_order) {
    Queue<int> q;
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    CHECK(q.size() == 3);
    CHECK(q.dequeue() == 1);
    CHECK(q.dequeue() == 2);
    CHECK(q.dequeue() == 3);
    CHECK(q.empty());
}

TEST(queue_dequeue_empty_throws) {
    Queue<int> q;
    CHECK_THROWS(q.dequeue());
    CHECK_THROWS(q.front());
}

TEST(queue_remove_middle_head_tail) {
    Queue<int> q;
    for (int i = 1; i <= 4; ++i) q.enqueue(i);
    CHECK(q.remove(2));
    CHECK(q.remove(1));
    CHECK(q.remove(4));
    CHECK(!q.remove(9));
    CHECK(q.size() == 1);
    CHECK(q.front() == 3);
    q.enqueue(5);
    CHECK(q.dequeue() == 3);
    CHECK(q.dequeue() == 5);
}

TEST(queue_move_transfers_ownership) {
    Queue<int> a;
    a.enqueue(7);
    Queue<int> b(std::move(a));
    CHECK(a.empty());
    CHECK(b.size() == 1);
    CHECK(b.front() == 7);
}
