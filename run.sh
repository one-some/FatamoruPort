#!/bin/sh
gcc -o main \
    main.c \
    parse.c \
    vector.c \
	-fsanitize=address \
    -g -O0 \
    -lraylib \
&& ./main
