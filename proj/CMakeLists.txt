cmake_minimum_required(VERSION 3.28)
project(QTmap_RTS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Qt
find_package(Qt6 REQUIRED COMPONENTS Quick Location Positioning)

qt_standard_project_setup(REQUIRES 6.10)

# zewnętrzna biblioteka
find_package(simdjson REQUIRED)

# --------------------------------------------------
# SOURCES (C++ core + Qt backend)
# --------------------------------------------------
set(SOURCES
        src/main.cpp
        src/qt/Backend.cpp
        src/qt/EventReceiver.cpp
        src/qt/EventReceiver.h
)

set(HEADERS
        src/qt/Backend.h
        src/qt/EventReceiver.cpp
        src/qt/EventReceiver.h
)

# --------------------------------------------------
# EXECUTABLE (JEDEN)
# --------------------------------------------------
qt_add_executable(appQTmap
        ${SOURCES}
        ${HEADERS}
)

# --------------------------------------------------
# QML MODULE
# --------------------------------------------------
qt_add_qml_module(appQTmap
        URI QTmap
        VERSION 1.0
        QML_FILES
        qml/Main.qml
)

# --------------------------------------------------
# C++20 MODULES (RTS SYSTEM)
# --------------------------------------------------
target_sources(appQTmap
        PRIVATE
        FILE_SET CXX_MODULES FILES

        src/core/time/time.ixx
        src/core/task/task.ixx
        src/core/event/event.ixx
        src/core/queue/queue.ixx
        src/core/scheduler/scheduler.ixx
        src/core/eventData/eventData.ixx

        src/app/context/context.ixx
        src/app/tasks/tasks.ixx
        src/app/astar/astar.ixx
        src/app/navigationController/navigationController.ixx

        src/system/config/config.ixx
        src/system/nav_graph/nav_graph.ixx

        src/socket/eventReceiver/eventReceiver.ixx
)

# --------------------------------------------------
# LINKING
# --------------------------------------------------
target_link_libraries(appQTmap
        PRIVATE
        Qt6::Quick
        Qt6::Location
        Qt6::Positioning
        simdjson::simdjson
)