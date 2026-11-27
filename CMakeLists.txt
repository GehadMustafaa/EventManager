cmake_minimum_required(VERSION 3.16)
project(SmartEventManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(sem_core
  src/Event.cpp
  src/EventList.cpp
  src/EventManager.cpp
  src/Storage.cpp
  src/Menu.cpp)
target_include_directories(sem_core PUBLIC include)
if(MSVC)
  target_compile_options(sem_core PUBLIC /W4)
else()
  target_compile_options(sem_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

option(SEM_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(SEM_SANITIZERS AND NOT MSVC)
  target_compile_options(sem_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(sem_core PUBLIC -fsanitize=address,undefined)
endif()

option(SEM_BUILD_GUI "Build the Qt GUI (sem_gui)" ON)
if(SEM_BUILD_GUI)
  find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets QUIET)
  if(QT_FOUND)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    add_executable(sem_gui WIN32 MACOSX_BUNDLE
      gui/main_gui.cpp
      gui/MainWindow.cpp
      gui/MainWindow.h
      gui/CommandRegistry.cpp
      gui/CommandRegistry.h
      gui/CommandPalette.cpp
      gui/CommandPalette.h
      gui/ActionToast.cpp
      gui/ActionToast.h
      gui/ConfirmDialog.cpp
      gui/ConfirmDialog.h
      gui/EventDialog.cpp
      gui/EventDialog.h
      gui/AttendeeDialog.cpp
      gui/AttendeeDialog.h
      gui/Delegates.cpp
      gui/Delegates.h
      gui/FormField.h
      gui/Theme.cpp
      gui/Theme.h
      gui/ThemeTokens.inc)
    set_target_properties(sem_gui PROPERTIES AUTOMOC ON)
    target_include_directories(sem_gui PRIVATE gui)
    target_link_libraries(sem_gui PRIVATE sem_core Qt${QT_VERSION_MAJOR}::Widgets)
    if(MSVC)
      target_compile_options(sem_gui PRIVATE /utf-8)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_gui.cpp")
      find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test QUIET)
      if(Qt${QT_VERSION_MAJOR}Test_FOUND)
        add_executable(sem_gui_tests
          tests/test_gui.cpp
          gui/MainWindow.cpp
          gui/MainWindow.h
          gui/CommandRegistry.cpp
          gui/CommandRegistry.h
          gui/CommandPalette.cpp
          gui/CommandPalette.h
          gui/ActionToast.cpp
          gui/ActionToast.h
          gui/ConfirmDialog.cpp
          gui/ConfirmDialog.h
          gui/EventDialog.cpp
          gui/EventDialog.h
          gui/AttendeeDialog.cpp
          gui/AttendeeDialog.h
          gui/Delegates.cpp
          gui/Delegates.h
          gui/FormField.h
          gui/Theme.cpp
          gui/Theme.h
          gui/ThemeTokens.inc)
        set_target_properties(sem_gui_tests PROPERTIES AUTOMOC ON)
        target_include_directories(sem_gui_tests PRIVATE gui)
        target_link_libraries(sem_gui_tests PRIVATE sem_core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
        add_test(NAME sem_gui_tests COMMAND sem_gui_tests)
        if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.22")
          set_tests_properties(sem_gui_tests PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt${QT_VERSION_MAJOR}::Core>;PATH=path_list_prepend:$<TARGET_FILE_DIR:sem_gui_tests>")
        endif()
      endif()
    endif()
  else()
    message(STATUS "Qt not found: sem_gui skipped")
  endif()
endif()

add_executable(sem src/main.cpp)
target_link_libraries(sem PRIVATE sem_core)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp")
  enable_testing()
  set(SEM_TEST_SOURCES
    tests/test_main.cpp
    tests/test_queue.cpp
    tests/test_hash_table.cpp
    tests/test_event_list.cpp
    tests/test_event.cpp
    tests/test_event_manager.cpp
    tests/test_storage.cpp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_menu.cpp")
    list(APPEND SEM_TEST_SOURCES tests/test_menu.cpp)
  endif()
  add_executable(sem_tests ${SEM_TEST_SOURCES})
  target_link_libraries(sem_tests PRIVATE sem_core)
  add_test(NAME sem_tests COMMAND sem_tests)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/benchmark.cpp")
  add_executable(sem_bench bench/benchmark.cpp)
  target_link_libraries(sem_bench PRIVATE sem_core)
endif()
