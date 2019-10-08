# dir -Recurse *.* | Get-Content | Measure-Object -Line -Word
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(TuringMeshShaders)
include(FetchContent)

# CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(ASSIMP_BUILD_SHARED_LIBS OFF)

set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
#IMPORTERS
set( ASSIMP_BUILD_AMF_IMPORTER      OFF )
set( ASSIMP_BUILD_3DS_IMPORTER      OFF )
set( ASSIMP_BUILD_AC_IMPORTER       OFF )
set( ASSIMP_BUILD_ASE_IMPORTER      OFF )
set( ASSIMP_BUILD_ASSBIN_IMPORTER   OFF )
set( ASSIMP_BUILD_ASSXML_IMPORTER   OFF )
set( ASSIMP_BUILD_B3D_IMPORTER      OFF )
set( ASSIMP_BUILD_BVH_IMPORTER      OFF )
set( ASSIMP_BUILD_COLLADA_IMPORTER  OFF )
set( ASSIMP_BUILD_DXF_IMPORTER      OFF )
set( ASSIMP_BUILD_CSM_IMPORTER      OFF )
set( ASSIMP_BUILD_HMP_IMPORTER      OFF )
set( ASSIMP_BUILD_IRRMESH_IMPORTER  OFF )
set( ASSIMP_BUILD_IRR_IMPORTER      OFF )
set( ASSIMP_BUILD_LWO_IMPORTER      OFF )
set( ASSIMP_BUILD_LWS_IMPORTER      OFF )
set( ASSIMP_BUILD_MD2_IMPORTER      OFF )
set( ASSIMP_BUILD_MD3_IMPORTER      OFF )
set( ASSIMP_BUILD_MD5_IMPORTER      OFF )
set( ASSIMP_BUILD_MDC_IMPORTER      OFF )
set( ASSIMP_BUILD_MDL_IMPORTER      OFF )
set( ASSIMP_BUILD_NFF_IMPORTER      OFF )
set( ASSIMP_BUILD_NDO_IMPORTER      OFF )
set( ASSIMP_BUILD_OFF_IMPORTER      OFF )
set( ASSIMMP_BUILD_OBJ_IMPORTER     ON  ) #ON
set( ASSIMP_BUILD_OGRE_IMPORTER     OFF )
set( ASSIMP_BUILD_OPENGEX_IMPORTER  OFF )
set( ASSIMP_BUILD_PLY_IMPORTER      OFF )
set( ASSIMP_BUILD_MS3D_IMPORTER     OFF )
set( ASSIMP_BUILD_COB_IMPORTER      OFF )
set( ASSIMP_BUILD_BLEND_IMPORTER    OFF )
set( ASSIMP_BUILD_IFC_IMPORTER      OFF )
set( ASSIMP_BUILD_XGL_IMPORTER      OFF )
set( ASSIMMP_BUILD_FBX_IMPORTER     ON  ) #ON
set( ASSIMP_BUILD_Q3D_IMPORTER      OFF )
set( ASSIMP_BUILD_Q3BSP_IMPORTER    OFF )
set( ASSIMP_BUILD_RAW_IMPORTER      ON  ) #ON
set( ASSIMP_BUILD_SIB_IMPORTER      ON  ) #ON
set( ASSIMP_BUILD_SMD_IMPORTER      ON  ) #ON
set( ASSIMP_BUILD_STL_IMPORTER      OFF )
set( ASSIMP_BUILD_TERRAGEN_IMPORTER OFF )
set( ASSIMP_BUILD_3D_IMPORTER       OFF )
set( ASSIMP_BUILD_X_IMPORTER        OFF )
set( ASSIMP_BUILD_X3D_IMPORTER      OFF )
set( ASSIMP_BUILD_GLTF_IMPORTER     ON  ) #ON
set( ASSIMP_BUILD_3MF_IMPORTER      OFF )
set( ASSIMP_BUILD_MMD_IMPORTER      OFF )
# END IMPORTERS

# Externals
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.5.0
)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        6.0.0
)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3
)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        origin/docking
)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.6
)
FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG        8dccf9bf4e50d40c986fb0ddd264dd766e432019
)
FetchContent_Declare(
  vulkan
  GIT_REPOSITORY https://github.com/VZout/vk_headers_lib.git
  GIT_TAG        e166a6948b0b4cd9a2b6a5e924d79f12d8aed99c
)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        v5.0.0
)

