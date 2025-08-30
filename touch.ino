#include <Arduino.h>
#include <Wire.h>
#include "views.h"

// Touch GT911 (pins/addr déjà définis ailleurs)
extern int BPOS[48][4];
extern bool touchActivo;
extern int  last_touched;
extern long start_debounce;
extern const int debounce_time;
extern int  cox, coy;

// Déclencheurs + états UI existants
extern byte trigger_on[48];

// GFX (si besoin de logs à l’écran)
extern Arduino_GFX *gfx;

// Protos handlers vues secondaires
void handleTouchMenu(int x,int y);
void handleTouchPattern(int x,int y);
void handleTouchSong(int x,int y);
void handleTouchBrowser(int x,int y);
void handleTouchPicker(int x,int y);

// Routeur de vues
extern View currentView;

// GT911
#define GT911_ADDR 0x5D

// Reset GT911
void resetGT911() {
  pinMode(TOUCH_RST, OUTPUT);
  pinMode(TOUCH_INT, OUTPUT);
  digitalWrite(TOUCH_INT, LOW);
  delay(5);
  digitalWrite(TOUCH_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);
  pinMode(TOUCH_INT, INPUT);
  delay(50);
}

// I2C helpers
static uint8_t readRegister8(uint16_t reg) {
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.endTransmission(false);
  Wire.requestFrom(GT911_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}
static void readRegisterMultiple(uint16_t reg, uint8_t *buffer, uint8_t length) {
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)GT911_ADDR, (uint8_t)length);
  for (uint8_t i = 0; i < length && Wire.available(); i++) {
    buffer[i] = Wire.read();
  }
}
static void writeRegister8(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.write(value);
  Wire.endTransmission();
}

// Lecture tactile + dispatch
void read_touch(){
  // Status de touche
  uint8_t status = readRegister8(0x814E);

  if (status & 0x80) { // touche détectée
    if (!touchActivo){
      touchActivo = true;

      // Lit le premier point
      uint8_t data[8] = {0};
      readRegisterMultiple(0x8150, data, 8);
      uint16_t x = (data[1] << 8) | data[0];
      uint16_t y = (data[3] << 8) | data[2];
      cox = x; coy = y;

      // Dispatch selon la vue
      switch (currentView){
        case VIEW_MAIN:
          // logique existante déjà exécutée dans le bloc de détection (mapping trigger_on[])
        {
          // Mapper la touche sur la grille principale (remplir trigger_on[])
          for (byte f=0; f<48; f++){
            int x0=BPOS[f][0], y0=BPOS[f][1], w=BPOS[f][2], h=BPOS[f][3];
            if ((cox > x0) && (cox < x0+w) && (coy > y0) && (coy < y0+h)) {
              if (f==last_touched && (start_debounce + debounce_time > (long)millis())) break;
              trigger_on[f]=1;
              last_touched=f;
              start_debounce=millis();
              break;
            }
          }
          break;
        } 

        case VIEW_MENU:
          handleTouchMenu(cox, coy);
          break;

        case VIEW_PATTERN:
          handleTouchPattern(cox, coy);
          break;

        case VIEW_SONG:
          handleTouchSong(cox, coy);
          break;

        case VIEW_BROWSER:
          handleTouchBrowser(cox, coy);
          break;

        case VIEW_PICKER:
          handleTouchPicker(cox, coy);
          break;

        default:
          break;
      }
    }
  } else {
    touchActivo = false;
  }

  // Clear interrupt/status
  writeRegister8(0x814E, 0x00);
}
