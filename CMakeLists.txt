# Copyright (c) 2017 nyorain
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)
PROJECT(vulkan-msaa)

set(vmajor 0)
set(vminor 1)
set(vpatch 0)

option(Depend "Pull in dependencies in every build" true)
option(Debug "Build in debug mode" true)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable -Wno-missing-braces")
set(CMAKE_BUILD_TYPE Debug)

# to disable ninja byproduct warning
cmake_policy(SET CMP0058 NEW)

# find vulkan
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Vulkan REQUIRED)

# include dirs
include_directories("${CMAKE_CURRENT_SOURCE_DIR}") # headers
include_directories("${CMAKE_CURRENT_BINARY_DIR}") # e.g. config.hpp
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/assets")

# external libraries and headers
include_directories("${CMAKE_BINARY_DIR}/external/install/include")
include_directories("${Vulkan_INCLUDE_DIR}")
link_directories("${CMAKE_BINARY_DIR}/external/install/lib")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# external
include(ExternalProject)
set(ExternalProjectCMakeArgs
	-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/external/install
	-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DDebug=${Debug})

# ny
ExternalProject_Add(ny_ep
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ny
	GIT_REPOSITORY https://github.com/nyorain/ny.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/install
	CMAKE_ARGS ${ExternalProjectCMakeArgs}
		-DAndroid=${Android})

if(Depend)
	ExternalProject_Add_Step(ny_ep
		forceinstall
		DEPENDEES configure
		DEPENDERS install
		ALWAYS 1)
endif()

# vpp
ExternalProject_Add(vpp_ep
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/vpp
	GIT_REPOSITORY https://github.com/nyorain/vpp.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/install
	CMAKE_ARGS ${ExternalProjectCMakeArgs})

if(Depend)
	ExternalProject_Add_Step(vpp_ep
		forceinstall
		DEPENDEES configure
		DEPENDERS install
		ALWAYS 1)
endif()

# bintoheader
ExternalProject_Add(bintoheader_ep
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/bintoheader
	GIT_REPOSITORY https://github.com/nyorain/bintoheader.git
	GIT_TAG master
	INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/install
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/external/install)


if(Depend)
	ExternalProject_Add_Step(bintoheader_ep
		forceinstall
		DEPENDEES configure
		DEPENDERS install
		ALWAYS 1)
endif()

# executable
add_executable(triangle main.cpp window.cpp render.cpp engine.cpp)
target_link_libraries(triangle ny vpp ${Vulkan_LIBRARY})

add_custom_target(copy-scripts ALL)
add_dependencies(triangle copy-scripts)

add_custom_command(
	TARGET copy-scripts
	DEPENDS ${scripts}
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/assets/scripts"
		"${CMAKE_BINARY_DIR}/scripts"
	COMMENT "Copying scripts to binary dir")

# shaders
add_subdirectory(assets/shaders)
