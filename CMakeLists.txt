cmake_minimum_required(VERSION 3.0.1)

project(vm)

find_library(LIBJIT_LIBRARY NAMES jit PATH_SUFFIXES x86 x86_64)
find_path(LIBJIT_INCLUDE_DIRS jit/jit.h PATH_SUFFIXES jit)

find_path(READLINE_INCLUDE_DIRS NAMES readline/readline.h)
find_library(READLINE_LIBRARY NAMES readline)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBJIT_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Wextra -O0 -g -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Ofast")

if (CMAKE_BUILD_TYPE EQUAL DEBUG)
    add_definitions(-DDEBUG)
endif()

set(SOURCE_FILES
    src/vm.cpp
    src/compiler.cpp
    src/optimizer.cpp
    src/bf.cpp
    src/utils.cpp
)

add_executable(vm ${SOURCE_FILES})
target_link_libraries(vm ${LIBJIT_LIBRARY} ${READLINE_LIBRARY})
