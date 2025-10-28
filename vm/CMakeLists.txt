cmake_minimum_required(VERSION 3.16)
project(vm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(vm
    src/main.cpp
    src/vm.cpp
    src/bytecode.cpp
    src/compiler.cpp
    src/disassembler.cpp
    src/verifier.cpp
)

target_include_directories(vm PRIVATE include)
