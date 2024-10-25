# Rotten engine
Cross-platform C/C++ game engine with minimal external libraries and frameworks.

This project is a work in progress and will be subject to changes.

## Libraries
- SDL2
- nuklear ui
- stb_image
- cgltf
- cglm

## Building
Rotten uses Make tool for building and it has two separate makefiles, one for linux builds and one for windows via mingw.
Assets are stripped except shaders.

### Linux build and run
```
git submodule update --init
mkdir build
make

./build_assets.sh

cd build
./run.sh
```

### Windows build and run
```
git submodule update --init
mkdir build
make -f Makefile.mingw32

./build_assets.sh

cd build
run.bat
```

## Demo projects
### Desert cruise - tech demo ###
Drive a physically simulated car in a world greated dynamically from heightmap.
[![Cruisin](https://img.youtube.com/vi/bGzOv2R_E20/0.jpg)](https://youtu.be/bGzOv2R_E20)

## Tools
- `build-assets.sh` Copies all asset files from assets to build/assets and also compiles all the shaders.
- `build-shaders.sh` Compiles all shaders (currently just copies all modified shader files)

Both tools support daemon mode via user argument "-d" or "--daemon", meaning that all modified or copied assets in `assets/` folder are copied/build to `build/assets` folder.

## TODO
- Remove nuklear and create own minimal UI libary
- Remove cglm and create own minimal math libary
- Add audio support
