cmake_minimum_required(VERSION 3.16)
project(tsp_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(tsp_sim
        main.cpp
        graph.h
        graph.cpp
        mainwindow.h
        mainwindow.cpp
)

target_link_libraries(tsp_sim Qt6::Widgets)