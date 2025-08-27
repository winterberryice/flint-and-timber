cmake_minimum_required(VERSION 3.13)
project(app)
set(CMAKE_CXX_STANDARD 20)

# Add GLFW first
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# You have a few options for GLFW:

# Option 1: Use system GLFW (if installed)
find_package(glfw3 QUIET)
if(glfw3_FOUND)
    message(STATUS "Using system GLFW")
else()
    # Option 2: Fetch GLFW from GitHub
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)
endif()

# Use the WebGPU distribution
set(WEBGPU_BACKEND "DAWN" CACHE STRING "Backend to use")
add_subdirectory(webgpu)

add_executable(app "main.cpp")

if(APPLE)
    # For macOS, we need to link against the QuartzCore framework for CAMetalLayer
    find_library(QUARTZ_CORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(app PRIVATE ${QUARTZ_CORE_FRAMEWORK})

    # Compile main.cpp as Objective-C++ on macOS
    set_source_files_properties(main.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

# Link both WebGPU and GLFW
target_link_libraries(app PRIVATE webgpu glfw)