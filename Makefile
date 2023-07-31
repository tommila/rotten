# Directories
THIRD_PARTY_DIR := ./third_party
SDL2_DIR := $(THIRD_PARTY_DIR)/SDL2
IMGUI_DIR := $(THIRD_PARTY_DIR)/imgui
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

# List of source files
SRCS := ./src/game/rotten_nuklear.cpp ./src/sdl_platform.c

# Generate the list of object files
OBJS := $(SRCS:.c=.o)

.PHONY: all platform game

all: sokol_renderer platform game

sokol_renderer: ./src/core/sokol_renderer.c
	echo "locking renderer.so" > ./build/librenderer.lock
	$(CC) $< $(CFLAGS) $(DEFINES) -rdynamic -shared -o ./build/librenderer_sokol.so
	rm ./build/librenderer.lock

platform: ./src/sdl_platform.c
	$(CC) $< $(CFLAGS) $(DEFINES) $(SDL2_FLAGS) -o ./build/rottenmob $(SDL2_LIB) -lGL -lGLEW -lm -ldl

game: ./src/game/rotten.cpp ./src/game/rotten_nuklear.o
	echo "locking game.so" > ./build/librottengame.lock

	$(CXX) $< $(CFLAGS_CGLM) $(CXXFLAGS) -rdynamic -shared ./src/game/rotten_nuklear.o -o ./build/librottengame.so
	rm ./build/librottengame.lock

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f ./src/ext/sokol_gfx.o ./src/game/rotten_nuklear.o ./build/rottenmob
