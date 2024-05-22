```
  SLEEPYHEAD' source code
```

## how to build
i have personally only tried this in my own linux install so i'm not sure it will even work on anything else, i am deeply sorry

```sh
# for compiling in Linux, you can use other compilers but i havent tried myself, you'll have to set the CC environment variable to your compiler of choice. 

# the default compiler is "gcc" and it'll try to use ccache if installed and env var NO_CCACHE is not defined

# fedora
sudo dnf install -y SDL2-devel ccache gcc luajit unzip zip upx mingw64-gcc mingw64-SDL2

# debian/ubuntu
sudo apt install -y libsdl2-dev ccache gcc luajit zip upx mingw-w64 git
./setup_mingw.sh

# releases for both linux and windows
./build.lua release    # you can also use release_linux and release_windows

# creates a debug binaries for both linux and windows
./build.lua debug      # you can also use debug_linux and debug_windows

# runs mindbasket using tcc (requires tcc), it produces a slower binary but it takes less time to compile.
./build.lua run

# clean up the mess.
./build.lua cleanup

# check out all the available options:
./build.lua help
```

## tools I used:
  - Libresprite
  - VSCodium
  - Bitwig Studio
  - Blender

## special thanks:
  - RXI for [json.lua](https://github.com/rxi/json.lua), [vec](https://github.com/rxi/vec) and [json.lua](https://github.com/rxi/ini), huge inspiration to me
  - Sean Barrett for his awesome [STB libraries](https://github.com/nothings/stb)
  - Kikito for [bump.lua](https://github.com/kikito/bump.lua)
  - Oniietzschan for [porting bump.lua to 3D](https://github.com/oniietzschan/bump-3dpd)
  - Shakesoda for [TinyFX](https://github.com/shakesoda/tinyfx), the thing I used to write the renderer
  - Excessive for the [EXM model format and its Blender Addon](https://github.com/excessive/iqm-exm)
  - the Pontifical Catholic University of Rio de Janeiro for [Lua](https://www.lua.org)
  - The Simple DirectMedia Layer Team for [SDL](https://www.libsdl.org/), the base of my engine (and a lot out there)
  - My dad.
  - My mom.
  - My friends.
  - My God.