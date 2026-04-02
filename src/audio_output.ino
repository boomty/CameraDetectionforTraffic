// AudioPlayer.cpp

#include <Arduino.h>
#include "AudioTools.h"
#include "opening.h"

// ======================================================
// Public function declarations
// ======================================================
void audioBegin();
void audioPlay();
void audioStop();
bool audioIsPlaying();
void audioUpdate();

// ======================================================
// Audio format settings
// ======================================================
static constexpr uint8_t CHANNELS = 2;
static constexpr uint16_t SAMPLE_RATE = 22050;
static constexpr uint8_t BITS_PER_SAMPLE = 16;

// ======================================================
// Internal audio objects
// IMPORTANT:
// Replace StarWars30_raw / StarWars30_raw_len below if
// opening.h uses different variable names.
// Example:
//   opening_raw, opening_raw_len
// ======================================================
static MemoryStream music(StarWars30_raw, StarWars30_raw_len);
static I2SStream i2s;
static StreamCopy copier(i2s, music);

// ======================================================
// Internal state
// ======================================================
static bool g_initialized = false;
static bool g_playing = false;

// ======================================================
// Initialize audio hardware once
// ======================================================
void audioBegin() {
    if (g_initialized) return;

    auto config = i2s.defaultConfig(TX_MODE);
    config.pin_ws = 44;
    config.pin_bck = 7;
    config.pin_data = 43;
    config.sample_rate = SAMPLE_RATE;
    config.channels = CHANNELS;
    config.bits_per_sample = BITS_PER_SAMPLE;

    i2s.begin(config);

    // Prepare the memory stream
    music.begin();

    g_initialized = true;
    g_playing = false;
}

// ======================================================
// Start playback from the beginning
// ======================================================
void audioPlay() {
    if (!g_initialized) {
        audioBegin();
    }

    // Restart the memory stream from the beginning
    music.begin();
    g_playing = true;
}

// ======================================================
// Stop playback
// Note: this stops playback logic.
// Calling audioPlay() again restarts from the beginning.
// ======================================================
void audioStop() {
    g_playing = false;
}

// ======================================================
// Return whether audio is currently playing
// ======================================================
bool audioIsPlaying() {
    return g_playing;
}

// ======================================================
// Call this repeatedly inside loop()
// ======================================================
void audioUpdate() {
    if (!g_initialized) return;
    if (!g_playing) return;

    size_t bytes_copied = copier.copy();

    // If no more bytes were copied, playback has finished
    if (bytes_copied == 0) {
        g_playing = false;
    }
}