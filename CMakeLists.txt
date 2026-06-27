cmake_minimum_required(VERSION 3.10)
project(riscv-sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_executable(riscv-sim
    src/main.cpp
    src/cpu.cpp
    src/memory.cpp
)
