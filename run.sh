#!/bin/sh
gcc -o main \
    main.c \
    parse.c \
    -g -O0 \
    -lraylib \
&& ./main
