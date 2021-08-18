find_program(GLSLC NAMES glslc REQUIRED)

set(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders/)

file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/shaders/*.vert ${CMAKE_CURRENT_LIST_DIR}/shaders/*.frag)

set(shaders_hpp ${BINARY_DIR}/include/shaders.hpp)
file(WRITE ${shaders_hpp} "#pragma once\n#include <vector>\n#include <cstdint>\nnamespace Render::Shaders {\n")

foreach(glsl_file ${SHADERS})
	file(RELATIVE_PATH rel_path ${CMAKE_CURRENT_LIST_DIR}/shaders/ ${glsl_file})
	set(spv_file ${BINARY_DIR}/${rel_path})
	set(dep_file ${BINARY_DIR}/${rel_path}.d)
	set(cpp_file ${BINARY_DIR}/${rel_path}.cpp)
	add_custom_command(
		COMMAND ${GLSLC}
			${glsl_file} -o ${spv_file}
			--target-env=opengl -mfmt=c
			-MD -MF ${dep_file}
		
		OUTPUT ${spv_file}
		DEPENDS ${glsl_file}
		DEPFILE ${dep_file}
	)
	string(REGEX REPLACE "[^a-zA-Z0-9]" "_" name ${rel_path})
	file(APPEND ${shaders_hpp} "extern const std::vector<uint32_t> ${name};\n")
	file(WRITE ${cpp_file}_start "#include \"shaders.hpp\"\nconst std::vector<uint32_t> Render::Shaders::${name} = \n")
	file(WRITE ${cpp_file}_end ";\n")
	add_custom_command(
		COMMAND ${CMAKE_COMMAND} -E cat ${cpp_file}_start ${spv_file} ${cpp_file}_end > ${cpp_file}
		OUTPUT ${cpp_file}
		DEPENDS ${spv_file} ${cpp_file}_start ${cpp_file}_end
	)
	set(SPIRV_SHADERS ${SPIRV_SHADERS} ${cpp_file})
endforeach()

file(APPEND ${shaders_hpp} "}\n")

add_library(shaders ${SPIRV_SHADERS} ${shaders_hpp})
target_include_directories(shaders PUBLIC ${BINARY_DIR}/include)

set_property(TARGET shaders PROPERTY CXX_STANDARD 20)
set_property(TARGET shaders PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET shaders PROPERTY CXX_EXTENSIONS OFF)



