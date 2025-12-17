#!/bin/sh
gcc -o main \
	-Wl,--wrap=printf \
    src/*.c \
    -g -O0 \
    -lraylib \
    -fsanitize=address \
	-D PLATFORM_STUB \
&& ./main
