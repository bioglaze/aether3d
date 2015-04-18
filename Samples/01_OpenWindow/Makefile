UNAME := $(shell uname)
COMPILER := g++
ENGINE_LIB := libaether3d_linux.a
LINKER := -ldl -lxcb -lxcb-keysyms -lxcb-icccm -lX11-xcb -lX11 -lGL

ifeq ($(UNAME), Darwin)
COMPILER := clang++ --stdlib=libc++ -DAETHER3D_UNIX
ENGINE_LIB := libaether3d_osx.a
LINKER := -framework OpenAL -framework Cocoa -framework OpenGL -framework QuartzCore
endif

ifeq ($(OS),Windows_NT)
ENGINE_LIB := libaether3d_win.a
endif

WARNINGS := -pedantic -Wall -Wextra

all:
	mkdir -p ../../../aether3d_build/Samples
	$(COMPILER) $(WARNINGS) -I../../Engine/Include OpenWindow.cpp -std=c++11 ../../../aether3d_build/$(ENGINE_LIB) -o ../../../aether3d_build/Samples/OpenWindow $(LINKER)

