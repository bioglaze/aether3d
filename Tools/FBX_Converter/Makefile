UNAME := $(shell uname)
COMPILER := clang++ -Ifbx_sdk/include -Lfbx_sdk/lib/linux/x64 -ldl -lfbxsdk

ifeq ($(UNAME), Darwin)
COMPILER := clang++ --stdlib=libc++ -ldl -framework CoreFoundation
FBX_INCLUDE := -I"/Applications/Autodesk/FBX SDK/2018.1.1/include"
FBX_LIB := "/Applications/Autodesk/FBX SDK/2018.1.1/lib/clang/release/libfbxsdk.a"
all:
	$(COMPILER) -DRENDERER_OPENGL -g -std=c++11 $(FBX_LIB) convert_fbx.cpp ../../Engine/Core/Matrix.cpp $(FBX_INCLUDE) -I../../Engine/Include -o convert_fbx
endif

ifeq ($(UNAME), Linux)
COMPILER := g++
FBX_INCLUDE := -I/home/glaze/Downloads/fbx_sdk/include
FBX_LIB := /home/glaze/Downloads/fbx_sdk/lib/gcc4/x64/release/libfbxsdk.a
GCCWARNINGS := -Wall -pedantic -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization \
 -Wdouble-promotion -Wformat=2 -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-include-dirs \
 -Wshadow -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wtrampolines \
 -Wunsafe-loop-optimizations -Wvector-operation-performance -Wuseless-cast -Wformat=2
all:
	$(COMPILER) -DRENDERER_OPENGL -g -std=c++11 convert_fbx.cpp $(FBX_LIB) -lpthread -ldl ../../Engine/Core/Matrix.cpp $(FBX_INCLUDE) -I../../Engine/Include -o convert_fbx
endif

