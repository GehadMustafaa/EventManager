#pragma once
#include <iosfwd>
#include <string>
#include "EventManager.h"

class Menu {
public:
    Menu(EventManager& manager, std::istream& in, std::ostream& out, std::string dataPath);
    void run();

private:
    std::string prompt(const std::string& label);
    int promptInt(const std::string& label);
    std::string attendeeLabel(int attendeeId) const;
    void printMenu();
    bool dispatch(const std::string& choice);

    void createEvent();
    void listEvents();
    void registerAttendee();
    void bookTicket();
    void cancelBooking();
    void showEvent();
    void searchEvent();
    void searchAttendee();
    void deleteEvent();
    void saveData();
    void loadData();

    EventManager& manager_;
    std::istream& in_;
    std::ostream& out_;
    std::string dataPath_;
};
