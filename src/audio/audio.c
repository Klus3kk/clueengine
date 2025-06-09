#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include stb_vorbis for OGG support
#define STB_VORBIS_IMPLEMENTATION
#include "stb_vorbis.c"

// Global audio system instance
AudioSystem* audioSystem = NULL;

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Format size
    uint16_t audioFormat;   // Audio format (1 = PCM)
    uint16_t numChannels;   // Number of channels
    uint32_t sampleRate;    // Sample rate
    uint32_t byteRate;      // Byte rate
    uint16_t blockAlign;    // Block align
    uint16_t bitsPerSample; // Bits per sample
    char data[4];           // "data"
    uint32_t dataSize;      // Data size
} WAVHeader;

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

// Helper function for case-insensitive string comparison
int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

// Load WAV file
int loadWAVFile(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Failed to open WAV file: %s\n", filepath);
        return -1;
    }

    WAVHeader header;
    if (fread(&header, sizeof(WAVHeader), 1, file) != 1) {
        printf("Failed to read WAV header: %s\n", filepath);
        fclose(file);
        return -1;
    }

    // Verify WAV format
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        printf("Invalid WAV file format: %s\n", filepath);
        fclose(file);
        return -1;
    }

    // Find available buffer slot
    int bufferIndex = -1;
    for (int i = 0; i < MAX_AUDIO_BUFFERS; i++) {
        if (!audioSystem->buffers[i]) {
            bufferIndex = i;
            break;
        }
    }

    if (bufferIndex == -1) {
        printf("No available audio buffer slots\n");
        fclose(file);
        return -1;
    }

    // Allocate buffer structure
    audioSystem->buffers[bufferIndex] = (AudioBuffer*)malloc(sizeof(AudioBuffer));
    if (!audioSystem->buffers[bufferIndex]) {
        printf("Failed to allocate audio buffer\n");
        fclose(file);
        return -1;
    }

    AudioBuffer* buffer = audioSystem->buffers[bufferIndex];

    // Read audio data
    unsigned char* data = (unsigned char*)malloc(header.dataSize);
    if (!data) {
        printf("Failed to allocate audio data\n");
        free(audioSystem->buffers[bufferIndex]);
        audioSystem->buffers[bufferIndex] = NULL;
        fclose(file);
        return -1;
    }

    if (fread(data, header.dataSize, 1, file) != 1) {
        printf("Failed to read audio data: %s\n", filepath);
        free(data);
        free(audioSystem->buffers[bufferIndex]);
        audioSystem->buffers[bufferIndex] = NULL;
        fclose(file);
        return -1;
    }

    fclose(file);

    // Determine OpenAL format
    ALenum format;
    if (header.numChannels == 1) {
        format = (header.bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        buffer->format = (header.bitsPerSample == 8) ? AUDIO_FORMAT_MONO8 : AUDIO_FORMAT_MONO16;
    } else {
        format = (header.bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        buffer->format = (header.bitsPerSample == 8) ? AUDIO_FORMAT_STEREO8 : AUDIO_FORMAT_STEREO16;
    }

    // Create OpenAL buffer
    alGenBuffers(1, &buffer->buffer);
    if (alGetError() != AL_NO_ERROR) {
        printf("Failed to generate OpenAL buffer\n");
        free(data);
        free(audioSystem->buffers[bufferIndex]);
        audioSystem->buffers[bufferIndex] = NULL;
        return -1;
    }

    // Upload data to OpenAL
    alBufferData(buffer->buffer, format, data, header.dataSize, header.sampleRate);
    if (alGetError() != AL_NO_ERROR) {
        printf("Failed to upload audio data to OpenAL\n");
        alDeleteBuffers(1, &buffer->buffer);
        free(data);
        free(audioSystem->buffers[bufferIndex]);
        audioSystem->buffers[bufferIndex] = NULL;
        return -1;
    }

    // Store buffer info
    strncpy(buffer->filepath, filepath, sizeof(buffer->filepath) - 1);
    buffer->filepath[sizeof(buffer->filepath) - 1] = '\0';
    buffer->frequency = header.sampleRate;
    buffer->isLoaded = true;

    free(data);
    audioSystem->bufferCount++;

    printf("Successfully loaded WAV file: %s (Buffer %d)\n", filepath, bufferIndex);
    return bufferIndex;
}

// Load OGG file using stb_vorbis
int loadOGGFile(const char* filepath) {
    // Open the OGG file
    int error = 0;
    stb_vorbis* vorbis = stb_vorbis_open_filename(filepath, &error, NULL);
    if (!vorbis) {
        printf("Failed to open OGG file: %s (Error: %d)\n", filepath, error);
        return -1;
    }

    // Get file info
    stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    printf("OGG Info - Channels: %d, Sample Rate: %d\n", info.channels, info.sample_rate);

    // Get total samples
    int totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
    if (totalSamples <= 0) {
        printf("Invalid OGG file length: %s\n", filepath);
        stb_vorbis_close(vorbis);
        return -1;
    }

    // Calculate buffer size (16-bit samples)
    int bufferSize = totalSamples * info.channels * sizeof(short);
    short* audioData = (short*)malloc(bufferSize);
    if (!audioData) {
        printf("Failed to allocate memory for OGG data\n");
        stb_vorbis_close(vorbis);
        return -1;
    }

    // Read the entire file
    int samplesRead = stb_vorbis_get_samples_short_interleaved(vorbis, info.channels, audioData, totalSamples * info.channels);
    if (samplesRead <= 0) {
        printf("Failed to read OGG samples: %s\n", filepath);
        free(audioData);
        stb_vorbis_close(vorbis);
        return -1;
    }

    // Close the vorbis file
    stb_vorbis_close(vorbis);

    // Find available buffer slot
    int bufferIndex = -1;
    for (int i = 0; i < MAX_AUDIO_BUFFERS; i++) {
        if (!audioSystem->buffers[i]) {
            bufferIndex = i;
            break;
        }
    }

    if (bufferIndex == -1) {
        printf("No available audio buffer slots\n");
        free(audioData);
        return -1;
    }

    // Allocate buffer structure
    audioSystem->buffers[bufferIndex] = (AudioBuffer*)malloc(sizeof(AudioBuffer));
    if (!audioSystem->buffers[bufferIndex]) {
        printf("Failed to allocate audio buffer\n");
        free(audioData);
        return -1;
    }

    AudioBuffer* buffer = audioSystem->buffers[bufferIndex];

    // Determine OpenAL format (OGG is always 16-bit)
    ALenum format;
    if (info.channels == 1) {
        format = AL_FORMAT_MONO16;
        buffer->format = AUDIO_FORMAT_MONO16;
    } else {
        format = AL_FORMAT_STEREO16;
        buffer->format = AUDIO_FORMAT_STEREO16;
    }

    // Create OpenAL buffer
    alGenBuffers(1, &buffer->buffer);
    if (alGetError() != AL_NO_ERROR) {
        printf("Failed to generate OpenAL buffer for OGG\n");
        free(audioData);
        free(audioSystem->buffers[bufferIndex]);
        audioSystem->buffers[bufferIndex] = NULL;
        return -1;
    }

    // Upload data to OpenAL (actual samples read * channels * sizeof(short))
    int actualBufferSize = samplesRead * info.channels * sizeof(short);
    alBufferData(buffer->buffer, format, audioData, actualBufferSize, info.sample_rate);
    if (alGetError() != AL_NO_ERROR) {
        printf("Failed to upload OGG data to OpenAL\n");
        alDeleteBuffers(1, &buffer->buffer);
        free(audioData);
        free(audioSystem->buffers[bufferIndex]);
        audioSystem->buffers[bufferIndex] = NULL;
        return -1;
    }

    // Store buffer info
    strncpy(buffer->filepath, filepath, sizeof(buffer->filepath) - 1);
    buffer->filepath[sizeof(buffer->filepath) - 1] = '\0';
    buffer->frequency = info.sample_rate;
    buffer->isLoaded = true;

    free(audioData);
    audioSystem->bufferCount++;

    printf("Successfully loaded OGG file: %s (Buffer %d, %d samples, %d channels)\n", 
           filepath, bufferIndex, samplesRead, info.channels);
    return bufferIndex;
}

// Updated loadAudioBuffer function
int loadAudioBuffer(const char* filepath) {
    if (!audioSystem || !audioSystem->isInitialized) {
        printf("Audio system not initialized\n");
        return -1;
    }

    // Check if already loaded
    for (int i = 0; i < MAX_AUDIO_BUFFERS; i++) {
        if (audioSystem->buffers[i] && 
            strcmp(audioSystem->buffers[i]->filepath, filepath) == 0) {
            printf("Audio file already loaded: %s (Buffer %d)\n", filepath, i);
            return i;
        }
    }

    // Determine file type and load accordingly
    const char* ext = strrchr(filepath, '.');
    if (ext) {
        if (strcasecmp(ext, ".wav") == 0) {
            return loadWAVFile(filepath);
        } else if (strcasecmp(ext, ".ogg") == 0) {
            return loadOGGFile(filepath);
        }
    }

    printf("Unsupported audio format: %s\n", filepath);
    return -1;
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