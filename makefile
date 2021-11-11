all:
	@echo "	CXX main.cpp"
	@${CXX} -std=c++17 -L/usr/local/lib -o shex -DMOUSEWHEELYDIR=-1 main.cpp -I. -I/usr/local/include -lglfw -lvulkan -DNDEBUG


clean:
	@echo "	CLEAN"
	@rm -f ./shex

win:
	@echo "	CXX-win main.cpp"
	@x86_64-w64-mingw32-g++ -DNDEBUG -D_WIN64 -D_WIN32 -std=c++17 -o shex.exe main.cpp -I. -DMOUSEWHEELYDIR=-1 -static-libgcc -static-libstdc++ -mwindows -lvulkan -lglfw3 -lgdi32 -fstack-protector-all -L/usr/local/lib -I/usr/local/include

shaders:
	@echo "	MAKE shaders"
	@cd shaders && make
	@make all

.PHONY: all clean win shaders
