#!/bin/bash

sudo apt install -y mingw-w64 unzip
wget https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-devel-2.30.3-mingw.zip -P /tmp
cd /tmp
unzip SDL2-devel-2.30.3-mingw.zip
sudo cp -a SDL2-2.30.3/x86_64-w64-mingw32/. /usr/x86_64-w64-mingw32/
rm -rf SDL2-*