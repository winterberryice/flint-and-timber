cmake_minimum_required(VERSION 3.28)
project(FlintAndTimber)

set(TARGET_NAME "flint-and-timber")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable FetchContent
include(FetchContent)

# Fetch SDL3
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sdl3-git-tag.txt SDL3_GIT_TAG)
string(STRIP ${SDL3_GIT_TAG} SDL3_GIT_TAG)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG ${SDL3_GIT_TAG}
)

FetchContent_MakeAvailable(SDL3)

# ============ WebGPU Configuration ============
# Set WebGPU backend to Dawn BEFORE adding subdirectory
set(WEBGPU_BACKEND "DAWN" CACHE STRING "WebGPU backend to use")

# NEW: Tell WebGPU-distribution to build Dawn from source
set(WEBGPU_BUILD_FROM_SOURCE ON CACHE BOOL "Build Dawn from source")
if(UNIX AND NOT APPLE)
    set(DAWN_ENABLE_WAYLAND ON CACHE BOOL "Enable Wayland support")
    set(DAWN_USE_WAYLAND ON CACHE BOOL "Use Wayland platform")
    set(DAWN_ENABLE_X11 ON CACHE BOOL "Enable X11 support")
endif()
set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "Let Dawn fetch dependencies")

# Speed up build by disabling unnecessary Dawn features
set(TINT_BUILD_TESTS OFF CACHE BOOL "Skip Tint tests")
set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "Skip Dawn samples")
set(DAWN_USE_GLFW OFF CACHE BOOL "Skip GLFW in Dawn")

# Add WebGPU-distribution (Dawn) - now it will build from source!
add_subdirectory(webgpu)

# Add SDL3WebGPU helper
add_subdirectory(sdl3webgpu)

# Create executable
add_executable(${TARGET_NAME} main.cpp)

# Link libraries
target_link_libraries(${TARGET_NAME} PRIVATE 
    SDL3::SDL3
    webgpu
    sdl3webgpu
)

# Set RPATH for Linux to find shared libraries in the executable's directory
if(UNIX AND NOT APPLE)
    set_target_properties(${TARGET_NAME} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    set_target_properties(${TARGET_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
endif()

# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/webgpu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl3webgpu/include
)


# Copy WebGPU binaries using the proper function provided by WebGPU-distribution
# This should handle the Dawn binary copying automatically
if(COMMAND target_copy_webgpu_binaries)
    target_copy_webgpu_binaries(${TARGET_NAME})
endif()
