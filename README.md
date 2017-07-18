Vulkan MSAA Triangle
====================

Just a small example showing how to effectively use [ny](https://github.com/nyorain/ny),
[vpp](https://github.com/nyorain/vpp) and [bintoheader](https://github.com/nyorain/bintoheader) to
create a structured cross-platform msaa triangle in vulkan without too much boilerplate.

The main structure can be easily extended and already separates window and input logic from
the rendering logic. It could also be easily extended by application- or game-logic as
third component.

One can toggle between {1, 2, 4, 8} samples by using the associated keyboard keys.

Everything is brought together using meson, building it will download the dependencies automatically.
Requires a solid C++17 compiler, i.e. only gcc 7 atm (clang 5 soon probably as well, visual studio
might work in a couple of decades as well). Also requires 'glslangValidator' to be in a
binary path where it can be found by meson.
Works on windows and linux (native x11 and wayland support) and could support
android as well (due to the ny-android backend, not tested/developed for it though).
