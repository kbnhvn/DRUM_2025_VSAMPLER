// backend_shims.ino
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>

// ==== settings_load() → wrap vers Settings_load() (qui est statique dans settings.ino) ====
// On déclare ici une forward non-statique que settings.ino exposera via un helper.
extern void Settings__force_load();
extern void settings_load(void) {
  Settings__force_load();
}

// ==== audio_output_init() → wrap vers AudioOut_begin() ====
extern void AudioOut_begin();
extern void audio_output_init(void) {
  AudioOut_begin();
}

// ==== files_init() : initialisation SD + SPIFFS (création dossiers utiles) ====
extern SPIClass sdSPI;     // déjà déclaré dans ton vsampler.ino
#ifndef SD_CS
#define SD_CS 10          // si pas défini, valeur par défaut
#endif

extern void files_init(void) {
  // SPIFFS
  SPIFFS.begin(true);                // true: format si absent

  // SD (via HSPI si tu l’utilises : sdSPI.begin(...) est déjà fait côté pins ?)
  SD.begin(SD_CS, sdSPI);            // si ça échoue, ce n’est pas bloquant pour le link

  // Dossiers attendus par d’autres modules
  if (!SD.exists("/samples")) SD.mkdir("/samples");
  if (!SD.exists("/config"))  SD.mkdir("/config");
}
