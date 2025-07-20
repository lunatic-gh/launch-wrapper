cmake_minimum_required(VERSION 3.31)

project(launch-wrapper)

set(CMAKE_CXX_STANDARD 23)
file(GLOB_RECURSE sources "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)