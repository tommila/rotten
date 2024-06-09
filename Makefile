# Directories
THIRD_PARTY_DIR := ./third_party
SDL2_DIR := $(THIRD_PARTY_DIR)/SDL2
CGLM_DIR := $(THIRD_PARTY_DIR)/cglm

# Compiler and flags
CXX := g++
CC := gcc

# PKG_CONFIG paths
PKG_CONFIG_PATH := $(SDL2_DIR)/build/lib/pkgconfig/sdl2.pc
PKG_CONFIG := pkg-config

CXXFLAGS := -fPIC -g3 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wno-missing-braces
CFLAGS := -fPIC -g3 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wno-missing-braces
SDL2_FLAGS := $(shell $(PKG_CONFIG) $(PKG_CONFIG_PATH) --cflags sdl2)
SDL2_LIB := $(shell $(PKG_CONFIG) $(PKG_CONFIG_PATH) --libs sdl2)
DEFINES := -Dlinux -DSDL_VIDEO_DRIVER_X11

# CFLAGS for CGLM
CFLAGS_CGLM := -I$(THIRD_PARTY_DIR)/cglm/include

.PHONY: all platform game

all: sokol_renderer platform game

sokol_renderer: ./src/core/sokol_renderer.c
	echo "locking renderer.so" > ./build/librenderer.lock
	$(CC) $< $(CFLAGS) $(DEFINES) -rdynamic -shared -o ./build/librenderer_sokol.so
	rm ./build/librenderer.lock

platform: ./src/sdl_platform.c
	$(CC) $< $(CFLAGS) $(DEFINES) $(SDL2_FLAGS) -o ./build/rottenmob $(SDL2_LIB) -lGL -lGLEW -lm -ldl

game: ./src/game/car_game.cpp ./src/common/rotten_nuklear.o
	echo "locking game.so" > ./build/librottengame.lock

	$(CXX) $< $(CFLAGS_CGLM) $(CXXFLAGS) -rdynamic -shared ./src/common/rotten_nuklear.o -o ./build/librottengame.so
	rm ./build/librottengame.lock

clean:
	rm -f ./src/common/rotten_nuklear.o ./build/rottenmob ./build/librenderer_sokol.so ./build/librottengame.so
