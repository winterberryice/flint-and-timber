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

FetchContent_MakeAvailable(SDL3 glm)

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
add_subdirectory(third_party/webgpu)

# Add SDL3WebGPU helper
add_subdirectory(third_party/sdl3webgpu)

# ============ Atlas Generation ============
add_subdirectory(tools/atlas_generator)

set(ATLAS_ASSET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/block)
set(ATLAS_JSON_PATH ${ATLAS_ASSET_DIR}/atlas.json)
set(ATLAS_OUTPUT_PNG ${CMAKE_CURRENT_BINARY_DIR}/atlas.png)
set(ATLAS_OUTPUT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_atlas.hpp)

# Ensure the directory for the generated header exists
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Generate a list of texture dependencies from the JSON file
file(READ ${ATLAS_JSON_PATH} ATLAS_JSON_CONTENT)
string(JSON ATLAS_TEXTURE_FILES GET ${ATLAS_JSON_CONTENT})

set(ATLAS_TEXTURE_PATHS)
foreach(ROW ${ATLAS_TEXTURE_FILES})
    string(JSON TEXTURES GET ${ROW})
    foreach(TEXTURE ${TEXTURES})
        list(APPEND ATLAS_TEXTURE_PATHS "${ATLAS_ASSET_DIR}/${TEXTURE}")
    endforeach()
endforeach()

add_custom_command(
    OUTPUT ${ATLAS_OUTPUT_PNG} ${ATLAS_OUTPUT_HEADER}
    COMMAND atlas_generator ${ATLAS_ASSET_DIR} ${ATLAS_JSON_PATH} ${ATLAS_OUTPUT_PNG} ${ATLAS_OUTPUT_HEADER}
    DEPENDS atlas_generator ${ATLAS_JSON_PATH} ${ATLAS_TEXTURE_PATHS}
    COMMENT "Generating texture atlas and C++ header from ${ATLAS_JSON_PATH}..."
)

add_custom_target(generate_assets ALL DEPENDS ${ATLAS_OUTPUT_PNG} ${ATLAS_OUTPUT_HEADER})


# Collect all source files from src directory
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")

# Create executable
add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})

# Ensure assets are generated before the main target is built
add_dependencies(${TARGET_NAME} generate_assets)

# Include src directory for headers
target_include_directories(${TARGET_NAME} PRIVATE src)

# Link libraries
target_link_libraries(${TARGET_NAME} PRIVATE 
    glm::glm
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
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/webgpu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl3webgpu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)


# Copy WebGPU binaries using the proper function provided by WebGPU-distribution
# This should handle the Dawn binary copying automatically
if(COMMAND target_copy_webgpu_binaries)
    target_copy_webgpu_binaries(${TARGET_NAME})
endif()
