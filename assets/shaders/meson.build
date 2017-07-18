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
			shader + '_spv',
			output: shader + '.spv',
			input: shader,
			command: [glslang, '-V', '@INPUT@', '-o', '@OUTPUT@'])

		output = files('compiled/' + shader + '.h')
		name = shader.underscorify() + '_data'
		header = custom_target(
			shader + '_header',
			output: shader + '.h', # actually a dummy output file
			depends: spv,
			command: [bth_exe, spv.full_path(), output, '-size', '32', '-name', name])

		shaders += [header, output]
	endforeach
else
	message('No glslangValidator, cannot rebuild shaders from source')
endif
