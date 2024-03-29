cmake_minimum_required(VERSION 3.5.0)
project(database CXX)

SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.60.0 REQUIRED COMPONENTS system thread)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("debug")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread -std=c++2a -O0 -g -fsanitize=thread -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
else()
    message("release")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread -std=c++2a -O3 -flto ${CMAKE_CXX_FLAGS}")
endif()

set(hemirtmariadbinterface
    ${hemirtmariadbinterface}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mariadb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mariadbimpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/result.cpp
)

add_library(database ${hemirtmariadbinterface})
find_library(MARIADBLIB NAMES mariadb libmariadb libmariadb.so mariadb/mariadb mariadb/libmariadb mariadb/libmariadb.so)
target_link_libraries(database ${Boost_LIBRARIES} ${MARIADBLIB})