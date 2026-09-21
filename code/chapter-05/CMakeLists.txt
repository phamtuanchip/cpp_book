cmake_minimum_required(VERSION 3.20)
project(chapter04 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(check_env main.cpp)
target_compile_options(check_env PRIVATE -Wall -Wextra)
