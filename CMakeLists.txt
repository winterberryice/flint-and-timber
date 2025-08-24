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

# Disable building the DiligentEngine samples as they are not needed and cause build errors.
set(DILIGENT_BUILD_SAMPLES OFF CACHE BOOL "Build DiligentSamples module" FORCE)

# Add the DiligentEngine submodule directory to the CMake build.
add_subdirectory(external/DiligentEngine)


# Create the executable for our project from main.cpp
add_executable(flint-and-timber src/main.cpp src/FlintAndTimberApp.cpp)

# Add include directories for Diligent Engine
target_include_directories(flint-and-timber PRIVATE
    "external/DiligentEngine"
    "external/DiligentEngine/DiligentCore/include"
    "external/DiligentEngine/DiligentCore/Common/interface"
    "external/DiligentEngine/DiligentCore/Graphics/GraphicsEngine/interface"
    "external/DiligentEngine/DiligentGraphics/interface"
    "external/DiligentEngine/DiligentTools/include"
    "external/DiligentEngine/DiligentFX/include"
    "external/DiligentEngine/DiligentNativeApp/include"
)

# Link our application against the SDL3 library.
# CMake knows about the "SDL3::SDL3" target because we used add_subdirectory().
target_link_libraries(flint-and-timber PRIVATE SDL3::SDL3)

# Get the list of supported engine backend libraries (e.g. Diligent-GraphicsEngineD3D11-shared)
get_supported_backends(ENGINE_LIBRARIES)

# Link against Diligent Engine libraries
target_link_libraries(flint-and-timber PRIVATE
    Diligent-BuildSettings
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-TextureLoader
    DiligentFX
    Diligent-NativeAppBase
    ${ENGINE_LIBRARIES}
)


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
