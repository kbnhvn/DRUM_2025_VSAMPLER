#include <Arduino.h>
// Utiliser les objets globaux déclarés dans le .ino principal
extern Arduino_GFX *gfx;

static const int BL_CH = 0, BL_HZ = 1000, BL_RES = 8;
static int g_brightness_percent = 100;

void setBacklightPercent(int p){
  p = constrain(p, 0, 100);
  int duty = (p * 255) / 100;
  ledcWrite(BL_CH, duty);
  g_brightness_percent = p;
}
int getBacklightPercent(){ return g_brightness_percent; }

void backlight_init(){
  pinMode(GFX_BL, OUTPUT);
  ledcSetup(BL_CH, BL_HZ, BL_RES);
  ledcAttachPin(GFX_BL, BL_CH);
  setBacklightPercent(100);
}
