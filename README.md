# Rotten engine
Cross-platform C/C++ game engine with minimal external libraries and game frameworks.

This project is a work in progress and will be subject to changes.

## Libraries
- SDL2
- sokol-gfx
- nuklear ui
- stb_image
- cgltf
- cglm

## Building
Rotten is constructed using the Make tool and is divided into two separate Makefiles: `Makefile` is responsible for building the platform and game libraries, while `Makefile.third_party` handles the construction of all third-party libraries.
Currently only linux build is supported, but should compile to windows with minimal effort. Assets are stripped except shaders.
```
git submodule update --init
mkdir build

make -f makefile.third_party
make

./build_assets.sh

cd build
./rottenmob
```

## Demo projects
### Car game ###
Drive a physically simulated car in a world greated dynamically from heightmap.
[![Cruisin](https://img.youtube.com/vi/tXZvXTm9bbs/0.jpg)](http://www.youtube.com/watch?v=tXZvXTm9bbs)

## Tools
- `build-assets.sh` Copies all asset files from assets to build/assets and also compiles all the shaders.
- `build-shaders.sh` Compiles all shaders (currently just copies all modified shader files)

Both tools support daemon mode via user argument "-d" or "--daemon", meaning that all modified or copied assets in `assets/` folder are copied/build to `build/assets` folder.

## Thinks I want to do
- Opengl Renderer
- UI Libary
- Math libary
- Audio
