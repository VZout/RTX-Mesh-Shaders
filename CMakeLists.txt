# dir -Recurse *.* | Get-Content | Measure-Object -Line -Word
cmake_minimum_required(VERSION 3.12)
project(TuringMeshShaders)

# CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# C++ settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none was specified
set(DEFAULT_BUILD_TYPE "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(DEFAULT_BUILD_TYPE "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
set(CMAKE_DEBUG_POSTFIX "d")

# Use FindVulkan module added with CMAKE 3.7
if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	#find_package(Vulkan)
endif ()

if (WIN32)
	if (NOT VULKAN_FOUND)
		find_library(VULKAN_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/external/vulkan/libs)
		if (VULKAN_LIBRARY)
			set(VULKAN_FOUND ON)
			message(STATUS "Using bundled Vulkan library version")
		endif ()
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
else(WIN32)
	if (NOT VULKAN_FOUND)
		find_library(VULKAN_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/external/vulkan/libs" REQUIRED)
		if (VULKAN_LIBRARY)
			set(VULKAN_FOUND ON)
			message(STATUS "Using bundled Vulkan library version (Might take the lib bundled with Windows instead)")
		endif()
	endif()
	find_package(Threads REQUIRED)
if(USE_D2D_WSI)
	message(STATUS "Using direct to display extension...")
	add_definitions(-D_DIRECT2DISPLAY)
elseif(USE_WAYLAND_WSI)
	find_program(PKG_CONFIG pkg-config)
	if (NOT PKG_CONFIG)
		message(FATAL_ERROR "pkg-config binary not found")
	endif ()
	find_package(Wayland REQUIRED)
	if (NOT WAYLAND_FOUND)
		message(FATAL_ERROR "Wayland development package not found")
	endif ()
	pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
	if (NOT WAYLAND_PROTOCOLS_FOUND)
		message(FATAL_ERROR "Wayland protocols package not found")
	endif ()
	find_program(WAYLAND_SCANNER wayland-scanner)
	if (NOT WAYLAND_SCANNER)
		message(FATAL_ERROR "wayland-scanner binary not found")
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
	include_directories(${WAYLAND_INCLUDE_DIR})
	execute_process(COMMAND ${PKG_CONFIG} --variable=pkgdatadir wayland-protocols OUTPUT_VARIABLE PROTOCOL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${PROTOCOL_DIR}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
			COMMAND ${WAYLAND_SCANNER} private-code ${PROTOCOL_DIR}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
	include_directories(${CMAKE_BINARY_DIR})
else (USE_D2D_WSI)
	find_package(XCB REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
endif (USE_D2D_WSI)
endif (WIN32)

if (NOT VULKAN_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else ()
	message(STATUS "Found Vulkan: ${VULKAN_LIBRARY}")
endif ()

# Other Externals
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# Source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE GLSL_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.*")
file(GLOB IMGUI CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/*.cpp")

# Application
add_executable(Test ${HEADERS} ${SOURCES} ${GLSL_SOURCE_FILES} ${IMGUI})
target_link_libraries(Test ${VULKAN_LIBRARY} glfw)
target_include_directories(Test PUBLIC
	external/glfw/include
	external/vulkan/include external/glm/glm
	external/imgui)

# Compile GLSL Shaders Into SPRIV
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/bin/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/bin/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
	COMMENT "Compiling GLSL to SPRIV")
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(Test Shaders)
