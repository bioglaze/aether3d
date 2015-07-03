UNAME := $(shell uname)
COMPILER := g++

WARNINGS := -Wall -pedantic -Wextra -Wcast-align -Wctor-dtor-privacy -Wdisabled-optimization \
 -Wdouble-promotion -Wformat=2 -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-include-dirs \
 -Wshadow -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wtrampolines \
 -Wunsafe-loop-optimizations -Wvector-operation-performance -Wzero-as-null-pointer-constant

ifeq ($(UNAME), Darwin)
COMPILER := clang++
WARNINGS := -Wall
endif

all:
	$(COMPILER) $(WARNINGS) -std=c++11 convert_obj.cpp -I../../Engine/Include -o ../../../aether3d_build/convert_obj

