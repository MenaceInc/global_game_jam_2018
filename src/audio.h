#ifndef AUDIO_H
#define AUDIO_H

#include "resources.h"

#define SOUND_SOURCE_COUNT 64

struct SoundSource {
    ALuint id;
    i8 reserved, volume_type;
    r32 volume;
    Sound *sound;
};

#endif
