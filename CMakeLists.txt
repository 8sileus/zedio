cmake_minimum_required(VERSION 3.24.2)

project (ZED  VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(NOT  CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Version: " ${CMAKE_PROJECT_VERSION})
message(STATUS "C++ standard: " ${CMAKE_CXX_STANDARD})
message(STATUS "Compiler: " ${CMAKE_CXX_COMPILER_ID})
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

set(LIBS     
yaml-cpp)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/tests)