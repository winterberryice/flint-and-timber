cmake_minimum_required(VERSION 3.16)
project(AtlasGenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(atlas_generator main.cpp stb_impl.cpp)

target_include_directories(atlas_generator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party
)

# For std::filesystem
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(atlas_generator PRIVATE -lstdc++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
     target_link_libraries(atlas_generator PRIVATE -lc++fs)
endif()
