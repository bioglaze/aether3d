UNAME := $(shell uname)
COMPILER := g++
CCOMPILER := gcc
WARNINGS := -Wall -pedantic -Wextra -Wcast-align -Wctor-dtor-privacy -Wdisabled-optimization -Wdouble-promotion -Wformat=2 -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-include-dirs -Wshadow -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wtrampolines -Wunsafe-loop-optimizations -Wvector-operation-performance -Wzero-as-null-pointer-constant

ifeq ($(UNAME), Darwin)
COMPILER := clang++
CCOMPILER := clang
WARNINGS := -Wall -Wextra -pedantic
endif

all:
	$(CCOMPILER) -c ../../Engine/ThirdParty/stb_image.c -o stb_image.o
	$(COMPILER) $(WARNINGS) -std=c++11 -g -I../../Engine/ThirdParty SDF_Generator.cpp stb_image.o -o SDF_Generator

