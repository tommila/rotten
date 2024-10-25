#!/bin/bash

set -e

########################
######## MINGW #########
########################
if [ "$1" = "windows" ]; then
  
  if [ -d "./third_party/SDL2_Mingw" ]; then 
    echo "(MinGW-w64) SDL2 folder already existing"
  else
    mkdir -p "third_party/SDL2_Mingw"
    curl -L "https://github.com/libsdl-org/SDL/releases/download/release-2.30.8/SDL2-devel-2.30.8-mingw.tar.gz" -o "sdl2.tar.gz"
    tar -xzf "sdl2.tar.gz" -C "./third_party/SDL2_Mingw" --strip-components=1
    rm "sdl2.tar.gz"
  fi
  echo "(MinGW-w64) Compiling..."
  flags="-I/usr/i686-w64-mingw32/include 
  -L/usr/i686-w64-mingw32/lib 
  -Os" 

  rm -rf build_w64
  mkdir -p build_w64/lib
  ## renderer ##
  echo "(MinGW-w64) Compiling librenderer.dll"
  #sem --jobs 4
  /bin/i686-w64-mingw32-gcc ./src/core/opengl_renderer.c $flags -shared -Wl,--subsystem,windows,--out-implib,./build_w64/lib/renderer.a -o ./build_w64/lib/renderer.dll 
  ## game ##
  echo "(MinGW-w64) Compiling libgame.dll"
  #sem --jobs 4
  /bin/i686-w64-mingw32-g++ ./src/game/car_game.cpp $flags -shared -Wl,--subsystem,windows,--out-implib,./build_w64/lib/game.a -o ./build_w64/lib/game.dll 

  ## platform ##
  echo "(MinGW-w64) Compiling rotten_platform"
  #sem --jobs 4
  /bin/i686-w64-mingw32-gcc ./src/sdl_platform.c $flags -Wl,-rpath,./ -I./third_party/SDL2_Mingw/i686-w64-mingw32/include/SDL2 -o ./build_w64/rotten_platform -lopengl32 -lm -lSDL2main -lSDL2 -L./third_party/SDL2_Mingw/i686-w64-mingw32/lib 

  #sem --wait
  echo "rotten_platform.exe lib/game.dll lib/renderer.dll" > build_w64/run.bat
  cp third_party/SDL2_Mingw/i686-w64-mingw32/bin/SDL2.dll ./build_w64/
  cp /usr/i686-w64-mingw32/bin/libwinpthread-1.dll ./build_w64/
  cp /usr/i686-w64-mingw32/bin/libgcc_s_dw2-1.dll ./build_w64/

  cp ./docs/README.txt ./build_w64
  ./build_assets.sh ./build_w64

#################################
######## RELEASE BUILD ##########
#################################
elif [ "$1" = "release" ]; then
  rm -rf "./build_release"
  mkdir -p "./build_release/lib"
  #Figure out how to use user permissions instead of root
  docker build --progress=plain -f ./scripts/release.Dockerfile -t release .
  docker run -v $PWD/src:/usr/src -v $PWD/build_release:/usr/build_release release
  sudo chown -R $(id -un):$(id -gn) $PWD/build_release/*
  cp ./docs/README.txt ./build_release
  ./build_assets.sh ./build_release

############################  
######## SDL BUILD #########
############################
elif [ "$1" = "sdl" ]; then
  if [ -d "./third_party/SDL2" ]; then
    echo "(SDL) SDL2 folder already existing"
  else
    mkdir -p "third_party/SDL2"
    curl -L "https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-2.30.7.tar.gz" \
      -o "sdl2.tar.gz"
          tar -xzf "sdl2.tar.gz" -C "./third_party/SDL2" --strip-components=1
          rm "sdl2.tar.gz"
  fi 
  mkdir -p "./build/lib"
  install_path=$(realpath "./build/lib")
  sdl_install_path=$(realpath "./third_party/SDL2/Dist")
  cd "third_party/SDL2"
  mkdir -p "build"
  cd "build"
  ../configure --disable-esd \
    --disable-arts \
    --disable-video-directfb \
    --disable-rpath \
    --enable-alsa \
    --enable-alsa-shared \
    --enable-pulseaudio \
    --enable-pulseaudio-shared \
    --enable-x11-shared \
    --prefix=$sdl_install_path 
  make -j8
  make install
  find $sdl_install_path -name libSDL2-2.0.so.0 -exec cp {} "$install_path" \;

#########################################
########### GAME FILES BUILD ############
#########################################
else
  flags="-fPIC
  -g3
  -fno-omit-frame-pointer 
  -Wall -Wextra 
  -Wno-missing-field-initializers 
  -Wno-unused-parameter 
  -Wno-unused-function 
  -Wno-missing-braces 
  -Dlinux 
  -DSDL_VIDEO_DRIVER_X11"
  if [ -d "./third_party/SDL2/Dist" ]; then
    echo "Local SDL2 found"
    sdl_install_path=$(realpath "./third_party/SDL2/Dist")
    sdl_flags=$("$sdl_install_path"/bin/sdl2-config --cflags --libs)
  else
    echo "Local SDL2 not found, using system"
    sdl_flags=$(sdl2-config --cflags --libs)
  fi
  touch build/readlock
  echo "$sdl_flags"
  echo "(GCC) Compiling..."
  if [ -z "$1" ] || [ "$1" = "all" ] || [ "$1" = "renderer" ]; then
    ## renderer ##
    echo "(GCC) Compiling librenderer.so"
    #sem --jobs 4
    gcc ./src/core/opengl_renderer.c $flags -std=c11 -rdynamic -shared -o ./build/lib/librenderer.so 
  fi
  ## game ##
  if [ -z "$1" ] || [ "$1" = "all" ] || [ "$1" = "game" ]; then
    echo "(GCC) Compiling libgame.so"
    #sem --jobs 4
    g++ ./src/game/car_game.cpp $flags -rdynamic -std=c++11 -shared -o ./build/lib/libgame.so 
  fi

  ## platform ##
  if [ -z "$1" ] || [ "$1" = "all" ] || [ "$1" = "platform" ]; then
    echo "(GCC) Compiling rotten_platform"
    #sem --jobs 4
    gcc ./src/sdl_platform.c $flags $sdl_flags -std=c11 -o ./build/rotten_platform -Wl,-rpath,'$ORIGIN'/lib -lm 
  fi

  echo "(GCC) Create run script"
  echo "./rotten_platform lib/libgame.so lib/librenderer.so" > ./build/run.sh
  chmod +x ./build/run.sh
  rm build/readlock
fi


