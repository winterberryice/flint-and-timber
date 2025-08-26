# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.16)

# Define the project name, version, and language
project(FlintAndTimber VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the SDL3 submodule directory to the CMake build.
# This allows us to use the SDL3 targets defined in its own CMakeLists.txt.
# The second argument specifies the build directory for SDL.
add_subdirectory(external/SDL build_sdl)

# Create the executable for our project from main.cpp
add_executable(flint-and-timber src/main.cpp)

# Add The Forge include directory.
# This allows including headers like #include "Common_3/OS/Interfaces/IOperatingSystem.h"
target_include_directories(flint-and-timber PRIVATE external/TheForge)

# Link our application against the SDL3 library.
# CMake knows about the "SDL3::SDL3" target because we used add_subdirectory().
target_link_libraries(flint-and-timber PRIVATE SDL3::SDL3)

# NOTE: You will need to build The Forge libraries separately and link them to this target.
# The Forge does not use CMake, so this cannot be done automatically here.
# You will need to add target_link_libraries() with the correct paths to The Forge libs.
# Example for Linux with Vulkan backend:
# target_link_libraries(flint-and-timber PRIVATE Renderer-Vulkan OS)


# --- Platform-Specific Adjustments ---

# For Windows, we want a windowed application, not a console one.
if(WIN32)
    set_target_properties(flint-and-timber PROPERTIES WIN32_EXECUTABLE ON)
endif()

# For macOS, we need to create an application bundle to handle events correctly.
if(APPLE)
    set_target_properties(flint-and-timber PROPERTIES MACOSX_BUNDLE ON)
    
    # This helps locate the bundle resources correctly when running from the build dir
    set(CMAKE_MACOSX_RPATH 1)
endif()
