cmake_minimum_required(VERSION 3.0.0...3.24 FATAL_ERROR)
project(dawn-binaries VERSION 1.0.0)

# Prevent multiple includes
if (TARGET webgpu)
	message(STATUS "Target 'webgpu' already exists.")
	return()
endif()

# Include utility functions from webgpu distribution
include("${CMAKE_CURRENT_SOURCE_DIR}/../webgpu/cmake/utils.cmake")
include(FetchContent)

# Detect system architecture
detect_system_architecture()

# Only support shared libraries for now
set(WEBGPU_LINK_TYPE "SHARED" CACHE STRING "WebGPU link type")
if (NOT WEBGPU_LINK_TYPE STREQUAL "SHARED")
	message(FATAL_ERROR "Link type '${WEBGPU_LINK_TYPE}' is not supported. Only SHARED is currently supported.")
endif()

# Build URL to fetch from winterberryice/flint-and-timber-dawn
set(URL_PLATFORM)
set(URL_ARCHIVE_EXT)
set(LIB_FILENAME)
set(LIB_IMPLIB_FILENAME)  # Import library for Windows

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(URL_PLATFORM "windows")
	set(URL_ARCHIVE_EXT "zip")
	set(LIB_FILENAME "webgpu_dawn.dll")
	set(LIB_IMPLIB_FILENAME "webgpu_dawn.lib")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(URL_PLATFORM "linux")
	set(URL_ARCHIVE_EXT "tar.gz")
	set(LIB_FILENAME "libwebgpu_dawn.so")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(URL_PLATFORM "macos")
	set(URL_ARCHIVE_EXT "tar.gz")
	set(LIB_FILENAME "libwebgpu_dawn.dylib")
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

# Set library location (archives extract to lib/ at root level)
set(LIB_DIR "lib")
set(INCLUDE_DIR "include")
set(WEBGPU_RUNTIME_LIB "${Dawn_ROOT}/${LIB_DIR}/${LIB_FILENAME}")

if (NOT EXISTS "${WEBGPU_RUNTIME_LIB}")
	message(FATAL_ERROR "WebGPU library not found at expected location: ${WEBGPU_RUNTIME_LIB}")
endif()

set_target_properties(dawn_webgpu_imported PROPERTIES
	IMPORTED_LOCATION "${WEBGPU_RUNTIME_LIB}"
)

# On Windows, we also need to set the import library (.lib file)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(WEBGPU_IMPLIB "${Dawn_ROOT}/${LIB_DIR}/${LIB_IMPLIB_FILENAME}")
	if (NOT EXISTS "${WEBGPU_IMPLIB}")
		message(FATAL_ERROR "WebGPU import library not found at expected location: ${WEBGPU_IMPLIB}")
	endif()
	set_target_properties(dawn_webgpu_imported PROPERTIES
		IMPORTED_IMPLIB "${WEBGPU_IMPLIB}"
	)
	message(STATUS "Using WebGPU import library from '${WEBGPU_IMPLIB}'")
endif()

# Set include directories
target_include_directories(dawn_webgpu_imported INTERFACE
	"${Dawn_ROOT}/${INCLUDE_DIR}"
)

# Create webgpu interface target that matches other backends
add_library(webgpu INTERFACE)
target_link_libraries(webgpu INTERFACE dawn_webgpu_imported)

# This is used to advertise the flavor of WebGPU that this provides
target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_DAWN)

message(STATUS "Using WebGPU runtime from '${WEBGPU_RUNTIME_LIB}'")
set(WEBGPU_RUNTIME_LIB ${WEBGPU_RUNTIME_LIB} CACHE INTERNAL "Path to the WebGPU library binary")

# On macOS, Dawn binaries from winterberryice/flint-and-timber-dawn are universal
# This flag helps CI scripts know not to lipo Dawn binaries
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(DAWN_IS_UNIVERSAL_BINARY TRUE CACHE BOOL "Dawn binary is already universal (arm64+x86_64)")
	message(STATUS "Dawn binary is universal (contains both arm64 and x86_64)")
endif()

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
