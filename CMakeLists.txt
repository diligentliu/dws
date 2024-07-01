cmake_minimum_required(VERSION 3.5)

project(dws C CXX)

set(CMAKE_CXX_STANDARD 17)

set(CXX_FLAGS
    -g
    # -DVALGRIND
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -DCMAKE_CXX_CPPLINT=cpplint
    -Wall
    -Wextra
    -Werror
    # -Wconversion
    -Wno-conversion
    -Wno-unused-parameter
    -Wold-style-cast
    -Woverloaded-virtual
    -Wpointer-arith
    -Wshadow
    -Wwrite-strings
    -march=native
    -MMD
    -std=c++17
    -rdynamic
)

add_compile_options(${CXX_FLAGS})

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.0
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(src/base)
add_subdirectory(src/net)
add_subdirectory(src/tests)

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(dws main.cc)

target_link_libraries(dws dws_base dws_net)

enable_testing()
add_test(NAME DWSTest COMMAND dws_test)
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS dws_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)
add_dependencies(dws run_tests)

set(CPPLINT_COMMAND cpplint)

file(GLOB_RECURSE ALL_SOURCE_FILES
     "${CMAKE_SOURCE_DIR}/src/*.cc"
     "${CMAKE_SOURCE_DIR}/src/*.h"
)

set(FILE_LIST_PATH "${CMAKE_BINARY_DIR}/cpplint_file_list.txt")
file(WRITE ${FILE_LIST_PATH} "")
foreach(file ${ALL_SOURCE_FILES})
    file(APPEND ${FILE_LIST_PATH} "${file}\n")
endforeach()

add_custom_target(cpplint
    COMMAND cat ${FILE_LIST_PATH} | xargs ${CPPLINT_COMMAND}
    COMMENT "Running cpplint code style checker"
)

add_dependencies(${PROJECT_NAME} cpplint)
add_dependencies(dws_test cpplint)
