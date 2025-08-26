cmake_minimum_required(VERSION 3.13)
project(app)
set(CMAKE_CXX_STANDARD 20)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn")

# Get ALL targets from all directories
function(get_all_targets var)
    set(targets)
    get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

function(get_all_targets_recursive var dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_all_targets_recursive(${var} ${subdir})
    endforeach()
    
    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    set(${var} ${${var}} ${current_targets} PARENT_SCOPE)
endfunction()

get_all_targets(all_targets)
message(STATUS "ALL AVAILABLE TARGETS: ${all_targets}")

add_executable(app "main.cpp")

# Try with just glfw for now
target_link_libraries(app PRIVATE glfw)
target_include_directories(app PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/dawn/third_party/glfw/include"
)