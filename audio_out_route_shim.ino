// Petit shim : certains appels utilisent `AudioOut_setRoute(int)`
// On fait juste la conversion et on délègue à la vraie fonction enum.

#include <Arduino.h>

// AudioOut est défini dans DRUM_2025_VSAMPLER.ino.
// On le forward-déclare ici sans redéfinir l'enum.
enum AudioOut : uint8_t;
void AudioOut_setRoute(AudioOut r);  // impl. réelle dans audio_output.ino

// Surcharge attendue par les menus Settings (mangled `_Z17AudioOut_setRoutei`)
void AudioOut_setRoute(int r) {
  AudioOut_setRoute(static_cast<AudioOut>(r));
}
