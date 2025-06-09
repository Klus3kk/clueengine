#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global audio system instance
AudioSystem* audioSystem = NULL;

bool initAudioSystem() {
    if (audioSystem) {
        printf("Audio system already initialized\n");
        return true;
    }

    audioSystem = (AudioSystem*)malloc(sizeof(AudioSystem));
    if (!audioSystem) {
        printf("Failed to allocate audio system\n");
        return false;
    }

    // Initialize OpenAL
    audioSystem->device = alcOpenDevice(NULL);
    if (!audioSystem->device) {
        printf("Failed to open audio device\n");
        free(audioSystem);
        audioSystem = NULL;
        return false;
    }

    audioSystem->context = alcCreateContext(audioSystem->device, NULL);
    if (!audioSystem->context) {
        printf("Failed to create audio context\n");
        alcCloseDevice(audioSystem->device);
        free(audioSystem);
        audioSystem = NULL;
        return false;
    }

    alcMakeContextCurrent(audioSystem->context);

    // Initialize arrays
    for (int i = 0; i < MAX_AUDIO_BUFFERS; i++) {
        audioSystem->buffers[i] = NULL;
    }
    for (int i = 0; i < MAX_AUDIO_SOURCES; i++) {
        audioSystem->sources[i] = NULL;
    }

    audioSystem->bufferCount = 0;
    audioSystem->sourceCount = 0;
    audioSystem->masterVolume = 1.0f;
    audioSystem->isInitialized = true;

    // Set default listener properties
    audioSystem->listenerPosition = (Vector3){0.0f, 0.0f, 0.0f};
    audioSystem->listenerVelocity = (Vector3){0.0f, 0.0f, 0.0f};
    audioSystem->listenerOrientation[0] = (Vector3){0.0f, 0.0f, -1.0f}; // At
    audioSystem->listenerOrientation[1] = (Vector3){0.0f, 1.0f, 0.0f};  // Up

    setListenerPosition(&audioSystem->listenerPosition);
    setListenerVelocity(&audioSystem->listenerVelocity);
    setListenerOrientation(&audioSystem->listenerOrientation[0], &audioSystem->listenerOrientation[1]);

    printf("Audio system initialized successfully\n");
    return true;
}

void shutdownAudioSystem() {
    if (!audioSystem) return;

    // Stop all sounds
    stopAllSounds();

    // Clean up sources
    for (int i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (audioSystem->sources[i]) {
            destroyAudioSource(i);
        }
    }

    // Clean up buffers
    for (int i = 0; i < MAX_AUDIO_BUFFERS; i++) {
        if (audioSystem->buffers[i]) {
            unloadAudioBuffer(i);
        }
    }

    // Clean up OpenAL
    alcMakeContextCurrent(NULL);
    if (audioSystem->context) {
        alcDestroyContext(audioSystem->context);
    }
    if (audioSystem->device) {
        alcCloseDevice(audioSystem->device);
    }

    free(audioSystem);
    audioSystem = NULL;
    printf("Audio system shutdown\n");
}

void updateAudioSystem() {
    if (!audioSystem || !audioSystem->isInitialized) return;

    // Update any streaming audio or other per-frame audio tasks
    // This can be expanded based on needs
}

int createAudioSource() {
    if (!audioSystem || !audioSystem->isInitialized) return -1;

    for (int i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (!audioSystem->sources[i]) {
            audioSystem->sources[i] = (AudioSource*)malloc(sizeof(AudioSource));
            if (!audioSystem->sources[i]) return -1;

            AudioSource* source = audioSystem->sources[i];
            alGenSources(1, &source->source);
            
            if (alGetError() != AL_NO_ERROR) {
                free(audioSystem->sources[i]);
                audioSystem->sources[i] = NULL;
                return -1;
            }

            source->buffer = 0;
            source->position = (Vector3){0.0f, 0.0f, 0.0f};
            source->velocity = (Vector3){0.0f, 0.0f, 0.0f};
            source->volume = 1.0f;
            source->pitch = 1.0f;
            source->isLooping = false;
            source->isPlaying = false;
            source->is3D = false;
            source->isActive = true;

            audioSystem->sourceCount++;
            return i;
        }
    }
    return -1;
}

void destroyAudioSource(int sourceIndex) {
    if (!audioSystem || sourceIndex < 0 || sourceIndex >= MAX_AUDIO_SOURCES) return;
    if (!audioSystem->sources[sourceIndex]) return;

    AudioSource* source = audioSystem->sources[sourceIndex];
    alDeleteSources(1, &source->source);
    free(source);
    audioSystem->sources[sourceIndex] = NULL;
    audioSystem->sourceCount--;
}

