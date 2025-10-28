cmake_minimum_required(VERSION 3.10)
project(vm_c C)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_library(vm_c STATIC ${SRC})

add_executable(vm_c_example examples/main.c)
target_link_libraries(vm_c_example vm_c)
