#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include "EventList.h"
#include "HashTable.h"
#include "Queue.h"

namespace {

using Clock = std::chrono::steady_clock;

double microsecondsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::micro>(Clock::now() - start).count();
}

}

int main() {
    constexpr int kLookups = 1000;
    std::cout << std::left << std::setw(10) << "N"
              << std::setw(22) << "list find (us/op)"
              << std::setw(22) << "hash find (us/op)"
              << std::setw(22) << "queue enq+deq (us/op)" << '\n';

    for (int n : {1000, 10000, 100000}) {
        EventList list;
        HashTable<int, int> table;
        Queue<int> queue;
        for (int i = 1; i <= n; ++i) {
            list.append(Event(i, "e" + std::to_string(i), "2026-01-01", "V", 1));
            table.insert(i, i);
        }

        std::size_t found = 0;

        auto t0 = Clock::now();
        for (int i = 0; i < kLookups; ++i) found += list.findById(1 + (i * 7919) % n) != nullptr;
        double listUs = microsecondsSince(t0) / kLookups;

        t0 = Clock::now();
        for (int i = 0; i < kLookups; ++i) found += table.find(1 + (i * 7919) % n) != nullptr;
        double hashUs = microsecondsSince(t0) / kLookups;

        t0 = Clock::now();
        for (int i = 0; i < n; ++i) queue.enqueue(i);
        for (int i = 0; i < n; ++i) queue.dequeue();
        double queueUs = microsecondsSince(t0) / n;

        if (found != 2u * kLookups) std::cerr << "lookup mismatch\n";
        std::cout << std::left << std::setw(10) << n << std::fixed << std::setprecision(5)
                  << std::setw(22) << listUs << std::setw(22) << hashUs << std::setw(22) << queueUs << '\n';
    }
    return 0;
}
