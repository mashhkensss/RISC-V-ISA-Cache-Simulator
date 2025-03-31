cmake_minimum_required(VERSION 3.27)
project(RISC_V_ISA_Cache_Simulator)

set(CMAKE_CXX_STANDARD 20)

#include_directories(${CMAKE_SOURCE_DIR}/Entities)
#include_directories(${CMAKE_SOURCE_DIR}/Parameters)
#include_directories(${CMAKE_SOURCE_DIR}/Cache)

add_subdirectory(Parameters)
add_subdirectory(Cache)
add_subdirectory(Entities)

add_executable(RISC_V_ISA_Cache_Simulator main.cpp)

target_link_libraries(RISC_V_ISA_Cache_Simulator entities cache parameters)