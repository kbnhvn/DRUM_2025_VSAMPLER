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

// NOUVEAU: Table de correction gamma pour perception linéaire
static const uint8_t gammaTable[101] = {
    0,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3,   3,   3,
    4,   4,   4,   5,   5,   6,   6,   7,   8,   8,   9,  10,  11,  12,  13,  14,
   15,  16,  18,  19,  20,  22,  23,  25,  27,  28,  30,  32,  34,  36,  38,  40,
   43,  45,  48,  50,  53,  56,  59,  62,  65,  68,  71,  75,  78,  82,  85,  89,
   93,  97, 101, 105, 110, 114, 119, 123, 128, 133, 138, 143, 149, 154, 159, 165,
  171, 177, 183, 189, 195, 201, 208, 214, 221, 228, 235, 242, 249, 256, 264, 271,
  279, 287, 295, 303, 311, 320, 329, 337, 346, 356, 365, 375, 385, 395, 406, 416,
  427, 438, 449, 461, 472, 484, 496, 509, 521, 534, 547, 560, 574, 587, 601, 615,
  630, 644, 659, 675, 690, 706, 722, 738, 755, 772, 789, 806, 824, 842, 860, 879,
  898, 917, 937, 957, 977, 997,1018,1039,1061,1082,1105,1127,1150,1173,1197,1221,
  1245,1270,1295,1320,1346,1372,1398,1425,1452,1479,1507,1535,1564,1593,1622,1652,
  1682,1713,1744,1775,1807,1839,1872,1905,1938,1972,2006,2041,2076,2112,2148,2185,
  2222,2260,2298,2336,2375,2415,2455,2495,2536,2578,2620,2662,2705,2749,2793,2837,
  2882,2928,2974,3021,3068,3116,3165,3214,3264,3314,3365,3417,3469,3522,3575,3629,
  3684,3739,3795,3852,3909,3967,4026,4085,4145,4206,4267,4329,4392,4455,4519,4584,
  4649,4715,4782,4850,4918,4987,5057,5128,5199,5271,5344,5418,5493,5568,5645,5722,
  5800,5879,5959,6040,6122,6204,6288,6372,6458,6544,6631,6719,6808,6898,6989,7081,
  7174,7268,7363,7459,7556,7654,7753,7853,7954,8056,8159,8263,8368,8474,8581,8690,
  8799,8910,9022,9135,9249,9364,9480,9598,9716,9836,9957,10079,10202,10327,10452,10579,
  10707,10836,10966,11098,11231,11365,11500,11637,11775,11914,12055,12197,12340,12485,12631,12778,
  12927,13077,13228,13381,13535,13690,13847,14005,14165,14326,14489,14653,14819,14986,15154,15324,
  15495,15668,15842,16018,16195,16374,16554,16736,16919,17104,17291,17479,17669,17860,18053,18248,
  18444,18642,18842,19043,19246,19451,19657,19865,20075,20287,20500,20715,20932,21151,21371,21593,
  21817,22043,22271,22500,22731,22964,23199,23436,23675,23916,24159,24404,24651,24900,25151,25404,
  25659,25916,26175,26436,26699,26964,27231,27500,27771,28044,28319,28597,28876,29158,29441,29727,
  30015,30305,30597,30891,31188,31486,31787,32090,32395,32702,33012,33324,33638,33954,34273,34594,
  34917,35243,35570,35900,36233,36568,36905,37245,37587,37931,38278,38627,38979,39333,39690,40049,
  40411,40775,41142,41511,41883,42258,42635,43014,43396,43781,44168,44558,44950,45345,45743,46143,
  46546,46952,47361,47772,48186,48603,49023,49445,49870,50298,50729,51163,51599,52039,52481,52926,
  53374,53825,54279,54736,55196,55659,56125,56594,57066,57541,58019,58500,58984,59471,59961,60454,
  60950,61449,61951,62456,62965,63476,63991,64509,65030,65535
};

void backlight_init() {
  pinMode(GFX_BL, OUTPUT);
  analogWriteFrequency(GFX_BL, BL_HZ);
  analogWriteResolution(GFX_BL, BL_RES);
  
  // NOUVEAU: Démarrage progressif pour éviter flash
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
  
  // NOUVEAU: Application courbe gamma pour perception linéaire
  const uint16_t gammaCorrected = gammaTable[p];
  const uint16_t duty = (uint16_t)((bl_maxduty() * (uint32_t)gammaCorrected) / 65535u);
  
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
