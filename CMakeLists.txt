cmake_minimum_required(VERSION 3.5)

project(src C CXX)

set(CMAKE_CXX_STANDARD 17)

set(CXX_FLAGS
    -g
    # -DVALGRIND
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wno-unused-parameter
    -Wold-style-cast
    -Woverloaded-virtual
    -Wpointer-arith
    -Wshadow
    -Wwrite-strings
    -march=native
    # -MMD
    -std=c++17
    -rdynamic
)

add_compile_options(${CXX_FLAGS})

add_subdirectory(src/base)
add_subdirectory(src/net)

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(dws main.cc)

target_link_libraries(dws dws_base dws_net)