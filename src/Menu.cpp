#include "Menu.h"
#include <algorithm>
#include <exception>
#include <iostream>
#include "Storage.h"

Menu::Menu(EventManager& manager, std::istream& in, std::ostream& out, std::string dataPath)
    : manager_(manager), in_(in), out_(out), dataPath_(std::move(dataPath)) {}

std::string Menu::prompt(const std::string& label) {
    out_ << label << ": ";
    std::string line;
    std::getline(in_, line);
    return line;
}

int Menu::promptInt(const std::string& label) {
    std::string text = prompt(label);
    try {
        return std::stoi(text);
    } catch (const std::exception&) {
        throw std::invalid_argument("'" + text + "' is not a valid number");
    }
}

std::string Menu::attendeeLabel(int attendeeId) const {
    const Attendee* a = manager_.findAttendee(attendeeId);
    return a ? "#" + std::to_string(a->id) + " " + a->name + " <" + a->email + ">" : "#" + std::to_string(attendeeId);
}

void Menu::printMenu() {
    out_ << "\n=== Smart Event Management ===\n"
         << " 1. Create event\n"
         << " 2. List events\n"
         << " 3. Register attendee\n"
         << " 4. Book ticket\n"
         << " 5. Cancel booking\n"
         << " 6. Event details\n"
         << " 7. Search event by name\n"
         << " 8. Search attendee by email\n"
         << " 9. Delete event\n"
         << "10. Save\n"
         << "11. Load\n"
         << " 0. Exit\n";
}

void Menu::run() {
    for (;;) {
        printMenu();
        std::string choice = prompt("Choice");
        if (!in_ || choice == "0") return;
        try {
            if (!dispatch(choice)) out_ << "Unknown option\n";
        } catch (const std::exception& ex) {
            out_ << "Error: " << ex.what() << '\n';
        }
    }
}

bool Menu::dispatch(const std::string& choice) {
    if (choice == "1") createEvent();
    else if (choice == "2") listEvents();
    else if (choice == "3") registerAttendee();
    else if (choice == "4") bookTicket();
    else if (choice == "5") cancelBooking();
    else if (choice == "6") showEvent();
    else if (choice == "7") searchEvent();
    else if (choice == "8") searchAttendee();
    else if (choice == "9") deleteEvent();
    else if (choice == "10") saveData();
    else if (choice == "11") loadData();
    else return false;
    return true;
}

void Menu::createEvent() {
    std::string name = prompt("Name");
    std::string date = prompt("Date (YYYY-MM-DD)");
    std::string venue = prompt("Venue");
    int capacity = promptInt("Capacity");
    out_ << "Created event #" << manager_.createEvent(name, date, venue, capacity) << '\n';
}

void Menu::listEvents() {
    auto events = manager_.events();
    if (events.empty()) {
        out_ << "No events\n";
        return;
    }
    for (const Event* e : events)
        out_ << '#' << e->id() << ' ' << e->name() << " | " << e->date() << " | " << e->venue()
             << " | seats " << e->seatsAvailable() << '/' << e->capacity()
             << " | waitlist " << e->waitlistCount() << '\n';
}

void Menu::registerAttendee() {
    std::string name = prompt("Name");
    std::string email = prompt("Email");
    out_ << "Registered attendee #" << manager_.registerAttendee(name, email) << '\n';
}

void Menu::bookTicket() {
    int eventId = promptInt("Event ID");
    int attendeeId = promptInt("Attendee ID");
    switch (manager_.book(eventId, attendeeId)) {
        case Event::BookResult::Confirmed: out_ << "Booking confirmed\n"; break;
        case Event::BookResult::Waitlisted: out_ << "Event full: added to waiting list\n"; break;
        case Event::BookResult::Duplicate: out_ << "Attendee already booked or waitlisted\n"; break;
    }
}

void Menu::cancelBooking() {
    int eventId = promptInt("Event ID");
    int attendeeId = promptInt("Attendee ID");
    Event::CancelResult result = manager_.cancel(eventId, attendeeId);
    if (!result.cancelled) {
        out_ << "No booking found\n";
        return;
    }
    out_ << "Booking cancelled\n";
    if (result.promotedAttendeeId) out_ << "Promoted from waiting list: " << attendeeLabel(*result.promotedAttendeeId) << '\n';
}

void Menu::showEvent() {
    const Event* e = manager_.findEvent(promptInt("Event ID"));
    if (!e) {
        out_ << "Event not found\n";
        return;
    }
    out_ << '#' << e->id() << ' ' << e->name() << " | " << e->date() << " | " << e->venue() << '\n'
         << "Seats: " << e->seatsAvailable() << " available of " << e->capacity() << '\n'
         << "Confirmed:\n";
    std::vector<int> confirmed = e->confirmedIds();
    std::sort(confirmed.begin(), confirmed.end());
    for (int id : confirmed) out_ << "  " << attendeeLabel(id) << '\n';
    out_ << "Waiting list:\n";
    int position = 1;
    for (int id : e->waitlistIds()) out_ << "  " << position++ << ". " << attendeeLabel(id) << '\n';
}

void Menu::searchEvent() {
    const Event* e = manager_.findEventByName(prompt("Event name"));
    if (!e) out_ << "Event not found\n";
    else out_ << '#' << e->id() << ' ' << e->name() << " | " << e->date() << " | " << e->venue() << '\n';
}

void Menu::searchAttendee() {
    const Attendee* a = manager_.findAttendeeByEmail(prompt("Email"));
    if (!a) out_ << "Attendee not found\n";
    else out_ << attendeeLabel(a->id) << '\n';
}

void Menu::deleteEvent() {
    out_ << (manager_.deleteEvent(promptInt("Event ID")) ? "Event deleted\n" : "Event not found\n");
}

void Menu::saveData() {
    Storage::save(manager_, dataPath_);
    out_ << "Saved to " << dataPath_ << '\n';
}

void Menu::loadData() {
    Storage::load(manager_, dataPath_);
    out_ << "Loaded from " << dataPath_ << '\n';
}
