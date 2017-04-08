project(querystall C)
cmake_minimum_required(VERSION 2.8)

include_directories(glfw/include)
include_directories(glad/include)

set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

add_library(glad STATIC glad/src/glad.c)
include_directories(glad/include)

add_executable(querystall main.c)
target_link_libraries(querystall glfw glad)

