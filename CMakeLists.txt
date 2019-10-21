# dir -Recurse *.* | Get-Content | Measure-Object -Line -Word
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(TuringMeshShaders)
include(FetchContent)

if(MSVC)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Sileceable message func
function(message)
    if (NOT MESSAGE_QUIET)
        _message(${ARGN})
    endif()
endfunction()

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColorReset "${Esc}[m")
  set(ColorBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(Grey        "${Esc}[90m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

macro(add_dependency)
  foreach(contentName IN ITEMS ${ARGV})
    string(TOLOWER ${contentName} contentNameLower)
    FetchContent_GetProperties(${contentName})
    if(NOT ${contentNameLower}_POPULATED)
		message(STATUS "\tSetting up ${BoldBlue}${contentName}${ColorReset}")
      FetchContent_Populate(${contentName})

	  if(${ADD_DEP_USE_CMAKELIST})
		message("\t\t ${Grey}Using CMakeLists.txt${ColorReset}")
		set(MESSAGE_QUIET ON)
        add_subdirectory(${${contentNameLower}_SOURCE_DIR}
                         ${${contentNameLower}_BINARY_DIR})
		unset(MESSAGE_QUIET)
	  else()
		message("\t\t ${Grey}Using as header only library${ColorReset}")
      endif()
    endif()
  endforeach()
endmacro()

# CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(ASSIMP_BUILD_SHARED_LIBS OFF)

set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
#set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
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
set( ASSIMMP_BUILD_OBJ_IMPORTER     ON CACHE BOOL "" FORCE)
set( ASSIMP_BUILD_OGRE_IMPORTER     OFF )
set( ASSIMP_BUILD_OPENGEX_IMPORTER  OFF )
set( ASSIMP_BUILD_PLY_IMPORTER      OFF )
set( ASSIMP_BUILD_MS3D_IMPORTER     OFF )
set( ASSIMP_BUILD_COB_IMPORTER      OFF )
set( ASSIMP_BUILD_BLEND_IMPORTER    OFF )
set( ASSIMP_BUILD_IFC_IMPORTER      OFF )
set( ASSIMP_BUILD_XGL_IMPORTER      OFF )
set( ASSIMMP_BUILD_STEP_IMPORTER    OFF )
set( ASSIMMP_BUILD_FBX_IMPORTER     ON CACHE BOOL "" FORCE)
set( ASSIMP_BUILD_Q3D_IMPORTER      OFF )
set( ASSIMP_BUILD_Q3BSP_IMPORTER    OFF )
set( ASSIMP_BUILD_RAW_IMPORTER      OFF )
set( ASSIMP_BUILD_SIB_IMPORTER      OFF )
set( ASSIMP_BUILD_SMD_IMPORTER      OFF )
set( ASSIMP_BUILD_STL_IMPORTER      OFF )
set( ASSIMP_BUILD_TERRAGEN_IMPORTER OFF )
set( ASSIMP_BUILD_3D_IMPORTER       OFF )
set( ASSIMP_BUILD_X_IMPORTER        OFF )
set( ASSIMP_BUILD_X3D_IMPORTER      OFF )
set( ASSIMP_BUILD_GLTF_IMPORTER     OFF )
set( ASSIMP_BUILD_3MF_IMPORTER      OFF )
set( ASSIMP_BUILD_MMD_IMPORTER      OFF )
# END IMPORTERS

message(STATUS "${Magenta}Setting Up Dependencies${ColorReset}")

# Externals
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
  GIT_SHALLOW true
)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.5.0
  GIT_SHALLOW true
)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        6.0.0
  GIT_SHALLOW true
)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3
  GIT_SHALLOW true
)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        origin/docking
  GIT_SHALLOW true
)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.6
  GIT_SHALLOW true
)
FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG        83bb1a48d79729f2688d9c63e2754ce976a65f72
  GIT_SHALLOW true
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
  GIT_SHALLOW true
)
FetchContent_Declare(
  vma
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        v2.2.0
  GIT_SHALLOW true
)

FetchContent_GetProperties(googletest)
FetchContent_GetProperties(benchmark)
FetchContent_GetProperties(fmt)
FetchContent_GetProperties(glfw)
FetchContent_GetProperties(imgui)
FetchContent_GetProperties(glm)
FetchContent_GetProperties(tinygltf)
FetchContent_GetProperties(assimp)
FetchContent_GetProperties(vma)
# Set external options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "" FORCE)
# Build some externals in release mode
set(CMAKE_BUILD_TYPE Release)
set(ADD_DEP_USE_CMAKELIST TRUE)
add_dependency(googletest benchmark glfw assimp) # Libraries build in release mode.
unset(CMAKE_BUILD_TYPE CACHE)
set(CMAKE_BUILD_TYPE)
set(ADD_DEP_USE_CMAKELIST FALSE)
add_dependency(glm glm imgui tinygltf vma) # Header only libraries.
set(ADD_DEP_USE_CMAKELIST TRUE)
add_dependency(fmt) # Libraries build in debug mode.

# Vulkan External
# Use FindVulkan module
message(STATUS "Using module to find Vulkan")
#find_package(Vulkan)

