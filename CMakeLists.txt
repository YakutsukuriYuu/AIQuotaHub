cmake_minimum_required(VERSION 3.21)

project(AIQuotaHub VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

qt_add_executable(AIQuotaHub MACOSX_BUNDLE
    src/main.cpp
)

target_link_libraries(AIQuotaHub PRIVATE
    Qt6::Widgets
    Qt6::Network
)
