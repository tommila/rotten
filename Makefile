# Directories
THIRD_PARTY_DIR := ./third_party
SDL2_DIR := $(THIRD_PARTY_DIR)/SDL2
IMGUI_DIR := $(THIRD_PARTY_DIR)/imgui
CGLM_DIR := $(THIRD_PARTY_DIR)/cglm

# Compiler and flags
CXX := gcc
CXXFLAGS := -g3 -Wall -Wextra
DEFINES := -Dlinux -DSDL_VIDEO_DRIVER_X11

# PKG_CONFIG paths
PKG_CONFIG_PATH := $(SDL2_DIR)/build/lib/pkgconfig/sdl2.pc
PKG_CONFIG := pkg-config

# CFLAGS for CGLM
CFLAGS_CGLM := -I$(THIRD_PARTY_DIR)/cglm/include

# Third-party include paths
THIRD_PARTY_INCLUDE := \
	$(CFLAGS_CGLM) \
	-I$(THIRD_PARTY_DIR)/bgfx/include \
	-I$(THIRD_PARTY_DIR)/bx/include \
	-I$(THIRD_PARTY_DIR)/SDL2/build/include \
	-I$(THIRD_PARTY_DIR)/SDL2_image/build/include \
	-I$(THIRD_PARTY_DIR)/imgui

# Third-party libraries
THIRD_PARTY_LIBS := \
	-lstdc++ \
	-L$(THIRD_PARTY_DIR)/bgfx/.build/linux64_gcc/bin -l:libbgfxDebug.a \
	-L$(THIRD_PARTY_DIR)/bgfx/.build/linux64_gcc/bin -l:libbxDebug.a \
	-L$(THIRD_PARTY_DIR)/bgfx/.build/linux64_gcc/bin -l:libbimgDebug.a \
	-L$(THIRD_PARTY_DIR)/imgui/build -l:libImGui.a

.PHONY: all platform game

all: platform game

platform: ./src/platform.cpp
	$(CXX) $< $(CXXFLAGS) $(DEFINES) $(THIRD_PARTY_INCLUDE) $(THIRD_PARTY_LIBS) $(shell $(PKG_CONFIG) $(PKG_CONFIG_PATH) --cflags --libs sdl2) -lm -Wl,-rpath=$(THIRD_PARTY_DIR)/imgui/build -o ./build/rottenmob

game: ./src/game/rotten.cpp
	$(CXX) $< -fPIC -rdynamic -shared $(THIRD_PARTY_INCLUDE) $(CXXFLAGS) $(CFLAGS_CGLM) -o ./build/librottengame.so
