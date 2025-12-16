#pragma once

typedef struct {
	int x;
	int y;
} RVec2;

typedef struct {
	int x;
	int y;
	int width;
	int height;
} RRect;

typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} RColor;

bool rect_contains(RRect rect, RVec2 point);

#define R_WHITE (RColor) { 0xFF, 0xFF, 0xFF, 0xFF }
#define R_BLANK (RColor) { 0xFF, 0xFF, 0xFF, 0x00 }
