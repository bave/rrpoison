cmake_minimum_required (VERSION 2.8)
project (rrpoison)

macro(check_for_cxx11_compiler _VAR)

    set(${_VAR} 0)

    if( (MSVC AND (MSVC10 OR MSVC11 OR MSVC12)) OR
        (CMAKE_COMPILER_IS_GNUCXX AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.6) OR
        (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.1))

        set(${_VAR} 1)
        message(STATUS "Checking for C++11 compiler - available")

    else()

        message(STATUS "Checking for C++11 compiler - unavailable")

    endif()

endmacro()

check_for_cxx11_compiler(CXX11_COMPILER)

set(RRPOISON rrpoison.cpp)
set(NAME_PKT name_pkt.cpp)
set(IOPREY ioprey.cpp)
set(WROGGI wroggi.cpp)
#set(PKT_BUFFER pkt_buffer.cpp)
#set(NAME_BUFFER name_buffer.cpp)

if(CXX11_COMPILER)

    IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

        set(CMAKE_CXX_FLAGS "-g -DDEBUG -O0 -Wall -std=c++11 -Wno-unused-variable")
        add_executable(rrpoison ${RRPOISON})
        add_executable(np ${NAME_PKT})
        #add_executable(pb ${PKT_BUFFER})
        #add_executable(nb ${NAME_BUFFER})
        add_executable(ioprey ${IOPREY})
        add_executable(wroggi ${WROGGI})

    ELSE(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

        set(CMAKE_CXX_FLAGS "-g -DDEBUG -O0 -Wall -std=c++11 -Wno-unused-variable")
        add_executable(rrpoison ${RRPOISON})
        add_executable(np ${NAME_PKT})
        TARGET_LINK_LIBRARIES(np resolv)
        #add_executable(pb ${PKT_BUFFER})
        #add_executable(nb ${NAME_BUFFER})
        add_executable(ioprey ${IOPREY})
        add_executable(wroggi ${WROGGI})
        TARGET_LINK_LIBRARIES(wroggi resolv)

    ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

else(CXX11_COMPILER)

    message(FATAL_ERROR "Your C++ compiler does not support C++11.")

endif(CXX11_COMPILER)


