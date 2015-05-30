UNAME := $(shell uname)
COMPILER := g++

ifeq ($(UNAME), Darwin)
COMPILER := clang++
endif

all:
	$(COMPILER) -std=c++11 CombineFiles.cpp -o CombineFiles

