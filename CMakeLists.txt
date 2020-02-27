cmake_minimum_required(VERSION "3.2")

project(littlec C)

file(GLOB_RECURSE SOURCES "source/*.c" "source/*.h")

add_executable(littlec "${SOURCES}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/binaries)

if(WIN32)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT littlec)
endif(WIN32)
