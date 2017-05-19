Vulkan MSAA Triangle
====================

Just a small example showing how to effectively use [ny](https://github.com/nyorain/ny),
[vpp](https://github.com/nyorain/vpp) and [bintoheader](https://github.com/nyorain/bintoheader) to
create a structured cross-platform msaa triangle in vulkan without too much boilerplate.

The main structure can be easily extended and already separates window and input logic from
the rendering logic. It could also be easily extended by application- or game-logic as
third component.

Everything is brought together using cmake, building it will download the dependencies automatically.
Requires a solid C++17 compiler, i.e. gcc 7 or clang 4 (with a standard library including <any>).
Works on windows and linux (native x11 and wayland support) and could support
android as well (due to the ny-android backend, not tested/developed for it though).
