// Backlight control (PWM)
#include <Arduino.h>
extern Arduino_GFX *gfx;   // défini dans le .ino principal
#include "esp_task_wdt.h"

#ifndef GFX_BL
#define GFX_BL 1
#endif

static const uint8_t BL_RES = 10;
static const uint32_t BL_HZ = 5000;
static int currentBrightness = 100;  // NOUVEAU: État actuel
static bool backlightEnabled = true; // NOUVEAU: État on/off
static unsigned long lastActivity = 0; // NOUVEAU: Auto-dimming

static inline uint16_t bl_maxduty() { return (1u << BL_RES) - 1u; }

// CORRECTION: Table de correction gamma taille correcte (256 valeurs pour index 0-100)
static const uint16_t gammaTable[101] = {
    0,   16,   32,   48,   64,   80,   96,   112,   128,   144,   160,   176,   192,   208,   224,   240,
    256,   272,   288,   304,   320,   336,   352,   368,   384,   400,   416,   432,   448,   464,   480,   496,
    512,   528,   544,   560,   576,   592,   608,   624,   640,   656,   672,   688,   704,   720,   736,   752,
    768,   784,   800,   816,   832,   848,   864,   880,   896,   912,   928,   944,   960,   976,   992,   1008,
    1024,   1040,   1056,   1072,   1088,   1104,   1120,   1136,   1152,   1168,   1184,   1200,   1216,   1232,   1248,   1264,
    1280,   1296,   1312,   1328,   1344,   1360,   1376,   1392,   1408,   1424,   1440,   1456,   1472,   1488,   1504,   1520,
    1536,   1552,   1568,   1584,   1600
};

// CORRECTION: Déclaration de la fonction avant utilisation
void setBacklightPercent(int p);

void backlight_init() {
  pinMode(GFX_BL, OUTPUT);
  analogWriteFrequency(GFX_BL, BL_HZ);
  analogWriteResolution(GFX_BL, BL_RES);
  
  // CORRECTION: Démarrage progressif pour éviter flash
  for (int i = 0; i <= 100; i += 5) {
    setBacklightPercent(i);
    delay(20);
  }
  
  currentBrightness = 100;
  lastActivity = millis();
  
  Serial.println("[BACKLIGHT] Initialized with gamma correction");
}

void setBacklightPercent(int p){
  if (p < 0)   p = 0;
  if (p > 100) p = 100;
  
  currentBrightness = p;
  lastActivity = millis();
  
  if (!backlightEnabled && p > 0) {
    backlightEnabled = true;
  } else if (p == 0) {
    backlightEnabled = false;
  }
  
  // CORRECTION: Application courbe gamma pour perception linéaire
  const uint16_t gammaCorrected = gammaTable[p];
  const uint16_t duty = (uint16_t)((bl_maxduty() * (uint32_t)gammaCorrected) / 1600u);
  
  analogWrite(GFX_BL, duty);

  Serial.printf("[BACKLIGHT] Set to %d%% (gamma: %d, duty: %d)\n", p, gammaCorrected, duty);
}

int getBacklightPercent() {
  return currentBrightness;
}

// NOUVEAU: Auto-dimming pour économie d'énergie
void updateBacklightActivity() {
  lastActivity = millis();
}

bool isBacklightEnabled() {
  return backlightEnabled;
}

// NOUVEAU: Dimming automatique après inactivité
void checkAutoDimming() {
  const unsigned long AUTO_DIM_DELAY = 300000; // 5 minutes
  const int DIM_LEVEL = 20; // 20% en mode veille
  
  if (!backlightEnabled) return;
  
  if (millis() - lastActivity > AUTO_DIM_DELAY) {
    if (currentBrightness > DIM_LEVEL) {
      Serial.println("[BACKLIGHT] Auto-dimming activated");
      
      // Transition graduelle vers dimming
      int startBright = currentBrightness;
      for (int i = startBright; i >= DIM_LEVEL; i -= 5) {
        setBacklightPercent(i);
        delay(50);
      }
      
      Serial.printf("[BACKLIGHT] Dimmed to %d%% after inactivity\n", DIM_LEVEL);
    }
  }
}

// NOUVEAU: Fonction de réveil du backlight
void wakeBacklight() {
  if (currentBrightness < 80) {
    // Réveil progressif
    for (int i = currentBrightness; i <= 80; i += 10) {
      setBacklightPercent(i);
      delay(30);
    }
    Serial.println("[BACKLIGHT] Wake up from auto-dim");
  }
  updateBacklightActivity();
}

void performWakeAnimation() {
  if (!gfx) return;
  
  gfx->fillScreen(BLACK);
  gfx->setTextColor(RGB565(100, 255, 100), BLACK);
  gfx->setCursor(190, 130);
  gfx->print("WAKE UP");
  
  // Réveil progressif du backlight
  for (int brightness = 0; brightness <= 80; brightness += 10) {
    setBacklightPercent(brightness);
    delay(80);
  }
  
  delay(1000);
}