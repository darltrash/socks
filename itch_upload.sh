#!/bin/bash

set -e

distrobox enter debian-10 -- ./build.lua release
#unzip socks-win64.zip
butler push ./out/socks-win64 darltrash/sleepyhead:win64
butler push ./out/socks-lin64 darltrash/sleepyhead:linux