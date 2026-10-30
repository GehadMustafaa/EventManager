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

add_executable(sem src/main.cpp)
target_link_libraries(sem PRIVATE sem_core)

add_executable(sem_tests
  tests/test_main.cpp
  tests/test_queue.cpp
  tests/test_hash_table.cpp
  tests/test_event_list.cpp
  tests/test_event.cpp
  tests/test_event_manager.cpp
  tests/test_storage.cpp)
target_link_libraries(sem_tests PRIVATE sem_core)

add_executable(sem_bench bench/benchmark.cpp)
target_link_libraries(sem_bench PRIVATE sem_core)

option(SEM_BUILD_GUI "Build the Qt GUI (sem_gui)" ON)
if(SEM_BUILD_GUI)
  find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets QUIET)
  if(QT_FOUND)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    set(CMAKE_AUTOMOC ON)
    add_executable(sem_gui
      gui/main_gui.cpp
      gui/MainWindow.cpp
      gui/MainWindow.h
      gui/EventDialog.cpp
      gui/EventDialog.h)
    target_include_directories(sem_gui PRIVATE gui)
    target_link_libraries(sem_gui PRIVATE sem_core Qt${QT_VERSION_MAJOR}::Widgets)
  else()
    message(STATUS "Qt not found: sem_gui skipped")
  endif()
endif()

enable_testing()
add_test(NAME sem_tests COMMAND sem_tests)
