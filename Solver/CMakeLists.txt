cmake_minimum_required(VERSION 3.24)

project(Solver CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Test")
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    FetchContent_MakeAvailable(googletest)
endif()

file(GLOB SOLVER_SOURCES CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
)

file(GLOB SOLVER_HEADERS CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.h"
)

add_executable(${PROJECT_NAME} ${SOLVER_SOURCES} ${SOLVER_HEADERS})

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE Core spdlog::spdlog)

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

if(CMAKE_BUILD_TYPE STREQUAL "Test")
    enable_testing()
    add_subdirectory(tests)
endif()
