add_library(gl INTERFACE)
if(UNIX)
	find_package(OpenGL REQUIRED MODULE COMPONENTS OpenGL)
	target_link_libraries(gl INTERFACE OpenGL::OpenGL)
	target_include_directories(gl INTERFACE ${CMAKE_CURRENT_LIST_DIR}/glvnd/)
else()
	FetchContent_Declare(GLEW
		URL https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip
	)
	FetchContent_GetProperties(GLEW)
	if(NOT glew_POPULATED)
		FetchContent_Populate(GLEW)
		add_subdirectory(${glew_SOURCE_DIR}/build/cmake/ EXCLUDE_FROM_ALL)
	endif()
	target_link_libraries(gl INTERFACE glew_s)
	target_include_directories(gl INTERFACE ${CMAKE_CURRENT_LIST_DIR}/glew/ ${glew_SOURCE_DIR}/include/)
endif()