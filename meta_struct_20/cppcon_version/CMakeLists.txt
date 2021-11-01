# CMakeList.txt : CMake project for tagged_sqlite, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(cpp20_tagged_struct)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)


# Add source to this project's executable.
add_executable (meta_struct_sqlite_example "meta_struct_sqlite_example.cpp" "meta_struct_sqlite.h")
target_link_libraries(meta_struct_sqlite_example SQLite::SQLite3 Threads::Threads ${CMAKE_DL_LIBS})


# Add source to this project's executable.
add_executable (fixed_string "fixed_string.cpp")
add_executable (meta_struct_1 "meta_struct_1.cpp")
add_executable (meta_struct_constructed_2 "meta_struct_constructed_2.cpp")
add_executable (meta_struct_init_3 "meta_struct_init_3.cpp")
add_executable (meta_struct_init_self_4 "meta_struct_init_self_4.cpp")
add_executable (meta_struct_print_5 "meta_struct_print_5.cpp")
add_executable (meta_struct_optional_arguments_6 "meta_struct_optional_arguments_6.cpp")
add_executable (meta_struct_required_named_args_7 "meta_struct_required_named_args_7.cpp")
add_executable (meta_struct_conversion_8 "meta_struct_conversion_8.cpp")
add_executable (meta_struct_attributes_9 "meta_struct_applications_10.cpp")
add_executable (meta_struct_applications_10 "meta_struct_applications_10.cpp")
add_executable (meta_struct_example_11 "meta_struct_example_11.cpp" "meta_struct.h")





# TODO: Add tests and instal l targets if needed.
