cmake_minimum_required(VERSION 3.28)
project(FlintAndTimber)

set(TARGET_NAME "flint-and-timber")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable FetchContent
include(FetchContent)

# Fetch SDL3
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl3-git-tag.txt SDL3_GIT_TAG)
string(STRIP ${SDL3_GIT_TAG} SDL3_GIT_TAG)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG ${SDL3_GIT_TAG}
)

# Fetch GLM
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm-git-tag.txt GLM_GIT_TAG)
string(STRIP ${GLM_GIT_TAG} GLM_GIT_TAG)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG ${GLM_GIT_TAG}
)

# Fetch ImGui
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-git-tag.txt IMGUI_GIT_TAG)
string(STRIP ${IMGUI_GIT_TAG} IMGUI_GIT_TAG)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${IMGUI_GIT_TAG}
)

FetchContent_MakeAvailable(SDL3 glm imgui)

# ============ ImGui Configuration ============
# ImGui doesn't provide a CMakeLists.txt, so we need to build it ourselves
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
)

# ============ WebGPU Configuration ============
# Set WebGPU backend to Dawn BEFORE adding subdirectory
set(WEBGPU_BACKEND "DAWN" CACHE STRING "WebGPU backend to use")

# Use precompiled Dawn binaries from winterberryice/flint-and-timber-dawn
set(WEBGPU_BUILD_FROM_SOURCE OFF CACHE BOOL "Build Dawn from source")

# NOTE: The following options are only relevant when building from source
# Keeping them commented for reference if we need to switch back
# if(UNIX AND NOT APPLE)
#     set(DAWN_ENABLE_WAYLAND ON CACHE BOOL "Enable Wayland support")
#     set(DAWN_USE_WAYLAND ON CACHE BOOL "Use Wayland platform")
#     set(DAWN_ENABLE_X11 ON CACHE BOOL "Enable X11 support")
# endif()
# set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "Let Dawn fetch dependencies")
# set(TINT_BUILD_TESTS OFF CACHE BOOL "Skip Tint tests")
# set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "Skip Dawn samples")
# set(DAWN_USE_GLFW OFF CACHE BOOL "Skip GLFW in Dawn")

# Add WebGPU-distribution (Dawn) - will use precompiled binaries
add_subdirectory(third_party/webgpu)

# Add SDL3WebGPU helper
add_subdirectory(third_party/sdl3webgpu)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")

# Create executable (including ImGui sources)
add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

# Include src directory for headers
target_include_directories(${TARGET_NAME} PRIVATE src)

# Link libraries
target_link_libraries(${TARGET_NAME} PRIVATE
    glm::glm
    SDL3::SDL3
    webgpu
    sdl3webgpu
)

# Define ImGui WebGPU backend (using Dawn)
target_compile_definitions(${TARGET_NAME} PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)

# Set RPATH for Linux to find shared libraries in the executable's directory
if(UNIX AND NOT APPLE)
    set_target_properties(${TARGET_NAME} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    set_target_properties(${TARGET_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
endif()

# Set RPATH for macOS to find shared libraries in the executable's directory
if(APPLE)
    set_target_properties(${TARGET_NAME} PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    set_target_properties(${TARGET_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path"
    )
endif()

# --- Generate texture atlas header ---
set(GENERATED_ATLAS_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/atlas_bytes.hpp")
set(ATLAS_SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/block/atlas.png")

add_custom_command(
    OUTPUT ${GENERATED_ATLAS_HEADER}
    COMMAND ${CMAKE_COMMAND} -D ATLAS_INPUT_FILE=${ATLAS_SOURCE_FILE} -D ATLAS_OUTPUT_FILE=${GENERATED_ATLAS_HEADER} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateAtlasHeader.cmake"
    DEPENDS ${ATLAS_SOURCE_FILE} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateAtlasHeader.cmake"
    COMMENT "Converting ${ATLAS_SOURCE_FILE} to C++ header via CMake script"
    VERBATIM
)

add_custom_target(GenerateAtlasHeader DEPENDS ${GENERATED_ATLAS_HEADER})
add_dependencies(${TARGET_NAME} GenerateAtlasHeader)


# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/webgpu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl3webgpu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)


# Copy WebGPU binaries using the proper function provided by WebGPU-distribution
# This should handle the Dawn binary copying automatically
if(COMMAND target_copy_webgpu_binaries)
    target_copy_webgpu_binaries(${TARGET_NAME})
endif()
