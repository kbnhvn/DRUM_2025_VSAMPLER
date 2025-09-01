#include <Arduino.h>
#include <Wire.h>
#include "views.h"

// Touch GT911 avec améliorations
// Touch GT911 (pins/addr déjà définis ailleurs)
extern int BPOS[48][4];
extern bool touchActivo;
extern int  last_touched;
extern long start_debounce;
extern const int debounce_time;
extern int  cox, coy;

// Triggers et états UI
extern byte trigger_on[48];
extern View currentView;
extern void forceCompleteRedraw();

// GFX (si besoin de logs à l’écran)
extern Arduino_GFX *gfx;
extern void synthESP32_TRIGGER(uint8_t voice);

// Protos handlers vues secondaires
void handleTouchMenu(int x,int y);
void handleTouchPattern(int x,int y);
void handleTouchSong(int x,int y);
void handleTouchBrowser(int x,int y);
void handleTouchPicker(int x,int y);

// GT911
#define GT911_ADDR 0x5D

// Reset GT911
void resetGT911() {
  pinMode(TOUCH_RST, OUTPUT);
  pinMode(TOUCH_INT, OUTPUT);
  digitalWrite(TOUCH_INT, LOW);
  delay(5);
  digitalWrite(TOUCH_RST, LOW);
  delay(20);  // NOUVEAU: Délai plus long pour stabilité
  digitalWrite(TOUCH_RST, HIGH);
  delay(100); // NOUVEAU: Délai plus long
  pinMode(TOUCH_INT, INPUT);
  delay(100);

  Serial.println("[TOUCH] GT911 reset completed");
}

// I2C helpers
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

// NOUVEAU: Lecture tactile robuste avec validation
void read_touch() {
  // Vérification I2C disponible
  if (!Wire.available() && touchActivo) {
    // Potentiel timeout I2C, reset de l'état
    touchActivo = false;
    return;
  }
 
  uint8_t status = readRegister8(0x814E);

  if (status & 0x80) { // Touch detected
    if (!touchActivo) {
      touchActivo = true;

      // NOUVEAU: Validation du délai debounce
      if (last_touched >= 0 && (start_debounce + debounce_time > (long)millis())) {
        writeRegister8(0x814E, 0x00); // Clear interrupt
        return; // Encore en période de debounce
      }

      // Lit le premier point
      uint8_t data[8] = {0};
      readRegisterMultiple(0x8150, data, 8);
      uint16_t x = (data[1] << 8) | data[0];
      uint16_t y = (data[3] << 8) | data[2];
      
      // NOUVEAU: Validation coordonnées
      if (x >= 480 || y >= 272) {
        Serial.printf("[TOUCH] Invalid coordinates: x=%d, y=%d\n", x, y);
        touchActivo = false;
        writeRegister8(0x814E, 0x00);
        return;
      }
 
      cox = x; coy = y;

      Serial.printf("[TOUCH] Detected at x=%d, y=%d in view %d\n", cox, coy, currentView);

      // Dispatch selon la vue
      switch (currentView){
        case VIEW_MAIN:
          // Mapping sur grille principale avec validation
          for (byte f = 0; f < 48; f++) {
            int x0 = BPOS[f][0], y0 = BPOS[f][1], w = BPOS[f][2], h = BPOS[f][3];
            
            // NOUVEAU: Validation des bounds
            if (w <= 0 || h <= 0) continue;
            if ((cox > x0) && (cox < x0+w) && (coy > y0) && (coy < y0+h)) {
              // NOUVEAU: Double vérification debounce
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
          Serial.printf("[TOUCH] Unknown view %d, returning to main\n", currentView);
          currentView = VIEW_MAIN;
          forceCompleteRedraw();
          break;
      }
    }
  } else {
    // NOUVEAU: Reset state plus propre
    if (touchActivo) {
      Serial.println("[TOUCH] Touch released");
    }
    touchActivo = false;
  }

  // Clear interrupt/status
  writeRegister8(0x814E, 0x00);
}

// NOUVEAU: Fonction de diagnostic touch
void diagnoseTouchSystem() {
  Serial.println("[TOUCH] === Touch System Diagnostics ===");
  
  // Test I2C communication
  Wire.beginTransmission(GT911_ADDR);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("[TOUCH] GT911 I2C communication: OK");
    
    // Lire quelques registres de base
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
  
  // État des boutons
  Serial.printf("[TOUCH] Current view: %d\n", currentView);
  Serial.printf("[TOUCH] Touch active: %s\n", touchActivo ? "YES" : "NO");
  Serial.printf("[TOUCH] Last touched: %d\n", last_touched);
  Serial.printf("[TOUCH] Debounce remaining: %ld ms\n", 
                max(0L, (start_debounce + debounce_time) - (long)millis()));
  
  Serial.println("[TOUCH] === End Diagnostics ===");
}
