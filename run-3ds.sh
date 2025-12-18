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
mkdir -p 3ds/out
rm -f 3ds/out/*
mkdir -p 3ds/romfs

echo "Compiling C..."
"$CC" $CFLAGS $INCLUDES -c src/*.c -D PLATFORM_3DS -fno-builtin-printf

echo "Linking..."
"$CC" -specs=3dsx.specs -Wl,--wrap=printf $ARCH *.o $LIBPATHS $LIBS -o 3ds/out/output.elf

ICON="cache/fgimage/ヤコポ_左楽.png"
convert $ICON -trim +repage -resize "48x48^" -gravity center -extent 48x48 cache/icon.png
"$DEVKITPRO/tools/bin/smdhtool" --create "The House in Fata Morgana" "Written by Novectacle" "Claire" cache/icon.png 3ds/out/output.smdh

echo "Creating 3DSX..."
"$TOOL3DSX" 3ds/out/output.elf 3ds/out/output.3dsx --romfs=3ds/romfs --smdh=3ds/out/output.smdh

# Cleanup object files
rm *.o

echo "Done comp"

./3ds/bin/azahar.AppImage 3ds/out/output.3dsx
