# shex: SHader + HEX
Hex viewer for Linux and FreeBSD using Vulkan via GLFW3

This GUI program is written in C++ and requires hardware that supports Vulkan.
It depends on vulkan and glfw3

To build: type inside the shex/ directory:

make

If you modify the shaders/shader.frag, or shaders/shader.vert then you will need to rebuild it.
Just type:

make shaders

after that shaders/frag.spv and shaders/vert.spv will be regenerated.
There is no need to rebuild an executable after shader changes.

To test: run the executable file with any binary file as parameter

./shex shaders/frag.spv

There is a windows build target, which allows to cross-compile a
windows binary on linux or FreeBSD. I'm not sure it's working, but if you want to test
it, type this in the shex/ directory:

make win

You might need to put these files in the same directory as shex.exe:
glfw3.dll
libwinpthread-1.dll
vulkan-1.dll
libssp-0.dll

in Arch, those files are located in /usr/x86_64-w64-mingw32/bin/

while compiling under FreeBSD install packages glm, glfw, glslang like this:

pkg install glm glfw glslang

Good Luck and Have Fun!
