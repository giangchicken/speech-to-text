
cmake_minimum_required(VERSION 3.16)

project(test_multiple_find_package)

find_package(Qt6Core REQUIRED)

add_subdirectory(subdir1)

add_executable(exe1 main.cpp)
target_link_libraries(exe1 Qt::Core)
