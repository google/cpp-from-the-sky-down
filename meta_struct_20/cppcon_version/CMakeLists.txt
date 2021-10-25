# CMakeList.txt : CMake project for tagged_sqlite, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(cpp20_tagged_struct)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)


# Add source to this project's executable.
add_executable (fixed_string "fixed_string.cpp")
add_executable (tagged_tuple_1 "tagged_tuple_1.cpp")
add_executable (tagged_tuple_constructed_2 "tagged_tuple_constructed_2.cpp")





# TODO: Add tests and instal l targets if needed.
