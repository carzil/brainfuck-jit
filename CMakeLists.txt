cmake_minimum_required(VERSION 3.0.1)

project(vm)

find_library(LIBJIT_LIBRARY NAMES jit PATH_SUFFIXES x86 x86_64)
find_path(LIBJIT_INCLUDE_DIR jit/jit.h PATH_SUFFIXES jit)
set(LIBJIT_INCLUDE_DIRS ${LIBJIT_INCLUDE_DIR})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBJIT_INCLUDE_DIRS}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Wextra -O0 -g -fsanitize=address")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Ofast")

if (CMAKE_BUILD_TYPE EQUAL DEBUG)
    add_definitions(-DDEBUG)
endif()

set(SOURCE_FILES
    src/vm.c
    src/compiler.c
    src/optimizer.c
    src/bf.c
)

add_executable(vm ${SOURCE_FILES})
target_link_libraries(vm ${LIBJIT_LIBRARY})
