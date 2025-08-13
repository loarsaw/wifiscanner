cmake_minimum_required(VERSION 3.10)
project(embedded_wifi_scanner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUSCPP REQUIRED sdbus-c++)

include_directories(include ${SDBUSCPP_INCLUDE_DIRS})
link_directories(${SDBUSCPP_LIBRARY_DIRS})

add_library(wifi_scanner src/wifi_scanner.cpp)
target_link_libraries(wifi_scanner ${SDBUSCPP_LIBRARIES})

add_executable(wifi_cli src/main.cpp)
target_link_libraries(wifi_cli wifi_scanner ${SDBUSCPP_LIBRARIES})

