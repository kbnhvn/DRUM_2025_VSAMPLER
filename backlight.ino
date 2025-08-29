// Backlight control (PWM)
#include <Arduino.h>
#include "esp32-hal-ledc.h"
extern Arduino_GFX *gfx;   // défini dans le .ino principal

#ifndef GFX_BL
#define GFX_BL 1
#endif

static const uint8_t BL_CH  = 0;
static const uint8_t BL_RES = 8;
static const double  BL_HZ  = 20000.0;

static inline uint16_t bl_maxduty() { return (1u << BL_RES) - 1u; }

void backlight_init(){
  pinMode(GFX_BL, OUTPUT);
  ledcSetup(BL_CH, BL_HZ, BL_RES);
  ledcAttachPin(GFX_BL, BL_CH);
  // 100% par défaut
  ledcWrite(BL_CH, bl_maxduty());
}

void setBacklightPercent(int p){
  if (p < 0)   p = 0;
  if (p > 100) p = 100;
  const uint16_t duty = (uint16_t)((bl_maxduty() * (uint32_t)p) / 100u);
  ledcWrite(BL_CH, duty);
}

int getBacklightPercent(){
  const int duty = ledcRead(BL_CH);
  const int maxd = bl_maxduty();
  if (maxd <= 0) return 0;
  return (duty * 100) / maxd;
}
