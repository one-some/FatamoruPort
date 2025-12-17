#pragma once

#ifndef PLATFORM_3DS
	#define PATH(path) path
	#define DATA_PATH(path) "cache/" path
#else
	#define PATH(path) "romfs:/" path
	#define DATA_PATH(path) "romfs:/" path
#endif

char* find_file(char* storage, const char* patterns[]);
char* find_bgm(char* storage);
char* find_sfx(char* storage);
char* find_image(char* storage);
char* find_script(char* storage);
char* find_font(char* storage);
