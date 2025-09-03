// ========== CORRECTIONS TOUCH.INO ==========
#include <Arduino.h>
#include <Wire.h>
#include "views.h"

extern int BPOS[48][4];
extern bool touchActivo;
extern int  last_touched;
extern long start_debounce;
extern const int debounce_time;
extern int  cox, coy;

extern byte trigger_on[48];
extern View currentView;
extern void forceCompleteRedraw();

extern Arduino_GFX *gfx;
extern void synthESP32_TRIGGER(uint8_t voice);

void handleTouchMenu(int x,int y);
void handleTouchPattern(int x,int y);
void handleTouchSong(int x,int y);
void handleTouchBrowser(int x,int y);
void handleTouchPicker(int x,int y);

#define GT911_ADDR 0x5D

void resetGT911() {
  pinMode(TOUCH_RST, OUTPUT);
  pinMode(TOUCH_INT, OUTPUT);
  digitalWrite(TOUCH_INT, LOW);
  delay(5);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);
  digitalWrite(TOUCH_RST, HIGH);
  delay(100);
  pinMode(TOUCH_INT, INPUT);
  delay(100);

  Serial.println("[TOUCH] GT911 reset completed");
}

static uint8_t readRegister8(uint16_t reg) {
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.endTransmission(false);
  Wire.requestFrom(GT911_ADDR, 1);
  
  if (!Wire.available()) {
    Serial.printf("[TOUCH] I2C read timeout for reg 0x%04X\n", reg);
  }
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
  
  if (Wire.available() < length) {
    Serial.printf("[TOUCH] I2C partial read: got %d, expected %d\n", Wire.available(), length);
  }
}

static void writeRegister8(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg & 0xFF);
  Wire.write(value);
  Wire.endTransmission();
}

void read_touch() {
  if (!Wire.available() && touchActivo) {
    touchActivo = false;
    return;
  }
 
  uint8_t status = readRegister8(0x814E);

  if (status & 0x80) { // Touch detected
    if (!touchActivo) {
      touchActivo = true;

      if (last_touched >= 0 && (start_debounce + debounce_time > (long)millis())) {
        writeRegister8(0x814E, 0x00);
        return;
      }

      uint8_t data[8] = {0};
      readRegisterMultiple(0x8150, data, 8);
      uint16_t x = (data[1] << 8) | data[0];
      uint16_t y = (data[3] << 8) | data[2];
      
      if (x >= 480 || y >= 272) {
        Serial.printf("[TOUCH] Invalid coordinates: x=%d, y=%d\n", x, y);
        touchActivo = false;
        writeRegister8(0x814E, 0x00);
        return;
      }
 
      cox = x; coy = y;

      Serial.printf("[TOUCH] Detected at x=%d, y=%d in view %d\n", cox, coy, (int)currentView);

      // CORRECTION CRITIQUE: Le switch doit être exhaustif et correct
      switch (currentView){
        case VIEW_MAIN:
          Serial.println("[TOUCH] Processing main view touch");
          // Traitement des zones tactiles principales
          for (byte f = 0; f < 48; f++) {
            int x0 = BPOS[f][0], y0 = BPOS[f][1], w = BPOS[f][2], h = BPOS[f][3];
            
            if (w <= 0 || h <= 0) continue;
            if ((cox > x0) && (cox < x0+w) && (coy > y0) && (coy < y0+h)) {
              if (f == last_touched && (start_debounce + debounce_time > (long)millis())) {
                Serial.printf("[TOUCH] Debounce active for button %d\n", f);
                break;
              }
              
              trigger_on[f] = 1;
              last_touched = f;
              start_debounce = millis();
              
              Serial.printf("[TOUCH] Main view button %d triggered\n", f);
              break;
            }
          }
          break;

        case VIEW_MENU:
          Serial.println("[TOUCH] Processing menu view touch");
          handleTouchMenu(cox, coy);
          memset(trigger_on, 0, sizeof(trigger_on));
          last_touched = -1;
          break;

        case VIEW_PATTERN:
          Serial.println("[TOUCH] Processing pattern view touch");
          handleTouchPattern(cox, coy);
          memset(trigger_on, 0, sizeof(trigger_on));
          last_touched = -1;
          break;

        case VIEW_SONG:
          Serial.println("[TOUCH] Processing song view touch");
          handleTouchSong(cox, coy);
          memset(trigger_on, 0, sizeof(trigger_on));
          last_touched = -1;
          break;

        case VIEW_BROWSER:
          Serial.println("[TOUCH] Processing browser view touch");
          handleTouchBrowser(cox, coy);
          memset(trigger_on, 0, sizeof(trigger_on));
          last_touched = -1;
          break;

        case VIEW_PICKER:
          Serial.println("[TOUCH] Processing picker view touch");
          handleTouchPicker(cox, coy);
          memset(trigger_on, 0, sizeof(trigger_on));
          last_touched = -1;
          break;

        case VIEW_WIFI:
          Serial.println("[TOUCH] Processing WiFi view touch (if implemented)");
          // handleTouchWifi(cox, coy); // Si cette vue existe
          memset(trigger_on, 0, sizeof(trigger_on));
          last_touched = -1;
          break;

        default:
          // CORRECTION: Ne plus forcer le retour à main automatiquement
          Serial.printf("[TOUCH] Unhandled view %d at x=%d, y=%d\n", (int)currentView, cox, coy);
          // NE PAS changer automatiquement de vue ici
          // currentView = VIEW_MAIN;
          // forceCompleteRedraw();
          break;
      }
    }
  } else {
    if (touchActivo) {
      Serial.println("[TOUCH] Touch released");
    }
    touchActivo = false;
  }

  writeRegister8(0x814E, 0x00);
}

void diagnoseTouchSystem() {
  Serial.println("[TOUCH] === Touch System Diagnostics ===");
  
  Wire.beginTransmission(GT911_ADDR);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("[TOUCH] GT911 I2C communication: OK");
    
    uint8_t productId[4];
    readRegisterMultiple(0x8140, productId, 4);
    Serial.printf("[TOUCH] Product ID: %c%c%c%c\n", 
                  productId[0], productId[1], productId[2], productId[3]);
    
    uint16_t version = readRegister8(0x8144) | (readRegister8(0x8145) << 8);
    Serial.printf("[TOUCH] Firmware version: 0x%04X\n", version);
    
  } else {
    Serial.printf("[TOUCH] GT911 I2C error: %d\n", error);
    Serial.println("[TOUCH] Attempting reset...");
    resetGT911();
  }
  
  Serial.printf("[TOUCH] Current view: %d\n", currentView);
  Serial.printf("[TOUCH] Touch active: %s\n", touchActivo ? "YES" : "NO");
  Serial.printf("[TOUCH] Last touched: %d\n", last_touched);
  Serial.printf("[TOUCH] Debounce remaining: %ld ms\n", 
                max(0L, (start_debounce + debounce_time) - (long)millis()));
  
  Serial.println("[TOUCH] === End Diagnostics ===");
}

void changeView(View newView) {
  Serial.printf("[VIEW] Changing from view %d to view %d\n", (int)currentView, (int)newView);
  currentView = newView;
  
  // Reset des états tactiles
  memset(trigger_on, 0, sizeof(trigger_on));
  touchActivo = false;
  show_last_touched = false;
  last_touched = -1;
  
  // forceCompleteRedraw();
}