cmake_minimum_required(VERSION 3.5)
project(test)

file(GLOB_RECURSE TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(dws_test ${TEST_SOURCE})

target_link_libraries(dws_test dws_base dws_net gtest gtest_main)

target_include_directories(dws_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
