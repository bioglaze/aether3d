UNAME := $(shell uname)
COMPILER ?= g++
VULKAN_LINKER := -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lX11 -lvulkan -lopenal
LIB_PATH := -L. -L../../Engine/ThirdParty/lib

ifeq ($(OS),Windows_NT)
LIB_PATH := -L../../Engine/ThirdParty/lib -I$(VULKAN_SDK)\Include
endif

WARNINGS := -g -Wpedantic -Wall -Wextra
SANITIZERS := -fsanitize=address,undefined

all:
ifneq ($(OS),Windows_NT)
	mkdir -p ../../../aether3d_build/Samples
	$(COMPILER) -DRENDERER_VULKAN $(WARNINGS) $(LIB_PATH) -I../../Engine/ThirdParty -I../../Engine/Include PBRSample.cpp -std=c++11 ../../../aether3d_build/libaether3d_linux_vulkan.a -o ../../../aether3d_build/Samples/PBRSample_vulkan $(VULKAN_LINKER)
endif
ifeq ($(OS),Windows_NT)
	$(COMPILER) -DRENDERER_VULKAN $(WARNINGS) $(LIB_PATH) -L$(VULKAN_SDK)/Lib -I../../Engine/ThirdParty -I../../Engine/Include PBRSample.cpp -std=c++11 ../../../aether3d_build/libaether3d_win_vulkan.a -o ../../../aether3d_build/Samples/PBRSample_vulkan -lOpenAL32 -lgdi32 -lvulkan-1
endif

