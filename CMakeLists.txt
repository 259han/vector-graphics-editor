cmake_minimum_required(VERSION 3.16)
project(vector-graphics-editor VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE SOURCES 
    "include/*.cpp"
    "include/*.h"
)

add_executable(${PROJECT_NAME} 
    main.cpp 
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    Qt6::Widgets
)