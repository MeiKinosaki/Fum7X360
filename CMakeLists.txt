cmake_minimum_required(VERSION 3.16)
project(Fum7X360 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(fm7core
    src/cpu/m6809.cpp
)
target_include_directories(fm7core PUBLIC src)

add_executable(fum7x360 src/main.cpp)
target_link_libraries(fum7x360 PRIVATE fm7core)

enable_testing()
add_executable(fm7core_tests tests/m6809_tests.cpp)
target_link_libraries(fm7core_tests PRIVATE fm7core)
add_test(NAME fm7core_tests COMMAND fm7core_tests)

add_executable(memory_bus_tests tests/memory_bus_tests.cpp)
target_include_directories(memory_bus_tests PRIVATE src)
add_test(NAME memory_bus_tests COMMAND memory_bus_tests)

add_executable(fm7_machine_tests tests/fm7_machine_tests.cpp)
target_link_libraries(fm7_machine_tests PRIVATE fm7core)
add_test(NAME fm7_machine_tests COMMAND fm7_machine_tests)

add_executable(fm7_scheduler_tests tests/fm7_scheduler_tests.cpp)
target_link_libraries(fm7_scheduler_tests PRIVATE fm7core)
add_test(NAME fm7_scheduler_tests COMMAND fm7_scheduler_tests)
