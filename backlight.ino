// Backlight control (PWM)
#include <Arduino.h>
extern Arduino_GFX *gfx;   // défini dans le .ino principal

#ifndef GFX_BL
#define GFX_BL 1
#endif

static const uint8_t BL_RES = 8;
static const uint32_t BL_HZ = 20000;
static inline uint16_t bl_maxduty() { return (1u << BL_RES) - 1u; }

void backlight_init(){
  pinMode(GFX_BL, OUTPUT);
  analogWriteFrequency(GFX_BL, BL_HZ);
  analogWriteResolution(GFX_BL, BL_RES);
  analogWrite(GFX_BL, bl_maxduty()); // 100%
}

void setBacklightPercent(int p){
  if (p < 0)   p = 0;
  if (p > 100) p = 100;
  const uint16_t duty = (uint16_t)((bl_maxduty() * (uint32_t)p) / 100u);
  analogWrite(GFX_BL, duty);
}

int getBacklightPercent(){
  return 100;
}
