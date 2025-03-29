cmake_minimum_required(VERSION 3.24.2)

project (ZED LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED 23)

# add_compile_options("-Wall")
# add_compile_options("-Wextra")
# add_compile_options("-Werror")
# add_compile_options("-Wundef")
# add_compile_options("-Wsign-compare")


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    add_compile_definitions(NEED_ZEDIO_LOG)
    message(STATUS "need zedio log")
endif()

add_compile_definitions(NOMINMAX)

message(STATUS "Version: " "${CMAKE_PROJECT_VERSION}")
message(STATUS "C++ standard: " "${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: " "${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type: " "${CMAKE_BUILD_TYPE}")
message(STATUS "Target Architecture: " "${CMAKE_GENERATOR_PLATFORM}")

set(DEPEND_LIBS 
# pthread
# uring
)

include_directories("${PROJECT_SOURCE_DIR}")


install(DIRECTORY "${PROJECT_SOURCE_DIR}/zedio" 
        DESTINATION "include")


add_subdirectory("${PROJECT_SOURCE_DIR}/examples")
enable_testing()
find_package(Boost)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()