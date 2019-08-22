# CMakeList.txt : CMake project for tagged_sqlite, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

find_package(SQLite3 REQUIRED)

# Add source to this project's executable.
add_executable (tagged_sqlite "tagged_sqlite.cpp" "tagged_sqlite.h")

target_compile_features(tagged_sqlite PUBLIC cxx_std_17)
target_link_libraries(tagged_sqlite SQLite::SQLite3)


# TODO: Add tests and install targets if needed.
