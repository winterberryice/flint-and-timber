# This file is part of the "Learn WebGPU for C++" book.
#   https://eliemichel.github.io/LearnWebGPU
#
# MIT License
# Copyright (c) 2022-2025 Elie Michel and the wgpu-native authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Modified to fetch from winterberryice/flint-and-timber-dawn releases

include(FetchContent)

detect_system_architecture()

# Check 'WEBGPU_LINK_TYPE' argument
set(USE_SHARED_LIB)
if (WEBGPU_LINK_TYPE STREQUAL "SHARED")
	set(USE_SHARED_LIB TRUE)
elseif (WEBGPU_LINK_TYPE STREQUAL "STATIC")
	set(USE_SHARED_LIB FALSE)
	message(FATAL_ERROR "Link type '${WEBGPU_LINK_TYPE}' is not supported yet in Dawn releases.")
else()
	message(FATAL_ERROR "Link type '${WEBGPU_LINK_TYPE}' is not valid. Possible values for WEBGPU_LINK_TYPE are SHARED and STATIC.")
endif()

# Build URL to fetch from winterberryice/flint-and-timber-dawn
set(URL_PLATFORM)
set(URL_ARCHIVE_EXT)
set(LIB_FILENAME)
set(LIB_DIR)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(URL_PLATFORM "windows")
	set(URL_ARCHIVE_EXT "zip")
	set(LIB_FILENAME "webgpu_dawn.dll")
	set(LIB_DIR "windows/lib")
	set(INCLUDE_DIR "windows/include")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(URL_PLATFORM "linux")
	set(URL_ARCHIVE_EXT "tar.gz")
	set(LIB_FILENAME "libwebgpu_dawn.so")
	set(LIB_DIR "linux/lib")
	set(INCLUDE_DIR "linux/include")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(URL_PLATFORM "macos")
	set(URL_ARCHIVE_EXT "tar.gz")
	set(LIB_FILENAME "libwebgpu_dawn.dylib")
	set(LIB_DIR "macos/lib")
	set(INCLUDE_DIR "macos/include")
else()
	message(FATAL_ERROR "Platform system '${CMAKE_SYSTEM_NAME}' not supported by this release of WebGPU.")
endif()

# Determine architecture suffix for URL
set(URL_ARCH)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	# macOS uses universal binaries
	set(URL_ARCH "universal")
elseif (ARCH STREQUAL "x86_64")
	set(URL_ARCH "x64")
elseif (ARCH STREQUAL "aarch64")
	set(URL_ARCH "aarch64")
else()
	message(FATAL_ERROR "Platform architecture '${ARCH}' not supported for system '${CMAKE_SYSTEM_NAME}'.")
endif()

# Read the Dawn version tag
file(READ "${CMAKE_CURRENT_LIST_DIR}/dawn-git-tag.txt" DAWN_GIT_TAG_CONTENT)
string(STRIP "${DAWN_GIT_TAG_CONTENT}" DAWN_GIT_TAG)

# Construct URL: https://github.com/winterberryice/flint-and-timber-dawn/releases/download/chromium%2F7187/dawn-linux-x64.tar.gz
set(URL_NAME "dawn-${URL_PLATFORM}-${URL_ARCH}")
string(TOLOWER "${URL_NAME}" FC_NAME)
set(URL "https://github.com/winterberryice/flint-and-timber-dawn/releases/download/${DAWN_GIT_TAG}/${URL_NAME}.${URL_ARCHIVE_EXT}")

# Declare and fetch
FetchContent_Declare(${FC_NAME}
	URL ${URL}
)
message(STATUS "Fetching WebGPU implementation from '${URL}'")
FetchContent_MakeAvailable(${FC_NAME})

set(Dawn_ROOT "${${FC_NAME}_SOURCE_DIR}")

# Create imported library target manually (no CMake config provided)
add_library(dawn_webgpu_imported SHARED IMPORTED GLOBAL)

# Set library location
set(WEBGPU_RUNTIME_LIB "${Dawn_ROOT}/${LIB_DIR}/${LIB_FILENAME}")
if (NOT EXISTS "${WEBGPU_RUNTIME_LIB}")
	message(FATAL_ERROR "WebGPU library not found at expected location: ${WEBGPU_RUNTIME_LIB}")
endif()

set_target_properties(dawn_webgpu_imported PROPERTIES
	IMPORTED_LOCATION "${WEBGPU_RUNTIME_LIB}"
)

# Set include directories
target_include_directories(dawn_webgpu_imported INTERFACE
	"${Dawn_ROOT}/${INCLUDE_DIR}"
)

# Create webgpu interface target that matches other backends
add_library(webgpu INTERFACE)
target_link_libraries(webgpu INTERFACE dawn_webgpu_imported)

# This is used to advertise the flavor of WebGPU that this provides
target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_DAWN)

# Add webgpu.hpp from the distribution's include directory
target_include_directories(webgpu INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")

message(STATUS "Using WebGPU runtime from '${WEBGPU_RUNTIME_LIB}'")
set(WEBGPU_RUNTIME_LIB ${WEBGPU_RUNTIME_LIB} CACHE INTERNAL "Path to the WebGPU library binary")

# The application's binary must find the .dll/.so/.dylib at runtime,
# so we automatically copy it next to the binary.
function(target_copy_webgpu_binaries Target)
	add_custom_command(
		TARGET ${Target} POST_BUILD
		COMMAND
			${CMAKE_COMMAND} -E copy_if_different
			${WEBGPU_RUNTIME_LIB}
			$<TARGET_FILE_DIR:${Target}>
		COMMENT
			"Copying '${WEBGPU_RUNTIME_LIB}' to '$<TARGET_FILE_DIR:${Target}>'..."
	)
endfunction()
