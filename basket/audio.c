#include <stdio.h>

#include "lib/AL/al.h"
#include "lib/AL/alc.h"

#define BASKET_INTERNAL
#include "basket.h"

#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_HEADER_ONLY
#include "lib/stb_vorbis.h"


int aud_init() {
    printf("setting up audio\n");

    ALCdevice *device = alcOpenDevice(NULL);
    if (!device) {
        printf("failed to open openal device\n");
        return 1;
    }

    ALCcontext *context = alcCreateContext(device, NULL);
    if (!context) {
        printf("failed to create openal context\n");
        alcCloseDevice(device);
        return 1;
    }

    if (!alcMakeContextCurrent(context)) {
        printf("failed to make openal context current\n");
        alcDestroyContext(context);
        alcCloseDevice(device);
        return 1;
    }

    return 0;
}

int aud_load_ogg(Sound *sound, const u8 *mem, u32 len, bool spatialize) {
    int channels, sample_rate;
    short *decoded_data;
    int samples = stb_vorbis_decode_memory(mem, len, &channels, &sample_rate, &decoded_data);

    if (spatialize) {
        // downmix! 
        // TODO: PORT THIS TO MORE CHANNELS
        if (channels == 2) {
            int o = 0;

            for (int i = 0; i < samples; i += 2) 
                decoded_data[o++] = (decoded_data[i] + decoded_data[i + 1]) / 2;

            samples = samples / 2;
            channels = 1;
        }
    }

    alGenBuffers(1, sound);

    alBufferData(
        *sound, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, 
        decoded_data, samples * channels * sizeof(short), sample_rate
    );
    
    free(decoded_data);

    return 0;
}

int aud_init_source(Source *source, Sound audio) {
    alGenSources(1, source);
    alSourcei(*source, AL_BUFFER, audio);
    return 0;
}

void aud_play(Source source) {
    alSourcePlay(source);
}

void aud_set_position(Source audio, f32 position[3]) {
    alSource3f(audio, AL_POSITION, position[0], position[1], position[2]);
    alSourcei(audio, AL_DISTANCE_MODEL, AL_INVERSE_DISTANCE);
}

void aud_set_velocity(Source audio, f32 velocity[3]) {
    alSource3f(audio, AL_VELOCITY, velocity[0], velocity[1], velocity[2]);
}

// TODO: CHECK IF EVIL
void aud_set_paused(Source audio, bool paused) {
    alSourcei(audio, AL_PAUSED, paused);
}

void aud_set_looping(Source audio, bool paused) {
    alSourcei(audio, AL_LOOPING, paused);
}

void aud_set_pitch(Source audio, f32 pitch) {
    alSourcef(audio, AL_PITCH, pitch);
}

void aud_set_area(Source audio, f32 distance) {
    alSourcef(audio, AL_MAX_DISTANCE, distance);
}

void aud_set_gain(Source audio, f32 gain) {
    alSourcef(audio, AL_GAIN, gain);
}

void aud_listener(f32 position[3]) {
    alListener3f(AL_POSITION, position[0], position[1], position[2]);
}

void aud_orientation(f32 towards[3], f32 up[3]) {
    f32 orientation[6] = { 
        towards[0], towards[1], towards[2], 
        up[0], up[1], up[2] 
    };
    
    alListenerfv(AL_ORIENTATION, orientation);
}

void aud_listener_gain(f32 volume) {
    alListeneri(AL_GAIN, volume);
}

void aud_global_pause(bool pause) {
    alListeneri(AL_PAUSED, pause);
}

int aud_byebye() {
    ALCcontext *context = alcGetCurrentContext();
    alcDestroyContext(context);

    ALCdevice *device = alcGetContextsDevice(context);
    alcCloseDevice(device);

    return 0;
}