void playSound(const char* filepath) {
    if (!audioSystem || !audioSystem->isInitialized) {
        printf("Audio system not initialized\n");
        return;
    }

    int bufferIndex = loadAudioBuffer(filepath);
    if (bufferIndex < 0) {
        printf("Failed to load audio buffer for: %s\n", filepath);
        return;
    }

    int sourceIndex = createAudioSource();
    if (sourceIndex < 0) {
        printf("Failed to create audio source\n");
        return;
    }

    playSoundSource(sourceIndex, bufferIndex);
}

void playSound3D(const char* filepath, Vector3* position) {
    if (!audioSystem || !audioSystem->isInitialized) return;

    int bufferIndex = loadAudioBuffer(filepath);
    if (bufferIndex < 0) return;

    int sourceIndex = createAudioSource();
    if (sourceIndex < 0) return;

    setSourcePosition(sourceIndex, position);
    setSource3D(sourceIndex, true);
    playSoundSource(sourceIndex, bufferIndex);
}

void playMusic(const char* filepath, bool loop) {
    if (!audioSystem || !audioSystem->isInitialized) return;

    int bufferIndex = loadAudioBuffer(filepath);
    if (bufferIndex < 0) return;

    int sourceIndex = createAudioSource();
    if (sourceIndex < 0) return;

    setSourceLooping(sourceIndex, loop);
    setSource3D(sourceIndex, false);
    playSoundSource(sourceIndex, bufferIndex);
}

int playSoundSource(int sourceIndex, int bufferIndex) {
    if (!isSourceValid(sourceIndex) || bufferIndex < 0 || bufferIndex >= MAX_AUDIO_BUFFERS) return -1;
    if (!audioSystem->buffers[bufferIndex]) return -1;

    AudioSource* source = audioSystem->sources[sourceIndex];
    source->buffer = audioSystem->buffers[bufferIndex]->buffer;
    
    alSourcei(source->source, AL_BUFFER, source->buffer);
    alSourcePlay(source->source);
    
    if (alGetError() != AL_NO_ERROR) return -1;
    
    source->isPlaying = true;
    return 0;
}

void stopSound(int sourceIndex) {
    if (!isSourceValid(sourceIndex)) return;
    
    AudioSource* source = audioSystem->sources[sourceIndex];
    alSourceStop(source->source);
    source->isPlaying = false;
}

void pauseSound(int sourceIndex) {
    if (!isSourceValid(sourceIndex)) return;
    
    alSourcePause(audioSystem->sources[sourceIndex]->source);
}

void resumeSound(int sourceIndex) {
    if (!isSourceValid(sourceIndex)) return;
    
    alSourcePlay(audioSystem->sources[sourceIndex]->source);
}

void setSourcePosition(int sourceIndex, Vector3* position) {
    if (!isSourceValid(sourceIndex) || !position) return;
    
    AudioSource* source = audioSystem->sources[sourceIndex];
    source->position = *position;
    alSource3f(source->source, AL_POSITION, position->x, position->y, position->z);
}

void setSourceVelocity(int sourceIndex, Vector3* velocity) {
    if (!isSourceValid(sourceIndex) || !velocity) return;
    
    AudioSource* source = audioSystem->sources[sourceIndex];
    source->velocity = *velocity;
    alSource3f(source->source, AL_VELOCITY, velocity->x, velocity->y, velocity->z);
}

void setSourceVolume(int sourceIndex, float volume) {
    if (!isSourceValid(sourceIndex)) return;
    
    AudioSource* source = audioSystem->sources[sourceIndex];
    source->volume = volume;
    alSourcef(source->source, AL_GAIN, volume * audioSystem->masterVolume);
}

void setSourcePitch(int sourceIndex, float pitch) {
    if (!isSourceValid(sourceIndex)) return;
    
    AudioSource* source = audioSystem->sources[sourceIndex];
    source->pitch = pitch;
    alSourcef(source->source, AL_PITCH, pitch);
}

