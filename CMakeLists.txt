cmake_minimum_required(VERSION 3.24.2)

project (ZED  VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT  CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Version: " "${CMAKE_PROJECT_VERSION}")
message(STATUS "C++ standard: " "${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: " "${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type: " "${CMAKE_BUILD_TYPE}")

set(DEPEND_LIBS 
pthread
uring
)

include_directories("${PROJECT_SOURCE_DIR}/zed")


install(DIRECTORY "${PROJECT_SOURCE_DIR}/zed" 
        DESTINATION "include")


enable_testing()
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")