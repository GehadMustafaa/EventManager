#pragma once
#include <functional>
#include <string>
#include <vector>

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

struct TestFailure {
    std::string message;
};

inline std::vector<TestCase>& testRegistry() {
    static std::vector<TestCase> registry;
    return registry;
}

struct TestRegistrar {
    TestRegistrar(std::string name, std::function<void()> fn) {
        testRegistry().push_back({std::move(name), std::move(fn)});
    }
};

#define TEST(name) \
    static void name(); \
    static TestRegistrar registrar_##name(#name, name); \
    static void name()

#define CHECK(cond) \
    do { \
        if (!(cond)) throw TestFailure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + #cond}; \
    } while (0)

#define CHECK_THROWS(expr) \
    do { \
        bool thrown = false; \
        try { expr; } catch (const std::exception&) { thrown = true; } \
        CHECK(thrown); \
    } while (0)
