cmake_minimum_required(VERSION 3.28)
project(SDL3WebGPUBasic)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable FetchContent
include(FetchContent)

# Fetch SDL3
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main
)

FetchContent_MakeAvailable(SDL3)

# Set WebGPU backend to Dawn BEFORE adding subdirectory
set(WEBGPU_BACKEND "DAWN" CACHE STRING "WebGPU backend to use")

# Add WebGPU-distribution (Dawn)
add_subdirectory(webgpu)

# Add SDL3WebGPU helper
add_subdirectory(sdl3webgpu)

# Create executable
add_executable(${PROJECT_NAME} main.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3
    webgpu
    sdl3webgpu
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/webgpu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl3webgpu/include
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static)
endif()

# Copy WebGPU binaries using the proper function provided by WebGPU-distribution
# This should handle the Dawn binary copying automatically
if(COMMAND target_copy_webgpu_binaries)
    target_copy_webgpu_binaries(${PROJECT_NAME})
endif()
