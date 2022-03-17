set(NCPROJECT_NAME "ncTiledViewer")
set(NCPROJECT_EXE_NAME "nctiledviewer")
set(NCPROJECT_VENDOR "Angelo Theodorou")
set(NCPROJECT_COPYRIGHT "Copyright ©2020-2022 ${NCPROJECT_VENDOR}")
set(NCPROJECT_DESCRIPTION "A viewer for Tiled maps made with the nCine")
set(NCPROJECT_HOMEPAGE "https://ncine.github.io")
set(NCPROJECT_REVERSE_DNS "io.github.ncine.nctiledviewer")

set(NCPROJECT_INCLUDE_DIRS include)

set(NCPROJECT_SOURCES
	include/main.h
	include/MapModel.h
	include/TmxParser.h
	include/MapFactory.h
	include/FileDialog.h
	include/CameraController.h

	src/main.cpp
	src/TmxParser.cpp
	src/MapFactory.cpp
	src/FileDialog.cpp
	src/CameraController.cpp
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

function(callback_after_target)
	include(custom_pugixml)

	if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
		if(IS_DIRECTORY ${NCPROJECT_DATA_DIR})
			file(GLOB MAP_FILES "${NCPROJECT_DATA_DIR}/data/maps/*[.tmx,.tsx,.png]")
		endif()
		set(NCPROJECT_ANDROID_ASSETS ${MAP_FILES} CACHE STRING "" FORCE)
	else()
		target_include_directories(${NCPROJECT_EXE_NAME} PRIVATE ${NCPROJECT_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME}/src)
		target_sources(${NCPROJECT_EXE_NAME} PRIVATE ${NCPROJECT_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME}/src/pugixml.cpp)
		target_compile_definitions(${NCPROJECT_EXE_NAME} PRIVATE "PUGIXML_NO_XPATH" "PUGIXML_NO_STL" "PUGIXML_NO_EXCEPTIONS")
	endif()
endfunction()

# Don't edit beyond this line
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	return()
endif()

cmake_minimum_required(VERSION 3.13)
project(${NCPROJECT_NAME})

find_path(NCPROJECT_DIR cmake/project_main.cmake
	PATHS ${nCine_DIR} $ENV{NCINE_DIR}
	PATH_SUFFIXES project ../project ../../../project ../../../share/ncine/project
	NO_CMAKE_FIND_ROOT_PATH # For Emscripten
	DOC "Path to the nCine template project scripts")

if(IS_DIRECTORY ${NCPROJECT_DIR})
	include(${NCPROJECT_DIR}/cmake/project_main.cmake)
else()
	if(NOT DEFINED nCine_DIR AND "$ENV{NCINE_DIR}" STREQUAL "")
		message(FATAL_ERROR "Set the nCine_DIR CMake variable or the NCINE_DIR environment variable to a valid nCine directory.")
	elseif(DEFINED nCine_DIR)
		message(FATAL_ERROR "The nCine_DIR CMake variable \"${nCine_DIR}\" is not a valid nCine directory.")
	elseif(NOT "$ENV{NCINE_DIR}" STREQUAL "")
		message(FATAL_ERROR "The NCINE_DIR environment variable \"$ENV{NCINE_DIR}\" is not a valid nCine directory.")
	endif()
endif()
