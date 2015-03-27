OUTPUT_DIR := ../../aether3d_build

UNAME := $(shell uname)
COMPILER := g++
CCOMPILER := gcc
ENGINE_LIB := libaether3d_linux.a
STD_LIB := -std=c++11
INCLUDES := -IInclude -IVideo -ICore -IThirdParty

ifeq ($(UNAME), Darwin)
COMPILER := clang++
CCOMPILER := clang
STD_LIB := -std=c++11 -stdlib=libc++
ENGINE_LIB := libaether3d_osx.a
endif

WARNINGS := -Wall -ansi -pedantic

all:
	mkdir -p ../../aether3d_build
	$(CCOMPILER) $(INCLUDES) $(WARNINGS) -c ThirdParty/GL/glxw.c -o $(OUTPUT_DIR)/glxw.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/RendererGL.cpp -o $(OUTPUT_DIR)/RendererGL.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/ShaderGL.cpp -o $(OUTPUT_DIR)/ShaderGL.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/Texture2D_GL45.cpp -o $(OUTPUT_DIR)/Texture2D_GL45.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/DDSLoader.cpp -o $(OUTPUT_DIR)/DDSLoader.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Components/TransformComponent.cpp -o $(OUTPUT_DIR)/TransformComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Components/SpriteRendererComponent.cpp -o $(OUTPUT_DIR)/SpriteRendererComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Core/FileWatcher.cpp -o $(OUTPUT_DIR)/FileWatcher.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Core/String.cpp -o $(OUTPUT_DIR)/String.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Core/Scene.cpp -o $(OUTPUT_DIR)/Scene.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Core/System.cpp -o $(OUTPUT_DIR)/System.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Components/GameObject.cpp -o $(OUTPUT_DIR)/GameObject.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Components/CameraComponent.cpp -o $(OUTPUT_DIR)/CameraComponent.o
ifeq ($(UNAME), Darwin)
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/GfxDeviceGL41.cpp -o $(OUTPUT_DIR)/GfxDeviceGL41.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/WindowOSX_GL.mm -o $(OUTPUT_DIR)/Window.o
else
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) -c Video/WindowXCB.cpp -o $(OUTPUT_DIR)/Window.o
endif
	ar rcs $(OUTPUT_DIR)/$(ENGINE_LIB) $(OUTPUT_DIR)/*.o

