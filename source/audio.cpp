#include "audio.h"

#define update_sound_source(source) { alSourcef((source)->id, AL_GAIN, (source)->volume * audio_type_volume((source)->volume_type) * audio_type_volume_modifiers[(source)->volume_type]); }
#define get_source_position(source, x, y, z) { alGetSource3f((source)->id, AL_POSITION, x, y, z); }
#define set_source_pitch(source, pitch) { alSourcef((source)->id, AL_PITCH, pitch); }
#define set_source_relative(source, relative) { alSourcei((source)->id, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE); }
#define set_source_position(source, x, y, z) { alSource3f((source)->id, AL_POSITION, x, y, z); }
#define set_source_velocity(source, x, y, z) { alSource3f((source)->id, AL_VELOCITY, x, y, z); }
#define set_listener_position(x, y, z) { alListener3f(AL_POSITION, x, y, z); }
#define set_listener_velocity(x, y, z) { alListener3f(AL_VELOCITY, x, y, z); }

global SoundSource sound_sources[SOUND_SOURCE_COUNT];
global r32 audio_type_volume_modifiers[MAX_AUDIO] = { 1, 1, 1 };

i8 source_playing(SoundSource *source) {
    ALint state = 0;
    alGetSourcei(source->id, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

SoundSource *reserve_sound_source() {
    for(i16 i = 0; i < SOUND_SOURCE_COUNT; i++) {
        if(!source_playing(sound_sources + i) && !sound_sources[i].reserved) {
            sound_sources[i].reserved = 1;
            return sound_sources + i;
        }
    }
    return NULL;
}

void unreserve_sound_source(SoundSource *s) {
    s->reserved = 0;
    set_source_relative(s, 1);
    set_source_position(s, 0, 0, 0);
}

SoundSource *play_sound(Sound *sound, r32 volume, r32 pitch, i8 loop, i8 volume_type) {
    if(sound->id) {
        for(i16 i = 0; i < SOUND_SOURCE_COUNT; i++) {
            if(!source_playing(sound_sources + i) && !sound_sources[i].reserved) {
                sound_sources[i].volume_type = volume_type;
                sound_sources[i].volume = volume;
                ALint buffer;
                alGetSourcei(sound_sources[i].id, AL_BUFFER, &buffer);
                if((ALuint)buffer != sound->id) {
                    alSourcei(sound_sources[i].id, AL_BUFFER, sound->id);
                }
                alSourcei(sound_sources[i].id, AL_LOOPING, (ALint)loop);
                alSourcef(sound_sources[i].id, AL_GAIN, volume * audio_type_volume(volume_type));
                alSourcef(sound_sources[i].id, AL_PITCH, pitch);
                set_source_relative(sound_sources+i, 1);
                set_source_position(sound_sources+i, 0, 0, 0);
                alSourcePlay(sound_sources[i].id);
                sound_sources[i].sound = sound;

                return sound_sources + i;
            }
        }
    }
    return NULL;
}

void play_source(SoundSource *source, Sound *sound, r32 volume, r32 pitch, i8 loop, i8 volume_type) {
    if(source && sound->id) {
        source->volume_type = volume_type;
        source->volume = volume;
        ALint buffer;
        alGetSourcei(source->id, AL_BUFFER, &buffer);
        if((ALuint)buffer != sound->id) {
            alSourcei(source->id, AL_BUFFER, sound->id);
        }
        alSourcei(source->id, AL_LOOPING, (ALint)loop);
        alSourcef(source->id, AL_GAIN, volume * audio_type_volume(volume_type));
        alSourcef(source->id, AL_PITCH, pitch);
        alSourcePlay(source->id);
        source->sound = sound;
    }
}

void stop_source(SoundSource *source) {
    alSourceStop(source->id);
    source->sound = NULL;
    source->volume = 0;
}

void stop_sound(Sound *sound) {
    for(i16 i = 0; i < SOUND_SOURCE_COUNT; i++) {
        if(source_playing(&sound_sources[i]) && sound_sources[i].sound == sound) {
            stop_source(&sound_sources[i]);
        }
    }
}

i8 sound_playing(Sound *sound) {
    for(i8 i = 0; i < SOUND_SOURCE_COUNT; i++) {
        if(source_playing(&sound_sources[i]) && sound_sources[i].sound == sound) {
            return 1;
        }
    }
    return 0;
}

r32 get_source_volume(SoundSource *source) {
    r32 val;
    alGetSourcef(source->id, AL_GAIN, &val);
    return val;
}

r32 get_source_pitch(SoundSource *source) {
    r32 val;
    alGetSourcef(source->id, AL_PITCH, &val);
    return val;
}

void set_source_volume(SoundSource *source, r32 volume) {
    source->volume = volume;
    update_sound_source(source);
}

SoundSource init_sound_source() {
    SoundSource s;
    s.reserved = 0;
    s.volume_type = 0;
    alGenSources(1, &s.id);
    alSourcef(s.id, AL_ROLLOFF_FACTOR, 1);
    alSourcef(s.id, AL_REFERENCE_DISTANCE, 256);
    alSourcef(s.id, AL_MAX_DISTANCE, 1024);
    s.sound = NULL;
    return s;
}

void clean_up_sound_source(SoundSource *s) {
    stop_source(s);
    alDeleteSources(1, &s->id);
    s->id = 0;
}

void init_audio() {
    for(i16 i = 0; i < SOUND_SOURCE_COUNT; i++) {
        sound_sources[i] = init_sound_source();
    }
}

void clean_up_audio() {
    for(i16 i = 0; i < SOUND_SOURCE_COUNT; i++) {
        clean_up_sound_source(sound_sources + i);
    }
}

void update_audio() {
    for(i16 i = 0; i < SOUND_SOURCE_COUNT; i++) {
        if(source_playing(&sound_sources[i]) && sound_sources[i].sound) {
            update_sound_source(&sound_sources[i]);
        }
        else {
            sound_sources[i].sound = NULL;
        }
    }
}
