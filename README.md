# Game and engine made from scratch
Cross-platform C/C++ game made from scratch using only minimal set of external libraries.
![Intro](https://github.com/user-attachments/assets/1b34269c-a4df-47c5-82b9-dffaaeda060a)
![Stats](https://github.com/user-attachments/assets/f8d37a63-bacb-4a8c-af5a-f6cb5a49d0df)
![Visual](https://github.com/user-attachments/assets/53997ed6-1026-4fb1-8662-a3cf828ded40)

Current features:
- Opengl renderer
- Audio mixing and engine synthesizer
- Car physics simulation
- UI library
- Math library
- Heightmap terrain generation
- Custom memory handling

Please note that active development happens in a private repository, so this one may not be updated frequently.

## Libraries
I try to use as few external libraries as possible and when I have to I try to use header only libraries.

Current libraries in use:
- SDL2
- stb_image
- stb_vorbis
- cgltf
- cglm

## Building
Building is handled by a simple bash script.
```
mkdir build

# Builds all game binary files
./build.sh

# Builds spesific binary file
./build.sh platform|game|renderer

# Builds SDL2 from source. Before building SDL2 is downloaded from github and extractet to
# local SDL2 folder
./build.sh sdl

# Builds windows game binary files using mingw64 compiler (which should be installed in advance).
./build.sh windows

# Builds a release versio for linux. In practice this means compiling the project in some old
# stable linux distribution thus maximizing compatibility for all linux distributions, in theory
# at leasts. I use docker for this purpose and a debian jessie image which is quite old by now. 
./build.sh release
```

## Running
Excecute the `run.sh|bat` file inside the build folder.

## Hot-reloading
This project supports hot-reloading, which means you can update the game code while the program is still running, without needing to close and restart it. Most changes will take effect immediately, but some updates, like changes to header files, will still require a full program restart. Additionally, the project currently supports asset hot-reloading but only for shader files.

## Tools
- `build-assets.sh <PATH> [--daemon|-d]` Currently all it does is copies all asset files from assets to build/assets. Daemon mode observers file changes and automatically copies modified files. Note that for asset license reasons I've stripped all assets from this repository.

## Next things to work on
- Simplify platform and renderer api.
- More physics and rework joint position solving
- SSE2/SSE3 SIMD
- Shadows
- Particles
