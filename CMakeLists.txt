cmake_minimum_required(VERSION 3.16)
project(FlintAndTimber)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/SDL)
add_subdirectory(external/bgfx)

add_executable(flint-and-timber src/main.cpp)

target_include_directories(flint-and-timber PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include>
)

target_link_libraries(flint-and-timber
    PRIVATE
    SDL3
    bgfx
)

if(WIN32)
    target_link_libraries(flint-and-timber PRIVATE -ldl -lpthread)
endif()

if(APPLE)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    target_link_libraries(flint-and-timber PRIVATE
        "-framework Metal"
        "-framework QuartzCore"
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(flint-and-timber PRIVATE -ldl -lpthread -lX11)
endif()

set_target_properties(flint-and-timber PROPERTIES
    CXX_STANDARD 17
)
