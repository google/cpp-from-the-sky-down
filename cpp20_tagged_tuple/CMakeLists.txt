# CMakeList.txt : CMake project for tagged_sqlite, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(cpp20_tagged_struct)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(GTest CONFIG REQUIRED)
find_package(Threads REQUIRED)


# Add source to this project's executable.
add_executable (tagged_tuple_test "tagged_tuple_test.cpp" "tagged_tuple.h")
target_link_libraries(tagged_tuple_test PRIVATE GTest::gtest GTest::gtest_main)

add_executable (example "example.cpp" "tagged_tuple.h")





# TODO: Add tests and instal l targets if needed.
