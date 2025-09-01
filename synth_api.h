#pragma once
#include <Arduino.h>

// Toutes les fonctions exposées par synthESP32.ino
// Types choisis pour correspondre aux usages actuels ET aux manglings observés.
// - voice : uint8_t
// - pitch : int
// - volumes/cutoff : uint8_t
// - pan : int8_t

void synthESP32_begin(void);

void synthESP32_TRIGGER(uint8_t voice);
void synthESP32_TRIGGER_P(uint8_t voice, int pitch);

void synthESP32_setMVol(uint8_t vol);      // 0..127 attendu dans le code
void synthESP32_setMFilter(uint8_t cutoff);// idem (barre MFI)

void synthESP32_setVol(uint8_t voice, uint8_t vol);
void synthESP32_setPan(uint8_t voice, int8_t pan);
void synthESP32_setPitch(uint8_t voice, int pitch);
void synthESP32_setReverse(uint8_t voice, uint8_t enable);
void synthESP32_setFilter(uint8_t voice, uint8_t cutoff);
void synthESP32_updateVolPan(uint8_t voice);
void synthESP32_setIni(uint8_t voice, int ini);    // NOUVEAU: Bornes sample
void synthESP32_setEnd(uint8_t voice, int end);    // NOUVEAU: Bornes sample

// Outils de sélection / aléa
void setSound(uint8_t voice);
void setRandomVoice(uint8_t voice);
void setRandomVoice2(uint8_t voice);
void setRandomPattern(uint8_t voice);
void setRandomNotes(uint8_t voice);
void setRandomPitch(uint8_t voice);


// NOUVEAU: Audio output switching
void switchAudioOutput(bool useDAC);

// NOUVEAU: System functions
void audio_task(void *userData);            // Task audio principal

// NOUVEAU: Constants pour validation
#define BANK_SIZE 256
#define MAX_VOICES 16
#define PREVIEW_SLOT 255

// NOUVEAU: Error codes
typedef enum {
  SYNTH_OK = 0,
  SYNTH_ERROR_INVALID_VOICE,
  SYNTH_ERROR_INVALID_SLOT,
  SYNTH_ERROR_NO_SAMPLE,
  SYNTH_ERROR_MEMORY
} synth_error_t;