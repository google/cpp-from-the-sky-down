# CMakeList.txt : CMake project for tagged_sqlite, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(tagged_sqlite)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SQLite3 REQUIRED)

# Add source to this project's executable.
add_executable (example "example.cpp" "tagged_sqlite.h")
target_link_libraries(example SQLite::SQLite3)


# TODO: Add tests and instal l targets if needed.