if (WIN32)
	if (NOT VULKAN_FOUND)
		FetchContent_GetProperties(vulkan)
		if(NOT vulkan_POPULATED)
			message(STATUS "Setting up VulkanLibHeaders")
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

# Setup MSVC Folders
if(MSVC)
	set_target_properties(assimp PROPERTIES FOLDER "Third Party")
	set_target_properties(IrrXML PROPERTIES FOLDER "Third Party")
	set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Third Party")
	if (TARGET zlib)
		set_target_properties(zlib PROPERTIES FOLDER "Third Party")
		set_target_properties(zlibstatic PROPERTIES FOLDER "Third Party")
	endif()
	set_target_properties(uninstall PROPERTIES FOLDER "Third Party")
	set_target_properties(benchmark PROPERTIES FOLDER "Third Party")
	set_target_properties(benchmark_main PROPERTIES FOLDER "Third Party")
	set_target_properties(gtest PROPERTIES FOLDER "Third Party")
	set_target_properties(gtest_main PROPERTIES FOLDER "Third Party")
	set_target_properties(gmock PROPERTIES FOLDER "Third Party")
	set_target_properties(gmock_main PROPERTIES FOLDER "Third Party")
	set_target_properties(fmt PROPERTIES FOLDER "Third Party")
	set_target_properties(glfw PROPERTIES FOLDER "Third Party")
endif()

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
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCES_FG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/frame_graph/*.cpp")
file(GLOB HEADERS_FG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/frame_graph/*.hpp")
file(GLOB SOURCES_SG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/scene_graph/*.cpp")
file(GLOB HEADERS_SG CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/scene_graph/*.hpp")
file(GLOB SOURCES_TASKS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/render_tasks/*.cpp")
file(GLOB HEADERS_TASKS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/render_tasks/*.hpp")
file(GLOB SOURCES_UTIL CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp")
file(GLOB HEADERS_UTIL CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.hpp")
file(GLOB SOURCES_GFX CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/*.cpp")
file(GLOB HEADERS_GFX CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/*.hpp")
file(GLOB SOURCES_IMGUI CONFIGURE_DEPENDS "${imgui_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/*.cpp")
file(GLOB HEADERS_IMGUI CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/*.hpp")

file(GLOB_RECURSE GLSL_SOURCE_FILES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.frag"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.vert"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.comp")
		#"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*.glsl")

source_group("High Level API" FILES ${SOURCES} ${HEADERS})
source_group("Frame Graph" FILES ${HEADERS_FG} ${SOURCES_FG})
source_group("Scene Graph" FILES ${HEADERS_SG} ${SOURCES_SG})
source_group("Render Tasks" FILES ${SOURCES_TASKS} ${HEADERS_TASKS})
source_group("Graphics" FILES ${HEADERS_GFX} ${SOURCES_GFX})
source_group("Utility" FILES ${HEADERS_UTIL} ${SOURCES_UTIL})
source_group("ImGui" FILES ${HEADERS_IMGUI} ${SOURCES_IMGUI})
source_group("Shaders" FILES ${GLSL_SOURCE_FILES})

# Application
add_library(WispVK
	${HEADERS} ${SOURCES}
	${HEADERS_FG} ${SOURCES_FG}
	${HEADERS_SG} ${SOURCES_SG}
	${HEADERS_TASKS} ${SOURCES_TASKS}
	${HEADERS_UTIL} ${SOURCES_UTIL}
	${HEADERS_GFX} ${SOURCES_GFX}
	${HEADERS_IMGUI} ${SOURCES_IMGUI})
target_link_libraries(WispVK ${VULKAN_LIBRARY} glfw fmt assimp)
target_include_directories(WispVK PUBLIC
	${glfw_SOURCE_DIR}/include
	${vulkan_SOURCE_DIR}/include
	${glm_SOURCE_DIR}/glm
	${imgui_SOURCE_DIR}
	${vma_SOURCE_DIR}/src
	${fmt_SOURCE_DIR}/include
	${assimp_SOURCE_DIR}/include
	${tinygltf_SOURCE_DIR})

# Compile GLSL Shaders Into SPRIV
if (NOT WIN32)
	set(GLSL_VALIDATOR "glslc")
else()
	if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
	  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslc.exe")
	else()
	  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslc.exe")
	endif()
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
    WispVK_Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
	SOURCES ${GLSL_SOURCE_FILES}
    )

add_dependencies(WispVK WispVK_Shaders)

# Copy external resources
add_custom_command(TARGET WispVK PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/resources/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		COMMENT "Copying Resources")

# Build Tests
message(STATUS "${Magenta}Configuring WispVk Tests & Benchmarks:${ColorReset}")
add_subdirectory(tests)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Demo)

message(STATUS "${Magenta}WispVK Configuration Info:${ColorReset}")
if(NOT CMAKE_BUILD_TYPE)
	message("\tBuild Type: ${Yellow}${CMAKE_CONFIGURATION_TYPES}${ColorReset}")
else()
	message("\tBuild Type: ${Yellow}${BUILD_TYPE}${ColorReset}")
endif()
message("\tC++ Version: ${Yellow}${CMAKE_CXX_STANDARD}${ColorReset}")
