#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <WiFi.h>

// --- State flags
bool g_spiffs_ok = false;
bool g_sd_ok     = false;

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
#ifndef SD_SCLK
#define SD_SCLK 12
#endif
#ifndef SD_MISO
#define SD_MISO 13
#endif
#ifndef SD_MOSI
#define SD_MOSI 11
#endif

extern void files_init(void) {
  // Serial très tôt (si pas déjà fait)
  if (!Serial) { Serial.begin(115200); delay(250); }

  Serial.println("[files] init...");

  // ---- SPIFFS
  g_spiffs_ok = SPIFFS.begin(true /* formatOnFail */);
  Serial.printf("[files] SPIFFS %s\n", g_spiffs_ok ? "OK" : "FAIL");

  // ---- SD (une seule tentative, vitesse raisonnable)
  SD.end();
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  g_sd_ok = SD.begin(SD_CS, sdSPI, 20000000 /*20 MHz*/);
  Serial.printf("[files] SD %s\n", g_sd_ok ? "OK" : "FAIL");

  if (g_sd_ok) {
    if (!SD.exists("/samples")) SD.mkdir("/samples");
    if (!SD.exists("/config"))  SD.mkdir("/config");
  }

  // ---- Wi-Fi par défaut OFF (évite les bootloops)
  WiFi.mode(WIFI_OFF);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
}
