shaders_src = [
	'triangle.frag',
	'triangle.vert']

shaders = []
glslang = find_program('glslangValidator', required: false)

if glslang.found()
	bth_sub = subproject('bintoheader')
	bth_exe = bth_sub.get_variable('bintoheader_executable')

	foreach shader : shaders_src
		spv = custom_target(
			shader.join('_spv'),
			input: shader,
			output: shader.join('.spv'),
			command: [glslang, '-V', '@INPUT@', '-o', '@OUTPUT@'])

		header = custom_target(
			shader.join('_header'),
			input: shader.join('.spv'),
			output: shader.join('.h'))
			command: [bth_exe, '-i', '@INPUT@', '-o', '@OUTPUT@', '-s32'])

		shaders += [header]
	endforeach
else
	message('No glslangValidator, cannot rebuild shaders from source')
endif
