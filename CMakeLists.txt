cmake_minimum_required(VERSION 3.12)
project(TuringMeshShaders)

file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "*.hpp")

add_executable(Test ${HEADERS} ${SOURCES})
