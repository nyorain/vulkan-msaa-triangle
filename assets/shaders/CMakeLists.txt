# shaders
# include via "shaders/compiled/..."
function(add_shader name project)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		COMMAND glslangValidator
					-V "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
					-o "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Building spirv shader - ${name}")

	string(REGEX REPLACE \\\. _ array_name ${name})
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/compiled/${name}.h"
		COMMAND ${CMAKE_BINARY_DIR}/external/install/bin/bintoheader
			-i "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
			-o "${CMAKE_CURRENT_SOURCE_DIR}/compiled/${name}.h"
			-s 32
			-n ${array_name}_data
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.spv"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		COMMENT "Generating spirv header - ${name}")


	add_custom_target(shader_${name} ALL
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/compiled/${name}.h")
	add_dependencies(${project} shader_${name})

	if(Depend)
		add_dependencies(shader_${name} bintoheader_ep)
	endif()
endfunction(add_shader)

# shaders to generate
add_shader("triangle.vert" triangle)
add_shader("triangle.frag" triangle)
