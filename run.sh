#!/bin/sh
gcc -o main \
    main.c \
    parse.c \
    vector.c \
    -g -O0 \
    -lraylib \
&& ./main
