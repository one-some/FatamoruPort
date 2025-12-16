#!/bin/bash
set -e

CC="$DEVKITARM/bin/arm-none-eabi-gcc"
TEX3DS="$DEVKITPRO/tools/bin/tex3ds"
TOOL3DSX="$DEVKITPRO/tools/bin/3dsxtool"

ARCH="-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft"
CFLAGS="-g -Wall -O2 -mword-relocations -ffunction-sections -D__3DS__ $ARCH"

INCLUDES="-I$DEVKITPRO/libctru/include -I$DEVKITPRO/libcitro2d/include -I$DEVKITPRO/libcitro3d/include -I."
LIBPATHS="-L$DEVKITPRO/libctru/lib -L$DEVKITPRO/libcitro2d/lib -L$DEVKITPRO/libcitro3d/lib"
LIBS="-lcitro2d -lcitro3d -lctru -lm"

echo "Cleaning up..."
mkdir -p out
rm -f out/*
mkdir -p romfs

echo "Compiling C..."
"$CC" $CFLAGS $INCLUDES -c src/*.c -D PLATFORM_3DS

echo "Linking..."
"$CC" -specs=3dsx.specs $ARCH *.o $LIBPATHS $LIBS -o out/output.elf

ICON="$DEVKITPRO/libctru/default_icon.png"
"$DEVKITPRO/tools/bin/smdhtool" --create "The House in Fata Morgana" "Interpreter" "Claire" $ICON out/output.smdh

echo "Creating 3DSX..."
"$TOOL3DSX" out/output.elf out/output.3dsx --romfs=romfs --smdh=out/output.smdh

# Cleanup object files
rm *.o

echo "Done comp"

./bin/azahar.AppImage out/output.3dsx
