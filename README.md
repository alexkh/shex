# shex
Hex viewer for Linux using Vulkan via GLFW3

This GUI program is written in C++ and requires hardware that supports Vulkan.
It depends on vulkan and glfw3

To build: type inside the shex/ directory:

./mk

If you modify the shaders/shader.frag, then you will need xxd to rebuild it.
Just type:

./shmk

and the script will re-generate the frag.h and vert.h. You will need to rebuild
the executable after that with ./mk script mentioned above.

To test: run the executable file with any binary file as parameter

./shex shaders/frag.spv

There is a windows build command 'mkwin', which allows to cross-compile a
windows binary on linux. I'm not sure it's working, but if you want to test
it, type this in the shex/ directory:

./mkwin

You might need to put these files in the same directory as shex.exe:
glfw3.dll
libwinpthread-1.dll
vulkan-1.dll
libssp-0.dll

in Arch, those files are located in /usr/x86_64-w64-mingw32/bin/

Good Luck and Have Fun!
