cmake_minimum_required(VERSION 3.16)

project(oopd_a3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pandemic src/main.cpp src/sweep.cpp)

target_include_directories(pandemic PRIVATE include)

target_compile_options(pandemic PRIVATE -Wall -Wextra -O2)