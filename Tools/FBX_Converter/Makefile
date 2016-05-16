UNAME := $(shell uname)
COMPILER := clang++ -Ifbx_sdk/include -Lfbx_sdk/lib/linux/x64 -ldl -lfbxsdk

ifeq ($(UNAME), Darwin)
COMPILER := clang++ --stdlib=libc++ -ldl -framework CoreFoundation
FBX_INCLUDE := -I"/Applications/Autodesk/FBX SDK/2015.1/include"
FBX_LIB := "/Applications/Autodesk/FBX SDK/2015.1/lib/clang/release/libfbxsdk.a"
endif

all:
	$(COMPILER) -DRENDERER_NULL -g -std=c++11 $(FBX_LIB) convert_fbx.cpp ../../Engine/Core/Matrix.cpp $(FBX_INCLUDE) -I../../Engine/Include -o convert_fbx

