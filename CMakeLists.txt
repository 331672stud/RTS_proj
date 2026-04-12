cmake_minimum_required(VERSION 3.28)
project(RTSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(rts_proj
    src/main.cpp)

target_sources(rts_proj
    PRIVATE
        src/main.cpp

    PRIVATE
        FILE_SET CXX_MODULES FILES

        src/core/time/time.ixx
        src/core/task/task.ixx
        src/core/event/event.ixx
        src/core/queue/queue.ixx
        src/core/scheduler/scheduler.ixx

        src/app/context/context.ixx
        src/app/dispatcher/dispatcher.ixx
        src/app/tasks/tasks.ixx

        src/system/config/config.ixx
)