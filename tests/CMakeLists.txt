file(GLOB COMMON_SOURCES "common/*.cpp")
file(GLOB COMMON_HEADERS "common/*.hpp")

function(add_test TEST_DIR TEST_NAME)
	message(STATUS "Configuring example ${TEST_NAME} in ${TEST_DIR}")

	# source
	file(GLOB SOURCES "${TEST_DIR}/*.cpp")
	file(GLOB HEADERS "${TEST_DIR}/*.hpp")

	add_executable(${TEST_NAME} ${HEADERS} ${SOURCES} ${COMMON_HEADERS} ${COMMON_SOURCES})
	target_include_directories(${TEST_NAME} PUBLIC ../src/)
	target_link_libraries(${TEST_NAME} WispVK)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD 20)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	set_target_properties(${TEST_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
	set_target_properties(${TEST_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
	
	if(MSVC)
		set_target_properties(${TEST_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TEST_NAME})
	endif()
endfunction(add_test)

function(add_benchmark TEST_DIR TEST_NAME)
	message(STATUS "Configuring example ${TEST_NAME} in ${TEST_DIR}")

	# source
	file(GLOB SOURCES "${TEST_DIR}/*.cpp")
	file(GLOB HEADERS "${TEST_DIR}/*.hpp")

	add_executable(${TEST_NAME} ${HEADERS} ${SOURCES} ${COMMON_HEADERS} ${COMMON_SOURCES})
	target_include_directories(${TEST_NAME} PUBLIC ../src/ ${benchmark_SOURCE_DIR}/include)
	target_link_libraries(${TEST_NAME} WispVK benchmark)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD 20)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	set_target_properties(${TEST_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
	set_target_properties(${TEST_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")

	if(MSVC)
		set_target_properties(${TEST_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TEST_NAME})
	endif()
endfunction(add_benchmark)

add_test(demo Demo)
add_test(test_pbr Test_PBR)
add_benchmark(bm_scene_graph BM_SceneGraph)
