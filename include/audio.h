#ifndef AUDIO_H
#define AUDIO_H

#include <AL/al.h>
#include <AL/alc.h>
#include <stdbool.h>
#include <stdint.h>
#include "Vectors.h"

#define MAX_AUDIO_SOURCES 32
#define MAX_AUDIO_BUFFERS 64

typedef enum {
    AUDIO_FORMAT_MONO8,
    AUDIO_FORMAT_MONO16,
    AUDIO_FORMAT_STEREO8,
    AUDIO_FORMAT_STEREO16
} AudioFormat;

typedef struct {
    ALuint buffer;
    char filepath[256];
    AudioFormat format;
    ALsizei frequency;
    bool isLoaded;
} AudioBuffer;

typedef struct {
    ALuint source;
    ALuint buffer;
    Vector3 position;
    Vector3 velocity;
    float volume;
    float pitch;
    bool isLooping;
    bool isPlaying;
    bool is3D;
    bool isActive;
} AudioSource;

typedef struct {
    ALCdevice* device;
    ALCcontext* context;
    AudioBuffer* buffers[MAX_AUDIO_BUFFERS];
    AudioSource* sources[MAX_AUDIO_SOURCES];
    int bufferCount;
    int sourceCount;
    Vector3 listenerPosition;
    Vector3 listenerVelocity;
    Vector3 listenerOrientation[2]; // At and Up vectors
    float masterVolume;
    bool isInitialized;
} AudioSystem;

extern AudioSystem* audioSystem;

// Core audio system functions
bool initAudioSystem();
void shutdownAudioSystem();
void updateAudioSystem();

// Buffer management
int loadAudioBuffer(const char* filepath);
void unloadAudioBuffer(int bufferIndex);
AudioBuffer* getAudioBuffer(int bufferIndex);

// Source management
int createAudioSource();
void destroyAudioSource(int sourceIndex);
AudioSource* getAudioSource(int sourceIndex);

// Playback functions
void playSound(const char* filepath);
void playSound3D(const char* filepath, Vector3* position);
void playMusic(const char* filepath, bool loop);
int playSoundSource(int sourceIndex, int bufferIndex);
void stopSound(int sourceIndex);
void pauseSound(int sourceIndex);
void resumeSound(int sourceIndex);

// Source property setters
void setSourcePosition(int sourceIndex, Vector3* position);
void setSourceVelocity(int sourceIndex, Vector3* velocity);
void setSourceVolume(int sourceIndex, float volume);
void setSourcePitch(int sourceIndex, float pitch);
void setSourceLooping(int sourceIndex, bool looping);
void setSource3D(int sourceIndex, bool is3D);

// Listener functions
void setListenerPosition(Vector3* position);
void setListenerVelocity(Vector3* velocity);
void setListenerOrientation(Vector3* at, Vector3* up);

// Global audio settings
void setMasterVolume(float volume);
float getMasterVolume();

// Utility functions
bool isSourcePlaying(int sourceIndex);
bool isSourceValid(int sourceIndex);
void stopAllSounds();
void pauseAllSounds();
void resumeAllSounds();

// Audio format detection
AudioFormat getAudioFormat(int channels, int bitsPerSample);
ALenum getOpenALFormat(AudioFormat format);

// Error handling
void checkALError(const char* operation);
const char* getALErrorString(ALenum error);

// Helper functions
int strcasecmp(const char *s1, const char *s2);

#endif