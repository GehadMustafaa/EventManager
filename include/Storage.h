#pragma once
#include <string>
#include "EventManager.h"

class Storage {
public:
    static void save(const EventManager& manager, const std::string& path);
    static void load(EventManager& manager, const std::string& path);
};
