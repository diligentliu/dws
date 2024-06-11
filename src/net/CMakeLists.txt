cmake_minimum_required(VERSION 3.5)
project(net)

include_directories(
    ./
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB_RECURSE NET_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(dws_net STATIC ${NET_SOURCE})

target_link_libraries(dws_net dws_base)

target_include_directories(dws_net PUBLIC ./ ${CMAKE_CURRENT_SOURCE_DIR}/include)
