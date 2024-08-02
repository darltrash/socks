#!/bin/bash

CONTAINER_NAME="basket-debian10"

# Stage 1: Create container.
if [ -z $BASKET_STAGE2 ]; then
    if distrobox list | grep -q "$CONTAINER_NAME"; then
        echo "Container '$CONTAINER_NAME' already exists."

    else
        echo "Creating Debian 10 container named '$CONTAINER_NAME'..."
        distrobox create -i debian:10 -n "$CONTAINER_NAME"
        echo "Container '$CONTAINER_NAME' created successfully."
    fi

    # Start Stage 2
    distrobox-enter -n "$CONTAINER_NAME" -- bash -c "BASKET_STAGE2=1 $0"

    exit 0

else # Stage 2: Inside container
    sudo apt install -y mingw-w64 unzip make gcc libsdl2-dev

    SDL=$(find "/usr/x86_64-w64-mingw32/" -type f -iname "*sdl2.dll" -print -quit)

    # If there's no SDL2.dll installed, we fetch it ourselves!
    if [ -z "$SDL" ]; then
        echo "No SDL2 for mingw found, fetching SDL2!"

        OLD=$PWD

        wget https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-devel-2.30.3-mingw.zip -P /tmp
        cd /tmp
        unzip SDL2-devel-2.30.3-mingw.zip
        sudo cp -a SDL2-2.30.3/x86_64-w64-mingw32/. /usr/x86_64-w64-mingw32/
        rm -rf SDL2-*

        cd $OLD

        SDL=$(find "/usr/x86_64-w64-mingw32/" -type f -iname "*sdl2.dll" -print -quit)

        # If it STILL can't find SDL2, then simply give up.
        if [ -z "$SDL" ]; then
            echo "Fatal error!"
            exit 1
        fi
    fi

    # Compile within the container!
    make MODE=RELEASE OUT_PATH=out/container DEPEND="-lm -lmvec -pthread -lSDL2 -lc -ldl" build
    make MODE=RELEASE OUT_PATH=out/container DEPEND="-L:$(dirname $SDL) -lSDL2" mingw32-64

    # We're done.
    exit 0
fi
