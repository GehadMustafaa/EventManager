#include <iostream>
#include "TestHarness.h"

int main() {
    int passed = 0;
    int failed = 0;
    for (const TestCase& t : testRegistry()) {
        try {
            t.fn();
            std::cout << "[PASS] " << t.name << '\n';
            ++passed;
        } catch (const TestFailure& f) {
            std::cout << "[FAIL] " << t.name << " -> " << f.message << '\n';
            ++failed;
        } catch (const std::exception& ex) {
            std::cout << "[FAIL] " << t.name << " -> exception: " << ex.what() << '\n';
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
