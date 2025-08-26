cmake_minimum_required(VERSION 3.13)
project(app)
set(CMAKE_CXX_STANDARD 20)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn")

# First, let's see what webgpu header generators are available
get_property(all_targets DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dawn" PROPERTY BUILDSYSTEM_TARGETS)
foreach(target ${all_targets})
    if(target MATCHES "webgpu.*gen" OR target MATCHES ".*headers.*gen")
        message(STATUS "Found generator target: ${target}")
    endif()
endforeach()

add_executable(app "main.cpp")

# Only use GLFW for now until we resolve the generator issue
target_link_libraries(app PRIVATE glfw)

target_include_directories(app PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/dawn/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/dawn/third_party/glfw/include"
)