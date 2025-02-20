cmake_minimum_required(VERSION 3.27)
project(RISC_V_ISA_Cache_Simulator)

set(CMAKE_CXX_STANDARD 20)

add_executable(RISC_V_ISA_Cache_Simulator
        main.cpp
        CacheConfig.cpp)
