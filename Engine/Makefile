OUTPUT_DIR := ../../aether3d_build

UNAME := $(shell uname)
COMPILER := g++
CCOMPILER := gcc
ENGINE_LIB := libaether3d_linux.a
STD_LIB := -std=c++11
INCLUDES := -IInclude -IVideo -ICore -IThirdParty
WARNINGS := -Wall -pedantic -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization \
 -Wdouble-promotion -Wformat=2 -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-include-dirs \
 -Wshadow -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wtrampolines \
 -Wunsafe-loop-optimizations -Wvector-operation-performance -Wzero-as-null-pointer-constant

ifeq ($(UNAME), Darwin)
COMPILER := clang++
CCOMPILER := clang
STD_LIB := -std=c++11 -stdlib=libc++
WARNINGS := -Wall -ansi -pedantic
ENGINE_LIB := libaether3d_osx.a
endif

DEFINES := -DRENDERER_OPENGL

ifeq ($(OS),Windows_NT)
ENGINE_LIB := libaether3d_win.a
CCOMPILER := gcc
WARNINGS := -Wall -ansi -pedantic
OUTPUT_DIR := ..\..\aether3d_build
endif

all:
ifeq ($(OS),Windows_NT)
	IF exist $(OUTPUT_DIR) ( echo building ) ELSE ( mkdir $(OUTPUT_DIR) )
endif
ifeq ($(UNAME), Darwin)
	mkdir -p $(OUTPUT_DIR)
endif
ifeq ($(UNAME), Linux)
	mkdir -p $(OUTPUT_DIR)
endif
	$(CCOMPILER) $(INCLUDES) $(WARNINGS) -c ThirdParty/GL/glxw.c -o $(OUTPUT_DIR)/glxw.o
	$(CCOMPILER) -c ThirdParty/stb_image.c -o $(OUTPUT_DIR)/stb_image.o
	$(CCOMPILER) -c ThirdParty/stb_vorbis.c -o $(OUTPUT_DIR)/stb_vorbis.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/RendererCommon.cpp -o $(OUTPUT_DIR)/RendererCommon.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/RendererGL.cpp -o $(OUTPUT_DIR)/RendererGL.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/ShaderGL.cpp -o $(OUTPUT_DIR)/ShaderGL.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/Texture2D_GL45.cpp -o $(OUTPUT_DIR)/Texture2D_GL45.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/TextureCubeGL.cpp -o $(OUTPUT_DIR)/TextureCube.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/TextureCommon.cpp -o $(OUTPUT_DIR)/TextureCommon.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/VertexBufferGL.cpp -o $(OUTPUT_DIR)/VertexBufferGL.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/DDSLoader.cpp -o $(OUTPUT_DIR)/DDSLoader.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/Material.cpp -o $(OUTPUT_DIR)/Material.o	
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Components/TransformComponent.cpp -o $(OUTPUT_DIR)/TransformComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Components/SpriteRendererComponent.cpp -o $(OUTPUT_DIR)/SpriteRendererComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Components/AudioSourceComponent.cpp -o $(OUTPUT_DIR)/AudioSourceComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Components/MeshRendererComponent.cpp -o $(OUTPUT_DIR)/MeshRendererComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Components/TextRendererComponent.cpp -o $(OUTPUT_DIR)/TextRendererComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/FileWatcher.cpp -o $(OUTPUT_DIR)/FileWatcher.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/Mesh.cpp -o $(OUTPUT_DIR)/Mesh.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/Font.cpp -o $(OUTPUT_DIR)/Font.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/AudioClip.cpp -o $(OUTPUT_DIR)/AudioClip.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/AudioSystemOpenAL.cpp -o $(OUTPUT_DIR)/AudioSystemOpenAL.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/FileSystem.cpp -o $(OUTPUT_DIR)/FileSystem.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/Matrix.cpp -o $(OUTPUT_DIR)/Matrix.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/Scene.cpp -o $(OUTPUT_DIR)/Scene.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Core/System.cpp -o $(OUTPUT_DIR)/System.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Components/GameObject.cpp -o $(OUTPUT_DIR)/GameObject.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Components/CameraComponent.cpp -o $(OUTPUT_DIR)/CameraComponent.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/GfxDeviceGL45.cpp -o $(OUTPUT_DIR)/GfxDeviceGL45.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/RenderTextureGL.cpp -o $(OUTPUT_DIR)/RenderTextureGL.o
ifeq ($(UNAME), Darwin)
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/WindowOSX_GL.mm -o $(OUTPUT_DIR)/Window.o
endif
ifeq ($(UNAME), Linux)
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/WindowXCB.cpp -o $(OUTPUT_DIR)/Window.o
endif
ifeq ($(OS),Windows_NT)
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/WindowWin32.cpp -o $(OUTPUT_DIR)/Window.o
	$(COMPILER) $(INCLUDES) $(WARNINGS) $(STD_LIB) $(DEFINES) -c Video/WindowWin32GL.cpp -o $(OUTPUT_DIR)/WindowGL.o
endif
	ar rcs $(OUTPUT_DIR)/$(ENGINE_LIB) $(OUTPUT_DIR)/*.o
ifeq ($(UNAME), Linux)
	rm $(OUTPUT_DIR)/*.o
endif
ifeq ($(UNAME), Darwin)
	rm $(OUTPUT_DIR)/*.o
endif


