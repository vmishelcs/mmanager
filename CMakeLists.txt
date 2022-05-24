cmake_minimum_required(VERSION 3.10)

project("mmry_alloc")

set(CMAKE_C_FLAGS "-g -Wall -pthread -D UNITY_INCLUDE_DOUBLE")

# No in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

include_directories(src)

add_subdirectory(src)
add_subdirectory(unity)

add_subdirectory(tests)
