#CC specifies which compiler we're using
CC = g++

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS = -I/usr/include/SDL2

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lSDL2 -lGL -lGLEW

#This is the target that compiles our executable
all : shex

shex: shex.cpp
	$(CC) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o shex shex.cpp
