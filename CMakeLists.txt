cmake_minimum_required(VERSION 4.2.3)
project(Boilerplate)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-O2 -Werror -Wall -Wno-unused)

add_executable(Boilerplate src/main.cpp)