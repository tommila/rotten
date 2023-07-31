# Directories
THIRD_PARTY_DIR := ./third_party
CGLM_DIR := $(THIRD_PARTY_DIR)/cglm

# Compiler and flags
CXX := g++
CC := gcc
CXXFLAGS := -fPIC -g3 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wno-missing-braces -Wunused-function
CFLAGS := -fPIC -g3 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wno-missing-braces
DEFINES := -Dlinux -DSDL_VIDEO_DRIVER_X11

.PHONY: all platform game

all: opengl_renderer platform game

opengl_renderer: ./src/core/opengl_renderer.c
	echo "locking renderer.so" > ./build/librenderer.lock
	$(CC) $< $(CFLAGS) $(DEFINES) -rdynamic -shared -o ./build/librenderer.so
	rm ./build/librenderer.lock

game: ./src/game/car_game.cpp ./src/common/rotten_nuklear.o
	echo "locking game.so" > ./build/librottengame.lock

	$(CXX) $< $(CXXFLAGS) -rdynamic -shared ./src/common/rotten_nuklear.o -o ./build/librottengame.so
	rm ./build/librottengame.lock

platform: ./src/sdl_platform.c
	$(CC) $< $(CFLAGS) $(DEFINES) -o ./build/rottenmob -Wl,-rpath,./ -lGL -lm -ldl `sdl2-config --cflags --libs`
	echo "./rottenmob ./librottengame.so ./librenderer.so" > ./build/run.sh
	chmod +x ./build/run.sh

clean:
	rm -f ./src/common/rotten_nuklear.o ./build/rottenmob ./build/librenderer.so ./build/librottengame.so
