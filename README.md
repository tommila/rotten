# rotten
Cross-platform game created with C from scratch.
The motivation of this project is to learn what it takes to create a cross platform game without using game framework libraries.
I chose C because it is less abstract than more modern languages, primarily because I want to understand how memory operates at lower levels.

This project is a work in progress and will be subject to changes.

## Architecture
Rotten compiles two binaries, a platform layer binary and a game library binary. The platform library encompasses all os related functionalities such as file loading, rendering, input reading etc. The game library encompasses only the game related stuff and communicates with the platform through an api. This split (hopefully) enables the swapping of either library without breaking the other.

## Libraries
- bgfx
- SDL2
- ImGui
- stb_image
- cgltf
- cglm

## Requirements
- libfreetype6-dev x11proto-core-dev libx11-dev libgl1-mesa-dev libgles2-mesa-dev libdrm-dev libgbm-dev libudev-dev libasound2-dev liblzma-dev libjpeg-dev libtiff-dev libwebp-dev git build-essential

## Building
Rotten is constructed using the Make tool and is divided into two separate Makefiles: `Makefile` is responsible for building the platform and game libraries, while `Makefile.third_party` handles the construction of all third-party libraries.
Currently only linux build is supported, but should compile to windows with minimal effort.
```
git submodule update --init
mkdir build

make -f makefile.third_party
make

./build_assets.sh

cd build
./rottenmob
```

## Tools
- `build-assets.sh` Copies all asset files from assets to build/assets and also compiles all the shaders.
- `build-shaders.sh` Compiles all shaders with bgfx shaderc compiler.

Both tools support daemon mode via user argument "-d" or "--daemon", meaning that all modified or copied assets in `assets/` folder are copied/build to `build/assets` folder.

## Features
- Game code hot-reload:
  Loads changed game code library (librottengame.so) during runtime
- Asset hot-reload:
  Updates changed model/texture/shader data during runtime, handy if build-assets.sh is running in daemon mode.
- Basic input
- wasd + wheel for zoom
- P for pause
- G for show/hide grid draw
- glTf model importing
- Mesh & Instanced Mesh rendering
