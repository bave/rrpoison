cmake_minimum_required (VERSION 2.8)
project (rrpoison)

set(RR_A rr_a.cpp)
set(CMAKE_CXX_FLAGS "-g -O0 -Wall -std=c++11 -Wno-unused-variable")
add_executable(rr_a ${RR_A})
