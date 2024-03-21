cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(modern-events VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Including CTest would prevent it from complaining of missing Dart config
include(CTest)

include(cmake/add_subdirectories.cmake)

# turn on lots of warnings
add_compile_options(-Wall -Wextra -pedantic -fno-rtti)

add_subdirectory(typed_event)

