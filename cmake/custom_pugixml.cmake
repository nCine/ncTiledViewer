# Download and unpack pugixml at configure time
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
	target_include_directories(${PACKAGE_EXE_NAME} PRIVATE ${CMAKE_BINARY_DIR}/pugixml-src/src)
	target_sources(${PACKAGE_EXE_NAME} PRIVATE ${CMAKE_BINARY_DIR}/pugixml-src/src/pugixml.cpp)
	target_compile_definitions(${PACKAGE_EXE_NAME} PRIVATE "PUGIXML_NO_XPATH" "PUGIXML_NO_STL" "PUGIXML_NO_EXCEPTIONS")
endif()
