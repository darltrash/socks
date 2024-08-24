# This makefile works in this way:
# - Turns package/ into a zip named ./package.bsk
#
# - Creates the ./out/ folder which then populates with
# 	subfolders for each "platform" (defined by out/$NAME/)
#
# - Compiles every .c file from basket into the output folder
#
# - Then links every file together into out/$NAME/socks(.exe) and ./thing(.exe)
#
# - If targetting Windows, copies over basket/lib/bin/*.dll to out/$NAME/
#
# - Copies ./package.bsk to out/$NAME/

# Use all cores
CORES = $(shell getconf _NPROCESSORS_ONLN)
MAKEFLAGS += -j$(CORES)

# Platform stuff! If you need to port this to another platformn
# you could do it by overriding the following variables:
NAME := $(shell uname -sm | sed 's/ /-/g' | tr '[:upper:]' '[:lower:]')
PRETTYNAME ?= $(NAME)
CC ?= gcc
LINKER ?= $(CC)
STRIP ?= strip
PKG-CONFIG ?= pkg-config
DEPEND ?= $(shell $(PKG-CONFIG) --static --libs sdl2)

# Defaults to debug builds, optimizes when MODE=RELEASE
CFLAGS := -O0 -ggdb -DTRASH_DEBUG=1

ifeq ($(MODE),RELEASE)
NAME := $(NAME)-release
CFLAGS := -Ofast -ffast-math -ftree-vectorize
else
ifdef PROFILER
CFLAGS += -pg
endif
endif

# Our compartmentalized output folder, full of .o's and your
# output binary
OUT_PATH ?= out
OUT := $(OUT_PATH)/$(NAME)/

# Source files and object files
SRCS := $(wildcard basket/*.c basket/lib/*.c)
OBJS := $(patsubst %.c, $(OUT)%.o, $(SRCS))
OUTS := $(foreach FILE, $(subst /,_, $(OBJS)), $(OUT)$(FILE))

# Builds the game for the platform described in the above variables
build: pack $(OBJS)
	$(shell mkdir -p $(OUT))
	$(LINKER) $(CFLAGS) $(OBJS) -o $(OUT)socks $(DEPEND)
	@cp package.bsk $(OUT)
	rm -rf $(OUT)/*.zip
	@cp $(OUT)socks* .
	cd $(OUT); zip -9 socks-$(PRETTYNAME).zip * -x *.zip basket/

# Wraps over Make, overriding the platform variables
mingw32-64:
	$(shell mkdir -p $(OUT_PATH)/mingw32-64)
	cp basket/lib/bin/* $(OUT_PATH)/mingw32-64/

	make \
	NAME=mingw32-64 \
	PRETTYNAME=windows-x64_64 \
	MODE=$(MODE) \
	CC=x86_64-w64-mingw32-gcc \
	STRIP=x86_64-w64-mingw32-strip \
	PKG-CONFIG=x86_64-w64-mingw32-pkg-config \
	build

SHADER_DIR = basket/shaders/
shader: $(SHADER_DIR)/*
	@rm -rf basket/shaders.h
	for file in $^ ; do \
	   xxd -i $${file} >> basket/shaders.h; \
	done

# Packs the assets
pack:
	@cd package; zip -9 ../package.bsk -r *

# Release both
release:
	make MODE=RELEASE build
	make MODE=RELEASE mingw32-64

force-shader-rebuild:
	@rm $(OUT)/basket/renderer.o

# Runs the project, sacrificing runtime performance for comptime performance
TCC_FLAGS = \
	-DTRASH_DEBUG -DFS_NAIVE_FILES \
	-DSDL_DISABLE_IMMINTRIN_H \
	-DCUTE_SOUND_SCALAR_MODE -DSTBI_NO_SIMD
quick-run:
	tcc $(SRCS) $(TCC_FLAGS) -o thing $(DEPEND)
	./socks

# Builds and runs the project
run: build
	./socks
	@cp package/gmon.out .

# Pattern rule for compiling source files into object files
$(OUT)%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS)

PREFIX ?= /usr
install: build
	@install -D $(OUT)/socks $(PREFIX)/bin/io.itch.darltrash.sleepyhead
	@install -D $(OUT)/package.bsk $(PREFIX)/bin/package.bsk
	@install -D platform/io.itch.darltrash.sleepyhead.desktop $(PREFIX)/share/applications/io.itch.darltrash.sleepyhead.desktop
	@install -D platform/io.itch.darltrash.sleepyhead.svg $(PREFIX)/share/icons/io.itch.darltrash.sleepyhead.svg

.PHONY: clean

# mint basquiat
