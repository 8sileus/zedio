cmake_minimum_required(VERSION 3.24.2)

project (ZED  VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS ON)
add_compile_options("-Wall")
add_compile_options("-Wundef")
add_compile_options("-Wsign-compare")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    add_compile_definitions(ZED_LOG)
endif()

message(STATUS "Version: " "${CMAKE_PROJECT_VERSION}")
message(STATUS "C++ standard: " "${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: " "${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type: " "${CMAKE_BUILD_TYPE}")

set(DEPEND_LIBS 
pthread
uring
)

include_directories("${PROJECT_SOURCE_DIR}")


install(DIRECTORY "${PROJECT_SOURCE_DIR}/zed" 
        DESTINATION "include")


add_subdirectory("${PROJECT_SOURCE_DIR}/examples")
enable_testing()
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")