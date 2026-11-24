cmake_minimum_required(VERSION 3.24)

project(Core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

if(NOT WIN32)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Relwithdebinfo" CACHE STRING "" FORCE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Debug build")
        target_compile_definitions(${PROJECT_NAME} PRIVATE _DEBUG)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Release build")
        target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(STATUS "Release with debug info build")
        target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
        target_compile_options(${PROJECT_NAME} PRIVATE -O3 -g)
    else()
        message(FATAL_ERROR "Invalid build type. Use Debug or Release.")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()
