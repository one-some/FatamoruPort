#include "audio.h"

AudioTrack load_track(char* path) {
    return (AudioTrack) {
        .valid = true,
        .resource = LoadSound(path)
    };
}

void play_sound(AudioTrack* track) {
    if (!track->valid) return;
    PlaySound(track->resource);
}

