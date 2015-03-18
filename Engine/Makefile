UNAME := $(shell uname)
COMPILER := g++
ENGINE_LIB := libaether3d_linux.a
STD_LIB := -std=c++11
INCLUDES := -IInclude -IVideo -IThirdParty

ifeq ($(UNAME), Darwin)
COMPILER := clang++
STD_LIB := -std=c++11 -stdlib=libc++
ENGINE_LIB := libaether3d_osx.a
endif

WARNINGS := -Wall -ansi -pedantic

all:
	mkdir -p ../../aether3d_build
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Core/Math.cpp -o ../../aether3d_build/Math.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Core/Scene.cpp -o ../../aether3d_build/Scene.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Core/System.cpp -o ../../aether3d_build/System.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Components/GameObject.cpp -o ../../aether3d_build/GameObject.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Components/CameraComponent.cpp -o ../../aether3d_build/CameraComponent.o
ifeq ($(UNAME), Darwin)
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/GfxDeviceGL41.cpp -o ../../aether3d_build/GfxDeviceGL41.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/WindowOSX_GL.mm -o ../../aether3d_build/Window.o
else
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/WindowXCB.cpp -o ../../aether3d_build/Window.o
endif
	ar rcs ../../aether3d_build/$(ENGINE_LIB) ../../aether3d_build/*.o

