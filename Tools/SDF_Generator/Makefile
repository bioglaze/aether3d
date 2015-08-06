UNAME := $(shell uname)
COMPILER := g++
CCOMPILER := gcc

ifeq ($(UNAME), Darwin)
COMPILER := clang++
CCOMPILER := clang
endif

all:
	$(CCOMPILER) -c ../../Engine/ThirdParty/stb_image.c -o stb_image.o
	$(COMPILER) -pedantic -Wall -Wextra -ansi -std=c++11 -g -I../../Engine/ThirdParty SDF_Generator.cpp stb_image.o -o SDF_Generator

