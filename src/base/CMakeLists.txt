cmake_minimum_required(VERSION 3.5)
project(base)

include_directories(
    ./
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB_RECURSE BASE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(dws_base STATIC ${BASE_SOURCE})

target_include_directories(dws_base PUBLIC ./ ${CMAKE_CURRENT_SOURCE_DIR}/include)
