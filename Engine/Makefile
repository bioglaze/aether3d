UNAME := $(shell uname)
COMPILER := g++
ENGINE_LIB := libaether3d_linux.a
STD_LIB := -std=c++11

ifeq ($(UNAME), Darwin)
COMPILER := clang++
STD_LIB := -std=c++11 -stdlib=libc++
ENGINE_LIB := libaether3d_osx.a
endif

WARNINGS := -Wall -ansi -pedantic

all:
	mkdir -p ../../aether3d_build
	$(COMPILER) -IInclude $(WARNINGS) $(STD_LIB) -c Core/System.cpp -o ../../aether3d_build/System.o
	$(COMPILER) -IInclude $(WARNINGS) $(STD_LIB) -c Video/WindowOSX_GL.mm -o ../../aether3d_build/Window.o
	ar rcs ../../aether3d_build/$(ENGINE_LIB) ../../aether3d_build/*.o

