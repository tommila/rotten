# Use the Debian Jessie base image
FROM debian:jessie
# Create a custom user with UID 1234 and GID 1234
RUN rm /etc/apt/sources.list

RUN echo "deb http://archive.debian.org/debian-security jessie/updates main" >> /etc/apt/sources.list.d/jessie.list

RUN echo "deb http://archive.debian.org/debian jessie main" >> /etc/apt/sources.list.d/jessie.list
# Update the package list and install necessary packages
RUN apt-get update && apt-get -y upgrade && apt-get -y install apt-utils
RUN apt-get install -y --force-yes build-essential curl libasound2-dev libpulse-dev libx11-dev libxext-dev libgl1-mesa-dev libxrandr-dev
WORKDIR /home/customuser
RUN mkdir -p /home/customuser/SDL2 
RUN curl -L "https://github.com/libsdl-org/SDL/releases/download/release-2.30.7/SDL2-2.30.7.tar.gz" \
    -o "sdl2.tar.gz" 
RUN tar -xzf "sdl2.tar.gz" -C "/home/customuser/SDL2" --strip-components=1 
RUN rm "sdl2.tar.gz" 

COPY scripts/compile_release.sh /run/compile_release.sh
CMD ["bash", "/run/compile_release.sh"]

