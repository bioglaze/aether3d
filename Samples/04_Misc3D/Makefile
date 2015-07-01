UNAME := $(shell uname)
COMPILER := g++
ENGINE_LIB := libaether3d_linux.a
LINKER := -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lX11 -lGL -lopenal
LIB_PATH := -L.

ifeq ($(UNAME), Darwin)
COMPILER := clang++ --stdlib=libc++ -DAETHER3D_UNIX
ENGINE_LIB := libaether3d_osx.a
LINKER := -framework IOKit -framework OpenAL -framework Cocoa -framework OpenGL -framework QuartzCore
LIB_PATH := -L.
endif

ifeq ($(OS),Windows_NT)
ENGINE_LIB := libaether3d_win.a
LINKER := -lOpenAL32 -lOpenGL32 -lgdi32
LIB_PATH := -L../../Engine/ThirdParty/lib
endif

WARNINGS := -pedantic -Wall -Wextra

all:
ifeq ($(OS),Windows_NT)
	IF exist ..\..\..\aether3d_build\Samples ( echo building ) ELSE ( mkdir ..\..\..\aether3d_build\Samples )
endif
ifneq ($(OS),Windows_NT)
	mkdir -p ../../../aether3d_build/Samples
endif
	$(COMPILER) $(WARNINGS) $(LIB_PATH) -I../../Engine/Include Misc3D.cpp -std=c++11 ../../../aether3d_build/$(ENGINE_LIB) -o ../../../aether3d_build/Samples/Misc3D $(LINKER)
