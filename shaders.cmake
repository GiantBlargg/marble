find_program(GLSLC NAMES glslc REQUIRED)

set(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders/)

add_library(shaders INTERFACE)
target_include_directories(shaders INTERFACE ${BINARY_DIR}/include)

file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/shaders/*.vert ${CMAKE_CURRENT_LIST_DIR}/shaders/*.frag)

foreach(glsl_file ${SHADERS})
	file(RELATIVE_PATH relpath ${CMAKE_CURRENT_LIST_DIR}/shaders/ ${glsl_file})
	set(spv_file ${BINARY_DIR}/${relpath})
	set(depfile ${BINARY_DIR}/${relpath}.d)
	set(hfile ${BINARY_DIR}/${relpath}.h)
	add_custom_command(
		COMMAND ${GLSLC}
			${glsl_file} -o ${spv_file}
			--target-env=opengl -mfmt=c
			-MD -MF ${depfile}
		
		OUTPUT ${spv_file}
		DEPENDS ${glsl_file}
		DEPFILE ${depfile}
	)
	string(REGEX REPLACE "[^a-zA-Z0-9]" "_" name ${relpath})
	file(WRITE ${hfile}_start "const std::vector<uint32_t> ${name} = \n")
	file(WRITE ${hfile}_end ";\n")
	add_custom_command(
		COMMAND ${CMAKE_COMMAND} -E cat ${hfile}_start ${spv_file} ${hfile}_end > ${hfile}
		OUTPUT ${hfile}
		DEPENDS ${spv_file} ${hfile}_start ${hfile}_end
	)
	set(SPIRV_SHADERS ${SPIRV_SHADERS} ${hfile})
endforeach()

set(shaders_h ${BINARY_DIR}/include/shaders.h)
set(shaders_h_start ${BINARY_DIR}/shaders.h_start)
set(shaders_h_end ${BINARY_DIR}/shaders.h_end)
file(WRITE ${shaders_h_start} "#pragma once\n#include <vector>\nnamespace Render::Shaders {\n")
file(WRITE ${shaders_h_end} "}\n")
add_custom_command(
	COMMAND ${CMAKE_COMMAND} -E cat ${shaders_h_start} ${SPIRV_SHADERS} ${shaders_h_end} > ${shaders_h}
	OUTPUT ${shaders_h}
	DEPENDS ${shaders_h_start} ${SPIRV_SHADERS} ${shaders_h_end}
)
target_sources(shaders INTERFACE ${shaders_h})



