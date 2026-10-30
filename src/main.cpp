#include <filesystem>
#include <iostream>
#include "EventManager.h"
#include "Menu.h"
#include "Storage.h"

int main(int argc, char* argv[]) {
    std::string dataPath = argc > 1 ? argv[1] : "data/events.db";

    EventManager manager;
    if (std::filesystem::exists(dataPath)) {
        try {
            Storage::load(manager, dataPath);
        } catch (const std::exception& ex) {
            std::cerr << ex.what() << '\n';
            return 1;
        }
    }

    Menu menu(manager, std::cin, std::cout, dataPath);
    menu.run();
    return 0;
}
