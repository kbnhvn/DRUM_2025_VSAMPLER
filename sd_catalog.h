#pragma once
#include <Arduino.h>
#include <vector>

// Métadonnées d’un sample sur SD
struct SampleMeta {
  String   path;          // chemin complet /samples/.../foo.wav
  String   name;          // nom affiché (ex: "kicks/808/foo.wav")
  uint32_t len   = 0;     // nb d’échantillons 16-bit
  int16_t* buf   = nullptr; // pointeur buffer une fois chargé (ou null si déchargé)
  uint32_t rate  = 44100; // Hz, si déterminable (sinon 44100 par défaut)
};

// Catalogue global
extern std::vector<SampleMeta> CATALOG;

// API exposée par sd_catalog.ino
void buildCatalog();                               // (ré)scan non-récursif ou récursif selon ton implémentation actuelle
bool assignSampleToSlot(int catIndex, int slot);   // charge/carte un sample SD vers un "slot" (SAM) utilisable
bool loadSampleBuffer(int catIndex, int slot);     // si tu as une API explicite de chargement

static inline float sampleSeconds(const SampleMeta& m) {
  const float rate = (m.rate > 0) ? (float)m.rate : 44100.0f;
  return (rate > 0.f) ? (float)m.len / rate : 0.f;
}

// CORRECTION: Structure pour header WAV avec validation
struct WavHeader {
  uint16_t format;
  uint16_t channels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  uint32_t dataSize;
  uint32_t dataOffset;
  bool valid;
};
