#!/bin/sh
gcc -o main \
    src/*.c \
    -g -O0 \
    -lraylib \
	-D PLATFORM_STUB \
&& ./main
