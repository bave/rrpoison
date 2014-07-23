cmake_minimum_required (VERSION 2.8)
project (rrpoison)

set(RRPOISON rrpoison.cpp)
set(CMAKE_CXX_FLAGS "-g -DDEBUG -O0 -Wall -std=c++11 -Wno-unused-variable")
add_executable(rrpoison ${RRPOISON})

set(RR_A rr_a.cpp)
set(CMAKE_CXX_FLAGS "-g -DDEBUG -O0 -Wall -std=c++11 -Wno-unused-variable")
add_executable(rr_a ${RR_A})

set(NAME_PKT name_pkt.cpp)
set(CMAKE_CXX_FLAGS "-g -DDEBUG -O0 -Wall -std=c++11 -Wno-unused-variable")
add_executable(np ${NAME_PKT})
TARGET_LINK_LIBRARIES(np resolv)

set(PKT_BUFFER pkt_buffer.cpp)
set(CMAKE_CXX_FLAGS "-g -DDEBUG -O0 -Wall -std=c++11 -Wno-unused-variable")
add_executable(pb ${PKT_BUFFER})

set(NAME_BUFFER name_buffer.cpp)
set(CMAKE_CXX_FLAGS "-g -DDEBUG -O0 -Wall -std=c++11 -Wno-unused-variable")
add_executable(nb ${NAME_BUFFER})
