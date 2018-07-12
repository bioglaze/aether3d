UNAME := $(shell uname)
COMPILER := g++ -g
ENGINE_LIB := libaether3d_linux_vulkan.a
LIBS := -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lX11 -lvulkan -lopenal

ifeq ($(OS),Windows_NT)
ENGINE_LIB := libaether3d_win_vulkan.a
LIBS := -L../ThirdParty/lib -lOpenAL32 -lOpenGL32 -lgdi32
endif

all:
	$(COMPILER) -DRENDERER_VULKAN -std=c++11 04_Serialization.cpp ../Core/Matrix.cpp -I../Include -o ../../../aether3d_build/Samples/04_Serialization ../../../aether3d_build/$(ENGINE_LIB) $(LIBS)
	$(COMPILER) -DRENDERER_VULKAN -std=c++11 02_Components.cpp ../Core/Matrix.cpp -I../Include -o ../../../aether3d_build/Samples/02_Components ../../../aether3d_build/$(ENGINE_LIB) $(LIBS)
	$(COMPILER) -DRENDERER_VULKAN -std=c++11 03_Simple3D.cpp ../Core/Matrix.cpp -I../Include -o ../../../aether3d_build/Samples/03_Simple3D ../../../aether3d_build/$(ENGINE_LIB) $(LIBS)
ifeq ($(OS),Windows_NT)
	g++ -Wall -march=native -std=c++11 -DRENDERER_VULKAN -DSIMD_SSE3 01_Math.cpp ../Core/Matrix.cpp ../Core/MatrixSSE3.cpp -I../Include -o ../../../aether3d_build/Samples/01_MathSSE
	g++ -Wall -DRENDERER_VULKAN -std=c++11 01_Math.cpp ../Core/Matrix.cpp -I../Include -o ../../../aether3d_build/Samples/01_Math
endif
ifeq ($(UNAME), Linux)
	g++ -DRENDERER_VULKAN -std=c++11 -march=native -fsanitize=address -DSIMD_SSE3 01_Math.cpp ../Core/Matrix.cpp ../Core/MatrixSSE3.cpp -I../Include -o ../../../aether3d_build/Samples/01_MathSSE
	g++ -DRENDERER_VULKAN -std=c++11 -fsanitize=address 01_Math.cpp ../Core/Matrix.cpp -I../Include -o ../../../aether3d_build/Samples/01_Math
endif

