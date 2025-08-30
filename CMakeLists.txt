cmake_minimum_required(VERSION 3.0...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

include(FetchContent)

  FetchContent_Declare(
    sdl3
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.2.8.zip
    URL_HASH MD5=1a4affbf987808da38a0d6c8aa802a5e
  )
  FetchContent_MakeAvailable(sdl3)

  function(target_copy_sdl3_binaries Target)
    add_custom_command(
      TARGET ${Target}
      POST_BUILD
      COMMAND
        ${CMAKE_COMMAND}
        -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:${Target}>
    )
  endfunction()


set(WEBGPU_BACKEND DAWN)

add_subdirectory(webgpu)
add_subdirectory(sdl3webgpu)

add_executable(App
	main.cpp
)

target_link_libraries(App PRIVATE webgpu sdl3webgpu SDL3::SDL3)

target_copy_webgpu_binaries(App)

set_target_properties(App PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
	target_compile_options(App PRIVATE /W4)
else()
	target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

if (XCODE)
	set_target_properties(App PROPERTIES
		XCODE_GENERATE_SCHEME ON
		XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
	)
endif()

if (EMSCRIPTEN)
	set_target_properties(App PROPERTIES SUFFIX ".html")
	target_link_options(App PRIVATE -sASYNCIFY)
endif()
