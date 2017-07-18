project('vk-msaa-triangle', 'cpp',
  license: 'BSL',
	version: '0.1.0',
	meson_version: '>=0.38',
  default_options: ['cpp_std=c++1z'])

rebuild_shaders = true

# default arrguments
# warnings and stuff
add_project_arguments(
	'-Wall',
	'-Wextra',
	'-Wno-unused-parameter',
	'-Wpedantic',
	'-Wno-missing-braces',
	language: 'cpp')

# project-specific stuff
add_project_arguments(
	'-DDLG_BASE_PATH="@0@"'.format(meson.source_root()),
	language: 'cpp')

dep_vpp = dependency('vpp', fallback: ['vpp', 'vpp_dep'])
dep_ny = dependency('ny', fallback: ['ny', 'ny_dep'])
dep_vulkan = meson.get_compiler('cpp').find_library('vulkan')

if rebuild_shaders
	subdir('assets/shaders')
else
	shaders = []
endif

src = [
	shaders,
	'engine.cpp',
	'main.cpp',
	'render.cpp',
	'window.cpp']

executable('triangle', src,
	dependencies: [dep_vpp, dep_vulkan, dep_ny],
	include_directories: include_directories('assets'))