FetchContent_GetProperties(googletest)
FetchContent_GetProperties(benchmark)
FetchContent_GetProperties(fmt)
FetchContent_GetProperties(glfw)
FetchContent_GetProperties(imgui)
FetchContent_GetProperties(glm)
FetchContent_GetProperties(tinygltf)
FetchContent_GetProperties(assimp)
# Set external options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "" FORCE)
# Build externals in release mode
set(CMAKE_BUILD_TYPE Release)
if(NOT googletest_POPULATED)
	message(STATUS "Setting up dependency: googletest")
	FetchContent_Populate(googletest)
	add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()
if(NOT benchmark_POPULATED)
	message(STATUS "Setting up dependency: benchmark")
	FetchContent_Populate(benchmark)
	add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR})
endif()
if(NOT glfw_POPULATED)
	message(STATUS "Setting up dependency: GLFW")
	FetchContent_Populate(glfw)
	add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
endif()
if(NOT assimp_POPULATED)
	message(STATUS "Setting up dependency: ASSIMP")
	FetchContent_Populate(assimp)
	add_subdirectory(${assimp_SOURCE_DIR} ${assimp_BINARY_DIR})
endif()
if(NOT glm_POPULATED)
	message(STATUS "Setting up dependency: GLM")
	FetchContent_Populate(glm)
endif()
if(NOT tinygltf_POPULATED)
	message(STATUS "Setting up dependency: TinyGLTF")
	FetchContent_Populate(tinygltf)
endif()
unset(CMAKE_BUILD_TYPE CACHE)
set(CMAKE_BUILD_TYPE)
if(NOT fmt_POPULATED)
	message(STATUS "Setting up dependency: fmt")
	FetchContent_Populate(fmt)
	add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()
if(NOT imgui_POPULATED)
	message(STATUS "Setting up dependency: ImGUI")
	FetchContent_Populate(imgui)
endif()

# Vulkan External
# Use FindVulkan module
message(STATUS "Using module to find Vulkan")
#find_package(Vulkan)

if (WIN32)
	if (NOT VULKAN_FOUND)
		FetchContent_GetProperties(vulkan)
		if(NOT vulkan_POPULATED)
			message(STATUS "Setting up dependency: VulkanLibHeaders")
			FetchContent_Populate(vulkan)
		endif()
		find_library(VULKAN_LIBRARY NAMES vulkan-1 vulkan PATHS ${vulkan_SOURCE_DIR}/libs)
		if (VULKAN_LIBRARY)
			set(VULKAN_FOUND ON)
			message(STATUS "Using bundled Vulkan library version")
		endif ()
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
else(WIN32)
	if (NOT VULKAN_FOUND)
		FetchContent_GetProperties(vulkan)
		if(NOT vulkan_POPULATED)
			message(STATUS "Setting up dependency: VulkanLibHeaders")
			FetchContent_Populate(vulkan)
		endif()
		find_library(VULKAN_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "(${vulkan_SOURCE_DIR}/libs" REQUIRED)
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
endif ()

# C++ settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	add_definitions( -DNOMINMAX )
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
else()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic")
endif()

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

# Source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB_RECURSE GLSL_SOURCE_FILES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.frag"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.vert"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.comp")
file(GLOB IMGUI CONFIGURE_DEPENDS "${imgui_SOURCE_DIR}/*.cpp")

# Application
add_library(WispVK ${HEADERS} ${SOURCES} ${GLSL_SOURCE_FILES} ${IMGUI})
target_link_libraries(WispVK ${VULKAN_LIBRARY} glfw fmt assimp)
target_include_directories(WispVK PUBLIC
	${glfw_SOURCE_DIR}/include
	${vulkan_SOURCE_DIR}/include
	${glm_SOURCE_DIR}/glm
	${imgui_SOURCE_DIR}
	${fmt_SOURCE_DIR}/include
	${assimp_SOURCE_DIR}/include
	${tinygltf_SOURCE_DIR})

# Compile GLSL Shaders Into SPRIV
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslc.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslc.exe")
endif()

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/bin/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
	COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/bin/shaders/"
    COMMAND ${GLSL_VALIDATOR} ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
	COMMENT "Compiling GLSL to SPRIV")
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(WispVK Shaders)

# Copy external resources
add_custom_command(TARGET WispVK PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/resources/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		COMMENT "Copying Resources")

# Build Tests
add_subdirectory(tests)
