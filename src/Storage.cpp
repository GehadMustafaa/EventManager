#include "Storage.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string part;
    while (std::getline(ss, part, '|')) parts.push_back(part);
    return parts;
}

void requireFields(const std::vector<std::string>& parts, std::size_t count) {
    if (parts.size() != count) throw std::runtime_error("malformed record");
}

}

void Storage::save(const EventManager& manager, const std::string& path) {
    std::ofstream file(path);
    if (!file) throw std::runtime_error("cannot open file for writing: " + path);

    for (const Attendee& a : manager.attendees())
        file << "A|" << a.id << '|' << a.name << '|' << a.email << '\n';

    for (const Event* e : manager.events()) {
        file << "E|" << e->id() << '|' << e->name() << '|' << e->date() << '|' << e->venue() << '|' << e->capacity() << '\n';
        for (int id : e->confirmedIds()) file << "C|" << e->id() << '|' << id << '\n';
        for (int id : e->waitlistIds()) file << "W|" << e->id() << '|' << id << '\n';
    }

    if (!file) throw std::runtime_error("write failed: " + path);
}

void Storage::load(EventManager& manager, const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open file for reading: " + path);

    manager.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> p = split(line);
        const std::string& tag = p[0];
        try {
            if (tag == "A") {
                requireFields(p, 4);
                manager.restoreAttendee(Attendee{std::stoi(p[1]), p[2], p[3]});
            } else if (tag == "E") {
                requireFields(p, 6);
                manager.restoreEvent(std::stoi(p[1]), p[2], p[3], p[4], std::stoi(p[5]));
            } else if (tag == "C") {
                requireFields(p, 3);
                manager.restoreConfirmed(std::stoi(p[1]), std::stoi(p[2]));
            } else if (tag == "W") {
                requireFields(p, 3);
                manager.restoreWaitlisted(std::stoi(p[1]), std::stoi(p[2]));
            } else {
                throw std::runtime_error("unknown record type");
            }
        } catch (const std::exception& ex) {
            manager.clear();
            throw std::runtime_error(std::string("load failed at '") + line + "': " + ex.what());
        }
    }
}
