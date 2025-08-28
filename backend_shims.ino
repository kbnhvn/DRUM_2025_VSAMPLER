// backend_shims.ino  — SD/SPIFFS init + wrappers propres
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>

// ===== WRAPS depuis ton code existant =====
extern void Settings_load();      // provient de settings.ino
extern void AudioOut_begin();     // provient de audio_output.ino

extern void settings_load(void) { Settings_load(); }
extern void audio_output_init(void) { AudioOut_begin(); }

// ====== Brochage SD (peut être surchargé au compile-time) ======
#ifndef SD_CS
  #define SD_CS   10
#endif
#ifndef SD_MOSI
  #define SD_MOSI 11
#endif
#ifndef SD_MISO
  #define SD_MISO 13
#endif
#ifndef SD_SCLK
  #define SD_SCLK 12
#endif

// Instance SPI pour SD (définie ailleurs ? sinon on la crée ici)
#if !defined(HAVE_SD_SPI_INSTANCE)
  #define HAVE_SD_SPI_INSTANCE 1
  SPIClass sdSPI(HSPI);
#endif

// Petit helper pour lister /samples (debug)
static void list_samples_dir() {
  if (!SD.exists("/samples")) { Serial.println("[files] /samples n'existe pas"); return; }
  File d = SD.open("/samples");
  if (!d || !d.isDirectory()) { Serial.println("[files] /samples: pas un répertoire"); return; }
  Serial.println("[files] /samples:");
  File f;
  while ((f = d.openNextFile())) {
    Serial.printf("  - %s (%u bytes)\n", f.name(), (unsigned)f.size());
    f.close();
  }
  d.close();
}

// Exposée au sketch
extern void files_init(void) {
  Serial.println("[files] init...");

  // --- SPIFFS (patterns/sons sauvegardés)
  if (!SPIFFS.begin(true)) {
    Serial.println("[files] SPIFFS.begin FAILED");
  } else {
    Serial.println("[files] SPIFFS OK");
  }

  // --- SD
  // Sécuriser la ligne CS avant begin()
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Arrêt propre + (re)config SPI pins
  SD.end();
  sdSPI.end();
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  // 25 MHz (si bus long/SD fragile, redescends à 10 MHz)
  const uint32_t kSdHz = 25000000;
  bool ok = SD.begin(SD_CS, sdSPI, kSdHz);
  if (!ok) {
    Serial.println("[files] SD.begin FAILED — vérifie brochage CS/MOSI/MISO/SCLK");
    // Tente plus lent (10 MHz) pour diagnostiquer
    ok = SD.begin(SD_CS, sdSPI, 10000000);
    if (!ok) {
      Serial.println("[files] SD.begin(10MHz) aussi FAILED");
      return; // laisse l’UI indiquer “No SD”
    } else {
      Serial.println("[files] SD OK @10MHz");
    }
  } else {
    Serial.printf("[files] SD OK @%u Hz\n", (unsigned)kSdHz);
  }

  // Dossiers requis
  if (!SD.exists("/samples")) SD.mkdir("/samples");
  if (!SD.exists("/config"))  SD.mkdir("/config");

  // Log contenu /samples pour confirmer que les WAV sont vus
  list_samples_dir();
}
