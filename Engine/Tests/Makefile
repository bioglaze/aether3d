UNAME := $(shell uname)
COMPILER := g++ -g
ENGINE_LIB := libaether3d_gl_linux.a
LIBS := -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lX11 -lGL -lopenal

ifeq ($(UNAME), Darwin)
COMPILER := clang++ -fsanitize=address
ENGINE_LIB := libaether3d_gl_osx.a
LIBS := -framework IOKit -framework OpenAL -framework Cocoa -framework OpenGL -framework QuartzCore
endif

ifeq ($(OS),Windows_NT)
ENGINE_LIB := libaether3d_gl_win.a
LIBS := -L../ThirdParty/lib -lOpenAL32 -lOpenGL32 -lgdi32
endif

all:
	$(COMPILER) -DRENDERER_OPENGL -std=c++11 04_Serialization.cpp ../Core/Matrix.cpp -I../Include -o 04_Serialization ../../../aether3d_build/$(ENGINE_LIB) $(LIBS)
	$(COMPILER) -DRENDERER_OPENGL -std=c++11 02_Components.cpp ../Core/Matrix.cpp -I../Include -o 02_Components ../../../aether3d_build/$(ENGINE_LIB) $(LIBS)
	$(COMPILER) -DRENDERER_OPENGL -std=c++11 03_Simple3D.cpp ../Core/Matrix.cpp -I../Include -o 03_Simple3D ../../../aether3d_build/$(ENGINE_LIB) $(LIBS)
ifeq ($(OS),Windows_NT)
	g++ -Wall -march=native -std=c++11 -DRENDERER_OPENGL -DSIMD_SSE3 01_Math.cpp ../Core/MatrixSSE3.cpp -I../Include -o 01_MathSSE
	g++ -Wall -DRENDERER_OPENGL -std=c++11 01_Math.cpp ../Core/Matrix.cpp -I../Include -o 01_Math
else
	clang++ -DRENDERER_OPENGL -std=c++11 -march=native -fsanitize=address -DSIMD_SSE3 01_Math.cpp ../Core/MatrixSSE3.cpp -I../Include -o 01_MathSSE
	clang++ -DRENDERER_OPENGL -std=c++11 -fsanitize=address 01_Math.cpp ../Core/Matrix.cpp -I../Include -o 01_Math
endif
