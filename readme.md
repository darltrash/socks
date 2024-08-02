```
  SLEEPYHEAD's source code
```

## how to build
in theory this should also work on Windows using Msys2, obviously also in WSL,
but i haven't tested it, at all.

it should only require the sdl2 headers to work (alongside the libc ofc), everything else is already bundled in the project.

this uses pkg-config for the libraries that the program needs, you can skip
that by setting the DEPEND variable to cc arguments, or even set PKG-CONFIG.
(honestly go read the makefile, it could also allow you to try compiling this
to other platforms, which would be NEATO.)

```sh
# debug builds
make build      # native, usually linux
make mingw32-64 # windows (requires mingw32)

# release builds
make MODE=RELEASE build
make MODE=RELEASE mingw32-64
make release                 # both!

# true release builds, this setups a debian 10 distrobox container
# that will link the game against an older glibc, useful for compat.
./release.sh

# generates a slower build of the game, but at a faster rate. useful for
# quick testing.
make quick-run

# builds the game to the native platform, then runs it.
# (compatible with MODE=RELEASE, too)
make run

# cleans up the mess (usually all self contained in out/)
make clear
```

## tools I used:
  - Libresprite
  - VSCodium
  - Bitwig Studio
  - Blender


## special thanks:
  - GOD.
  - RXI for [json.lua](https://github.com/rxi/json.lua), [vec](https://github.com/rxi/vec) and [ini](https://github.com/rxi/ini), huge inspiration to me
  - Sean Barrett for his awesome [STB libraries](https://github.com/nothings/stb)
  - Kikito for [bump.lua](https://github.com/kikito/bump.lua)
  - Oniietzschan for [porting bump.lua to 3D](https://github.com/oniietzschan/bump-3dpd)
  - Shakesoda for [TinyFX](https://github.com/shakesoda/tinyfx), the thing I used to write the renderer
  - Excessive for the [EXM model format and its Blender Addon](https://github.com/excessive/iqm-exm)
  - the Pontifical Catholic University of Rio de Janeiro for [Lua](https://www.lua.org)
  - The Simple DirectMedia Layer Team for [SDL](https://www.libsdl.org/), the base of my engine (and a lot of engines and tools out there)
  - My dad.
  - My mom.
  - My brother.
  - My friends (cyrneko, mike).
