UNAME := $(shell uname)
COMPILER := g++

WARNINGS := -Wall -pedantic -Wextra -Wcast-align -Wctor-dtor-privacy -Wdisabled-optimization \
 -Wdouble-promotion -Wformat=2 -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-include-dirs \
 -Wshadow -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wtrampolines \
 -Wunsafe-loop-optimizations -Wvector-operation-performance -Wzero-as-null-pointer-constant

ifeq ($(UNAME), Darwin)
COMPILER := clang++
WARNINGS := -g -Weverything -Wno-old-style-cast -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic \
-Wno-exit-time-destructors -Wno-float-equal -Wno-unused-macros -Wno-sign-conversion \
-Wno-missing-variable-declarations -Wno-undef -Wno-missing-prototypes -Wno-documentation \
-Wno-implicit-fallthrough -Wno-global-constructors

endif

all:
	$(COMPILER) $(WARNINGS) -std=c++11 convert_obj.cpp -I../../Engine/Include -o ../../../aether3d_build/convert_obj

