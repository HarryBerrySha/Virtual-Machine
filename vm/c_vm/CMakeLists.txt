cmake_minimum_required(VERSION 3.10)
project(vm_c C)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_library(vm_c STATIC ${SRC})

add_executable(vm_c_example examples/main.c)
target_link_libraries(vm_c_example vm_c)
add_executable(vm_trycatch examples/trycatch.c)
target_link_libraries(vm_trycatch vm_c)
add_executable(vm_closure examples/closure.c)
target_link_libraries(vm_closure vm_c)
add_executable(vm_closure_test examples/closure_test.c)
target_link_libraries(vm_closure_test vm_c)
add_executable(vm_compiler_closure examples/compiler_closure.c)
target_link_libraries(vm_compiler_closure vm_c)

## enable CTest and register tests
include(CTest)
enable_testing()
add_test(NAME vm_closure_test COMMAND vm_closure_test)
add_test(NAME vm_trycatch COMMAND vm_trycatch)
add_test(NAME vm_c_example COMMAND vm_c_example)
add_test(NAME vm_compiler_closure COMMAND vm_compiler_closure)

# cd vm/c_vm
# mkdir build; cd build
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cmake --build . --config Release --target vm_closure_test
# ctest --output-on-failure -C Release