void setSourceLooping(int sourceIndex, bool looping) {
    if (!isSourceValid(sourceIndex)) return;
    
    AudioSource* source = audioSystem->sources[sourceIndex];
    source->isLooping = looping;
    alSourcei(source->source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
}

void setSource3D(int sourceIndex, bool is3D) {
    if (!isSourceValid(sourceIndex)) return;
    
    AudioSource* source = audioSystem->sources[sourceIndex];
    source->is3D = is3D;
    
    if (is3D) {
        alSourcei(source->source, AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcef(source->source, AL_ROLLOFF_FACTOR, 1.0f);
        alSourcef(source->source, AL_REFERENCE_DISTANCE, 1.0f);
        alSourcef(source->source, AL_MAX_DISTANCE, 100.0f);
    } else {
        alSourcei(source->source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(source->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    }
}

void setListenerPosition(Vector3* position) {
    if (!audioSystem || !position) return;
    
    audioSystem->listenerPosition = *position;
    alListener3f(AL_POSITION, position->x, position->y, position->z);
}

void setListenerVelocity(Vector3* velocity) {
    if (!audioSystem || !velocity) return;
    
    audioSystem->listenerVelocity = *velocity;
    alListener3f(AL_VELOCITY, velocity->x, velocity->y, velocity->z);
}

void setListenerOrientation(Vector3* at, Vector3* up) {
    if (!audioSystem || !at || !up) return;
    
    audioSystem->listenerOrientation[0] = *at;
    audioSystem->listenerOrientation[1] = *up;
    
    ALfloat orientation[6] = {at->x, at->y, at->z, up->x, up->y, up->z};
    alListenerfv(AL_ORIENTATION, orientation);
}

void setMasterVolume(float volume) {
    if (!audioSystem) return;
    
    audioSystem->masterVolume = volume;
    alListenerf(AL_GAIN, volume);
    
    // Update all source volumes
    for (int i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (audioSystem->sources[i]) {
            setSourceVolume(i, audioSystem->sources[i]->volume);
        }
    }
}

float getMasterVolume() {
    if (!audioSystem) return 0.0f;
    return audioSystem->masterVolume;
}

bool isSourcePlaying(int sourceIndex) {
    if (!isSourceValid(sourceIndex)) return false;
    
    ALint state;
    alGetSourcei(audioSystem->sources[sourceIndex]->source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

bool isSourceValid(int sourceIndex) {
    if (!audioSystem || sourceIndex < 0 || sourceIndex >= MAX_AUDIO_SOURCES) return false;
    return audioSystem->sources[sourceIndex] != NULL && audioSystem->sources[sourceIndex]->isActive;
}

void stopAllSounds() {
    if (!audioSystem) return;
    
    for (int i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (audioSystem->sources[i]) {
            stopSound(i);
        }
    }
}

void pauseAllSounds() {
    if (!audioSystem) return;
    
    for (int i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (audioSystem->sources[i] && isSourcePlaying(i)) {
            pauseSound(i);
        }
    }
}

void resumeAllSounds() {
    if (!audioSystem) return;
    
    for (int i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (audioSystem->sources[i]) {
            resumeSound(i);
        }
    }
}

// Stub implementations for audio buffer management
// You'll need to implement these based on your audio file format needs
int loadAudioBuffer(const char* filepath) {
    // This is a stub - implement based on your audio file format (WAV, OGG, etc.)
    printf("Loading audio buffer: %s (stub implementation)\n", filepath);
    return -1; // Return valid buffer index when implemented
}

void unloadAudioBuffer(int bufferIndex) {
    if (!audioSystem || bufferIndex < 0 || bufferIndex >= MAX_AUDIO_BUFFERS) return;
    if (!audioSystem->buffers[bufferIndex]) return;
    
    alDeleteBuffers(1, &audioSystem->buffers[bufferIndex]->buffer);
    free(audioSystem->buffers[bufferIndex]);
    audioSystem->buffers[bufferIndex] = NULL;
    audioSystem->bufferCount--;
}

AudioBuffer* getAudioBuffer(int bufferIndex) {
    if (!audioSystem || bufferIndex < 0 || bufferIndex >= MAX_AUDIO_BUFFERS) return NULL;
    return audioSystem->buffers[bufferIndex];
}

AudioSource* getAudioSource(int sourceIndex) {
    if (!audioSystem || sourceIndex < 0 || sourceIndex >= MAX_AUDIO_SOURCES) return NULL;
    return audioSystem->sources[sourceIndex];
}

AudioFormat getAudioFormat(int channels, int bitsPerSample) {
    if (channels == 1) {
        return (bitsPerSample == 8) ? AUDIO_FORMAT_MONO8 : AUDIO_FORMAT_MONO16;
    } else {
        return (bitsPerSample == 8) ? AUDIO_FORMAT_STEREO8 : AUDIO_FORMAT_STEREO16;
    }
}

ALenum getOpenALFormat(AudioFormat format) {
    switch (format) {
        case AUDIO_FORMAT_MONO8: return AL_FORMAT_MONO8;
        case AUDIO_FORMAT_MONO16: return AL_FORMAT_MONO16;
        case AUDIO_FORMAT_STEREO8: return AL_FORMAT_STEREO8;
        case AUDIO_FORMAT_STEREO16: return AL_FORMAT_STEREO16;
        default: return AL_FORMAT_MONO16;
    }
}

void checkALError(const char* operation) {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        printf("OpenAL Error during %s: %s\n", operation, getALErrorString(error));
    }
}

const char* getALErrorString(ALenum error) {
    switch (error) {
        case AL_NO_ERROR: return "No error";
        case AL_INVALID_NAME: return "Invalid name";
        case AL_INVALID_ENUM: return "Invalid enum";
        case AL_INVALID_VALUE: return "Invalid value";
        case AL_INVALID_OPERATION: return "Invalid operation";
        case AL_OUT_OF_MEMORY: return "Out of memory";
        default: return "Unknown error";
    }
}