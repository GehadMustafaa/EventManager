#pragma once
#include <sstream>
#include <string>
#include <vector>
#include "EventManager.h"
#include "Storage.h"

class SnapshotStack {
public:
    void push(const EventManager& manager) {
        std::ostringstream out;
        Storage::save(manager, out);
        stack_.push_back(out.str());
        if (stack_.size() > 25) stack_.erase(stack_.begin());
    }

    bool canUndo() const { return !stack_.empty(); }

    bool undo(EventManager& manager) {
        if (stack_.empty()) return false;
        std::istringstream in(stack_.back());
        stack_.pop_back();
        Storage::load(manager, in);
        return true;
    }

    void discardLast() {
        if (!stack_.empty()) stack_.pop_back();
    }

    void clear() { stack_.clear(); }

private:
    std::vector<std::string> stack_;
};
