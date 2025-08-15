cmake_minimum_required(VERSION 3.16)
project(bgfx)

set(BGFX_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE BGFX_SOURCES
    "${BGFX_DIR}/src/*.cpp"
    "${BGFX_DIR}/3rdparty/khronos/glslang/glslang/MachineIndependent/*.cpp"
    "${BGFX_DIR}/3rdparty/khronos/glslang/glslang/GenericCodeGen/*.cpp"
    "${BGFX_DIR}/3rdparty/khronos/glslang/OGLCompilersDLL/*.cpp"
    "${BGFX_DIR}/3rdparty/khronos/spirv-tools/source/util/*.cpp"
)

add_library(bgfx STATIC ${BGFX_SOURCES})

target_include_directories(bgfx PUBLIC
    "${BGFX_DIR}/include"
    "${BGFX_DIR}/3rdparty"
    "${BGFX_DIR}/3rdparty/khronos"
)

if(WIN32)
    target_compile_definitions(bgfx PRIVATE "BGFX_CONFIG_RENDERER_DIRECT3D11=1" "BGFX_CONFIG_RENDERER_DIRECT3D12=1")
endif()

if(APPLE)
    target_compile_definitions(bgfx PRIVATE "BGFX_CONFIG_RENDERER_METAL=1")
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(bgfx PRIVATE "BGFX_CONFIG_RENDERER_VULKAN=1")
endif()
