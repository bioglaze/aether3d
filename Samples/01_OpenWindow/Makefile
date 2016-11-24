UNAME := $(shell uname)
COMPILER ?= g++
ENGINE_LIB := libaether3d_gl_linux.a
LINKER := -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lX11 -lGL -lopenal
VULKAN_LINKER := -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lX11 -lvulkan -lopenal
LIB_PATH := -L. -L/home/glaze/Downloads/VulkanSDK/1.0.33.0/x86_64/lib/

ifeq ($(UNAME), Darwin)
COMPILER := clang++ --stdlib=libc++ -DAETHER3D_UNIX
ENGINE_LIB := libaether3d_gl_osx.a
LINKER := -framework IOKit -framework OpenAL -framework Cocoa -framework OpenGL -framework QuartzCore
LIB_PATH := -L.
endif

ifeq ($(OS),Windows_NT)
ENGINE_LIB := libaether3d_gl_win.a
LINKER := -lOpenAL32 -lOpenGL32 -lgdi32
LIB_PATH := -L../../Engine/ThirdParty/lib
endif

WARNINGS := -g -Wpedantic -Wall -Wextra

all:
ifeq ($(OS),Windows_NT)
	IF exist ..\..\..\aether3d_build\Samples ( echo building ) ELSE ( mkdir ..\..\..\aether3d_build\Samples )
endif
ifneq ($(OS),Windows_NT)
	mkdir -p ../../../aether3d_build/Samples
endif
	$(COMPILER) -DRENDERER_OPENGL $(WARNINGS) $(LIB_PATH) -I../../Engine/Include OpenWindow.cpp -std=c++11 ../../../aether3d_build/$(ENGINE_LIB) -o ../../../aether3d_build/Samples/OpenWindow_gl $(LINKER)

vulkan:
	mkdir -p ../../../aether3d_build/Samples
	$(COMPILER) -DRENDERER_VULKAN $(WARNINGS) $(LIB_PATH) -I../../Engine/ThirdParty -I../../Engine/Include OpenWindow.cpp -std=c++11 ../../../aether3d_build/libaether3d_linux_vulkan.a -o ../../../aether3d_build/Samples/OpenWindow_vulkan $(VULKAN_LINKER)
