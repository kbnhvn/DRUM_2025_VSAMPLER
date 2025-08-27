#include <Arduino.h>
#include <SD.h>

// wraps
extern void Settings__force_load();
extern void settings_load(void) { Settings__force_load(); }

extern void AudioOut_begin();
extern void audio_output_init(void) { AudioOut_begin(); }

// SD init
extern SPIClass sdSPI;           // défini dans ton .ino principal
#ifndef SD_CS
#define SD_CS 10
#endif

extern void files_init(void) {
  // Monte la SD (via ton bus HSPI)
  // Si tu as déjà sdSPI.begin(...) quelque part, ok; sinon ajoute :
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(SD_CS, sdSPI, 10000000);
  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("SD.begin failed (DATAFS)"); // non bloquant
  }
  // Crée les dossiers utilisés ailleurs
  if (!SD.exists("/samples")) SD.mkdir("/samples");
  if (!SD.exists("/config"))  SD.mkdir("/config");
}
