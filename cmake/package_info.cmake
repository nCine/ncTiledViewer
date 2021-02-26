set(PACKAGE_NAME "ncTiledViewer")
set(PACKAGE_EXE_NAME "nctiledviewer")
set(PACKAGE_VENDOR "Angelo Theodorou")
set(PACKAGE_COPYRIGHT "Copyright ©2020-2021 ${PACKAGE_VENDOR}")
set(PACKAGE_DESCRIPTION "A viewer for Tiled maps made with the nCine")
set(PACKAGE_HOMEPAGE "https://ncine.github.io")
set(PACKAGE_REVERSE_DNS "io.github.ncine.nctiledviewer")

set(PACKAGE_INCLUDE_DIRS include)

set(PACKAGE_SOURCES
	include/main.h
	include/MapModel.h
	include/TmxParser.h
	include/MapFactory.h
	include/FileDialog.h
	include/CameraNode.h

	src/main.cpp
	src/TmxParser.cpp
	src/MapFactory.cpp
	src/FileDialog.cpp
	src/CameraNode.cpp
)

function(callback_after_target)
	include(custom_pugixml)

	if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
		if(IS_DIRECTORY ${PACKAGE_DATA_DIR})
			file(GLOB MAP_FILES "${PACKAGE_DATA_DIR}/data/maps/*[.tmx,.tsx,.png]")
		endif()
		set(PACKAGE_ANDROID_ASSETS ${MAP_FILES} CACHE STRING "" FORCE)
	else()
		target_include_directories(${PACKAGE_EXE_NAME} PRIVATE ${GENERATED_INCLUDE_DIR}/../../${PUGIXML_SOURCE_DIR_NAME}/src)
		target_sources(${PACKAGE_EXE_NAME} PRIVATE ${GENERATED_INCLUDE_DIR}/../../${PUGIXML_SOURCE_DIR_NAME}/src/pugixml.cpp)
		target_compile_definitions(${PACKAGE_EXE_NAME} PRIVATE "PUGIXML_NO_XPATH" "PUGIXML_NO_STL" "PUGIXML_NO_EXCEPTIONS")
	endif()
endfunction()
