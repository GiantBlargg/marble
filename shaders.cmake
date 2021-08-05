find_program(glslc_executable NAMES glslc REQUIRED)

add_library(shaders INTERFACE)
target_include_directories(shaders INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include)

file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/shaders/*)

foreach(source ${SHADERS})
	file(RELATIVE_PATH relpath ${CMAKE_CURRENT_LIST_DIR} ${source})
	set(include ${CMAKE_CURRENT_BINARY_DIR}/include/${relpath})
	set(depfile ${CMAKE_CURRENT_BINARY_DIR}/${relpath}.d)
	get_filename_component(depdir ${depfile} DIRECTORY)
	file(MAKE_DIRECTORY ${depdir})
	add_custom_command(
		OUTPUT ${include}
		DEPENDS ${source}
		DEPFILE ${depfile}
		COMMAND
			${glslc_executable}
			--target-env=opengl
			-mfmt=c
			-MD -MF ${depfile}
			-o ${include}
			${source}
	)
	target_sources(shaders INTERFACE ${include})
endforeach()