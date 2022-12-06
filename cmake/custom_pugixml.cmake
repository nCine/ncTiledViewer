set(PUGIXML_VERSION_TAG "v1.13")
# Download release archive (TRUE) or Git repository (FALSE)
set(PUGIXML_DOWNLOAD_ARCHIVE TRUE)

if(PUGIXML_DOWNLOAD_ARCHIVE)
	# Strip the initial "v" character from the version tag
	string(REGEX MATCH "^v[0-9]" PUGIXML_STRIP_VERSION ${PUGIXML_VERSION_TAG})
	if(PUGIXML_STRIP_VERSION STREQUAL "")
		set(PUGIXML_VERSION_TAG_DIR ${PUGIXML_VERSION_TAG})
	else()
		string(SUBSTRING ${PUGIXML_VERSION_TAG} 1 -1 PUGIXML_VERSION_TAG_DIR)
	endif()

	set(PUGIXML_SOURCE_DIR_NAME pugixml-${PUGIXML_VERSION_TAG_DIR})
else()
	set(PUGIXML_SOURCE_DIR_NAME pugixml-src)
endif()

if(ANDROID)
	return()
endif()

if(PUGIXML_DOWNLOAD_ARCHIVE AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
	if (IS_DIRECTORY ${CMAKE_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME})
		message(STATUS "pugixml release file \"${PUGIXML_VERSION_TAG}\" has been already downloaded")
		set(PUGIXML_SOURCE_DIR ${CMAKE_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME})
	else()
		file(DOWNLOAD https://github.com/zeux/pugixml/archive/${PUGIXML_VERSION_TAG}.tar.gz
			${CMAKE_BINARY_DIR}/${PUGIXML_VERSION_TAG}.tar.gz STATUS result)

		list(GET result 0 result_code)
		if(result_code)
			message(FATAL_ERROR "Cannot download pugixml release file ${PUGIXML_VERSION_TAG}")
		else()
			message(STATUS "Downloaded pugixml release file \"${PUGIXML_VERSION_TAG}\"")
			file(ARCHIVE_EXTRACT INPUT ${CMAKE_BINARY_DIR}/${PUGIXML_VERSION_TAG}.tar.gz DESTINATION ${CMAKE_BINARY_DIR})
			file(REMOVE ${CMAKE_BINARY_DIR}/${PUGIXML_VERSION_TAG}.tar.gz)
		endif()
	endif()

	if (IS_DIRECTORY ${CMAKE_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME})
		target_include_directories(${NCPROJECT_EXE_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME}/src)
		target_sources(${NCPROJECT_EXE_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME}/src/pugixml.cpp)
		target_compile_definitions(${NCPROJECT_EXE_NAME} PRIVATE "PUGIXML_NO_XPATH" "PUGIXML_NO_STL" "PUGIXML_NO_EXCEPTIONS")
	endif()
else()
	# Download pugixml repository at configure time
	configure_file(cmake/custom_pugixml_download.in pugixml-download/CMakeLists.txt)

	execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pugixml-download
	)
	if(result)
		message(STATUS "CMake step for pugixml failed: ${result}")
		set(PUGIXML_ERROR TRUE)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} --build .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pugixml-download
	)
	if(result)
		message(STATUS "Build step for pugixml failed: ${result}")
		set(PUGIXML_ERROR TRUE)
	endif()

	if(PUGIXML_ERROR)
		message(FATAL_ERROR "Cannot download pugixml repository")
	else()
		target_include_directories(${NCPROJECT_EXE_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME}/src)
		target_sources(${NCPROJECT_EXE_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${PUGIXML_SOURCE_DIR_NAME}/src/pugixml.cpp)
		target_compile_definitions(${NCPROJECT_EXE_NAME} PRIVATE "PUGIXML_NO_XPATH" "PUGIXML_NO_STL" "PUGIXML_NO_EXCEPTIONS")
	endif()
endif()
