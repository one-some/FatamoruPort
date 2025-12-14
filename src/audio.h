#pragma once

#include "raylib.h"

typedef struct {
    bool valid;
    Sound resource;
} AudioTrack;

typedef struct {
    AudioTrack bgm;
} AudioManager;

AudioTrack load_track(char* path);
void play_sound(AudioTrack* track);
