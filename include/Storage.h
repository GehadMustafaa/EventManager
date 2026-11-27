#pragma once
#include <iosfwd>
#include <string>
#include "EventManager.h"

class Storage {
public:
    static void save(const EventManager& manager, const std::string& path);
    static void load(EventManager& manager, const std::string& path);
    static void save(const EventManager& manager, std::ostream& out);
    static void load(EventManager& manager, std::istream& in);
    static void exportCsv(const EventManager& manager, const std::string& path);
    static std::string backupPath(const std::string& path);
    static bool hasBackup(const std::string& path);
    static void restoreBackup(EventManager& manager, const std::string& path);
};
