#!/bin/bash

set -e
echo "Hello"
if [ -d "/home/customuser/SDL2" ]; then 
  echo "(SDL) SDL2 folder already existing" 
else 
  mkdir -p /home/customuser/SDL2 
  curl -L "https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-2.30.7.tar.gz" \
    -o "sdl2.tar.gz" 
  tar -xzf "sdl2.tar.gz" -C "./home/customuser/SDL2" --strip-components=1 
  rm "sdl2.tar.gz" 
fi 
cd /home/customuser/SDL2 
mkdir -p build 
cd build 
echo "(SDL) configure"
../configure \
--disable-esd \
--disable-arts \
--disable-video-directfb \
--disable-rpath \
--enable-alsa \
--enable-alsa-shared \
--enable-pulseaudio \
--enable-pulseaudio-shared \
--enable-x11-shared \
--enable-video-opengl \
--prefix=/home/customuser/SDL2/Dist
echo "(SDL) Make"

make -j8
echo "(SDL) Install"
make install
find /home/customuser/SDL2/Dist -name libSDL2-2.0.so.0 -exec cp {} /usr/build_release/lib \;

echo "(GCC) Compiling..."
  flags="-fPIC 
  -Os
  -Dlinux 
  -DSDL_VIDEO_DRIVER_X11"
sdl_flags=$(/home/customuser/SDL2/Dist/bin/sdl2-config --cflags --libs)
#sdl_flags="-I$sdl_install_path/include/SDL2 -l:libSDL2.so"
## renderer ##
echo "$sdl_flags"
echo "(GCC) Compiling librenderer.so"
#sem --jobs 4
gcc /usr/src/core/opengl_renderer.c $flags -std=c11 -rdynamic -shared -o /usr/build_release/lib/librenderer.so 
## game ##
echo "(GCC) Compiling libgame.so"
#sem --jobs 4
g++ /usr/src/game/car_game.cpp $flags -std=c++11 -rdynamic -shared -o /usr/build_release/lib/libgame.so 

## platform ##
echo "(GCC) Compiling rotten_platform"
#sem --jobs 4
gcc /usr/src/sdl_platform.c $flags $sdl_flags -std=c11 -o /usr/build_release/rotten_platform -Wl,-rpath,'$ORIGIN'/lib -lm 

#sem --wait
echo "(GCC) Create run script"
echo "./rotten_platform lib/libgame.so lib/librenderer.so" > /usr/build_release/run.sh
chmod +x /usr/build_release/run.sh

