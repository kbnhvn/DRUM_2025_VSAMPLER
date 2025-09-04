#include <Arduino.h>
#include <WiFi.h>
#include "views.h"

extern void drawTopBar(const char* title, bool showBack);
extern void drawModernButton(int x, int y, int w, int h, int color, const char* text, bool active, bool pressed);
extern void flashButton(int x, int y, int w, int h, int color, const char* texto);
extern void drawProgressBar(int x, int y, int w, int h, int percent, int color);
extern void setBacklightPercent(int p);
extern int  getBacklightPercent();
extern void switchAudioOutput(bool useDAC);
extern void forceCompleteRedraw();

// Déclarations WiFi functions
extern void wifiConnectHome();
extern void wifiConnectPhone();
extern void startFileServer();
extern void stopFileServer();
extern bool isFileServerOn();

// Secrets WiFi
#include "secrets.h"
#ifdef __has_include
  #if __has_include("secrets.h")
    #include "secrets.h"
  #endif
#endif
#ifndef WIFI_HOME_SSID
  #define WIFI_HOME_SSID "xxxxxxxxxxxx"
  #define WIFI_HOME_PSK "xxxxxxxxxxxx"
  #define WIFI_PHONE_SSID "xxxxxxxxxxxx"
  #define WIFI_PHONE_PSK "xxxxxxxxxxxx"
#endif

extern Arduino_GFX *gfx;

View currentView = VIEW_MAIN;

static bool useExternalDAC = false;      // NOUVEAU: Switch audio HP/DAC
static unsigned long lastAnimFrame = 0;  // NOUVEAU: Animation timing
static int wifiConnectAnim = 0;          // NOUVEAU: Animation WiFi

// NOUVEAU: Status WiFi avec animations
void drawWiFiStatus() {
  // Zone status avec design card moderne
  gfx->fillRect(20, 130, 440, 60, UI_SURFACE);
  gfx->drawRect(20, 130, 440, 60, UI_ON_SURFACE);
  gfx->drawRect(21, 131, 438, 58, RGB565(60, 65, 75));
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnectAnim = 0;
    
    gfx->setTextColor(UI_SUCCESS, UI_SURFACE);
    gfx->setCursor(30, 145);
    gfx->print("CONNECTED");
    
    gfx->setTextColor(UI_ON_SURFACE, UI_SURFACE);
    gfx->setCursor(30, 155);
    gfx->print("IP: ");
    gfx->print(WiFi.localIP());
    
    gfx->setCursor(30, 165);
    gfx->print("SSID: ");
    gfx->print(WiFi.SSID());
    
    gfx->setCursor(30, 175);
    gfx->print("Signal: ");
    gfx->print(WiFi.RSSI());
    gfx->print(" dBm");
    
    // Signal strength bar
    int signal = constrain(map(WiFi.RSSI(), -90, -30, 0, 100), 0, 100);
    drawProgressBar(300, 160, 120, 15, signal, signal > 50 ? UI_SUCCESS : UI_WARNING);
    
  } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
    wifiConnectAnim = 0;
    gfx->setTextColor(UI_DANGER, UI_SURFACE);
    gfx->setCursor(30, 155);
    gfx->print("CONNECTION FAILED");
    
  } else {
    // Animation de connexion
    if (millis() - lastAnimFrame > 200) {
      wifiConnectAnim = (wifiConnectAnim + 1) % 4;
      lastAnimFrame = millis();
    }
    
    gfx->setTextColor(UI_WARNING, UI_SURFACE);
    gfx->setCursor(30, 145);
    gfx->print("CONNECTING");
    
    // Animation dots
    gfx->setCursor(130, 145);
    for (int i = 0; i < 3; i++) {
      gfx->setTextColor(i < wifiConnectAnim ? UI_WARNING : UI_SURFACE, UI_SURFACE);
      gfx->print(".");
    }
    
    // Progress animation
    int animPercent = (wifiConnectAnim * 25) % 100;
    drawProgressBar(30, 165, 200, 12, animPercent, UI_WARNING);
  }
}

bool detectBatteryMode() {
  // MÉTHODE 3: Variable manuelle (temporaire)
  return true; // Forcez true pour tester
}

void showUSBSleepWarning() {
  // Message d'info mode USB
  gfx->fillRect(80, 100, 320, 80, RGB565(40, 40, 20));
  gfx->drawRect(80, 100, 320, 80, RGB565(255, 200, 0));
  
  gfx->setTextColor(RGB565(255, 200, 0), RGB565(40, 40, 20));
  gfx->setCursor(170, 125);
  gfx->print("USB MODE");
  
  gfx->setTextColor(WHITE, RGB565(40, 40, 20));
  gfx->setCursor(120, 145);
  gfx->print("Switch to battery for");
  gfx->setCursor(140, 160);
  gfx->print("sleep functionality");
  
  delay(2500);
  drawMenuView(); // Redraw menu
}

void showSleepConfirmation() {
  // Écran de confirmation moderne
  gfx->fillScreen(RGB565(20, 20, 25));
  
  // Titre avec icône sleep
  gfx->setTextColor(RGB565(100, 50, 150), RGB565(20, 20, 25));
  gfx->setCursor(180, 80);
  gfx->print("SLEEP MODE");
  
  // Info
  gfx->setTextColor(WHITE, RGB565(20, 20, 25));
  gfx->setCursor(140, 110);
  gfx->print("Device will enter");
  gfx->setCursor(160, 130);
  gfx->print("deep sleep mode");
  
  gfx->setTextColor(RGB565(150, 150, 150), RGB565(20, 20, 25));
  gfx->setCursor(120, 150);
  gfx->print("Press SW1 to wake up");
  
  // Boutons de confirmation
  drawModernButton(120, 180, 100, 35, UI_SUCCESS, "CONFIRM", true, false);
  drawModernButton(260, 180, 100, 35, UI_DANGER, "CANCEL", true, false);
  
  // Barre de progression (auto-cancel après 10s)
  gfx->drawRect(140, 230, 200, 12, UI_ON_SURFACE);
  gfx->fillRect(142, 232, 196, 8, RGB565(40, 40, 45));
  
  // Attendre choix utilisateur
  unsigned long startTime = millis();
  const unsigned long timeout = 10000; // 10 secondes
  
  while (millis() - startTime < timeout) {
    read_touch();
    
    if (touchActivo) {
      // Bouton CONFIRM
      if (coy >= 180 && coy <= 215 && cox >= 120 && cox <= 220) {
        drawModernButton(120, 180, 100, 35, UI_SUCCESS, "CONFIRM", true, true);
        delay(200);
        
        Serial.println("[MENU] Sleep confirmed");
        performSoftwareSleep();
        return;
      }
      
      // Bouton CANCEL
      if (coy >= 180 && coy <= 215 && cox >= 260 && cox <= 360) {
        drawModernButton(260, 180, 100, 35, UI_DANGER, "CANCEL", true, true);
        delay(200);
        
        Serial.println("[MENU] Sleep cancelled");
        drawMenuView();
        return;
      }
    }
    
    // Mise à jour barre de progression
    unsigned long elapsed = millis() - startTime;
    int progress = (elapsed * 196) / timeout;
    gfx->fillRect(142, 232, progress, 8, RGB565(100, 50, 150));
    
    delay(50);
  }
  
  // Timeout - annulation automatique
  Serial.println("[MENU] Sleep timeout - cancelled");
  gfx->setTextColor(RGB565(255, 100, 100), RGB565(20, 20, 25));
  gfx->setCursor(190, 250);
  gfx->print("TIMEOUT");
  delay(1000);
  
  drawMenuView();
}

void performSoftwareSleep() {
  Serial.println("[POWER] === SOFTWARE SLEEP INITIATED ===");
  
  // Écran de préparation
  gfx->fillScreen(BLACK);
  gfx->setTextColor(RGB565(100, 50, 150), BLACK);
  gfx->setCursor(150, 110);
  gfx->print("ENTERING SLEEP");
  
  gfx->setTextColor(WHITE, BLACK);
  gfx->setCursor(170, 140);
  gfx->print("Saving...");
  
  // Arrêter l'audio
  for (int i = 0; i < 16; i++) {
    latch[i] = 0;
  }
  
  // Sauvegarder l'état
  save_pattern(0);
  save_sound(0);
  Serial.println("[POWER] State saved");
  
  // Arrêter les services
  if (isFileServerOn()) {
    stopFileServer();
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // Animation de fermeture
  gfx->setCursor(160, 160);
  gfx->print("Going to sleep...");
  
  for (int i = 3; i > 0; i--) {
    gfx->fillRect(160, 180, 160, 20, BLACK);
    gfx->setCursor(200, 195);
    gfx->printf("Sleep in %d", i);
    delay(1000);
  }
  
  // Réduction progressive du backlight
  for (int brightness = getBacklightPercent(); brightness > 0; brightness -= 10) {
    setBacklightPercent(brightness);
    delay(50);
  }
  
  gfx->fillScreen(BLACK);
  
  Serial.println("[POWER] Entering deep sleep");
  Serial.println("[POWER] Wake up: Switch SW1 to battery mode");
  Serial.flush();
  
  // DEEP SLEEP - Wake-up possible par reset/power cycle
  esp_deep_sleep_start();
  
  // Cette ligne ne sera jamais atteinte
}

void drawMenuView(){
  gfx->fillScreen(RGB565(15, 15, 20)); // Fond moderne
  
  // Header avec gradient
  for (int y = 0; y < 30; y++) {
    int shade = map(y, 0, 30, 80, 40);
    gfx->drawLine(0, y, 480, y, RGB565(shade, shade + 5, shade + 10));
  }
  
  gfx->setTextColor(WHITE, RGB565(60, 65, 75));
  gfx->setCursor(20, 20);
  gfx->print("SYSTEM MENU");

  // Boutons WiFi et serveur avec états visuels
  bool wifiConnected = WiFi.isConnected();
  bool isHome = wifiConnected && WiFi.SSID() == WIFI_HOME_SSID;
  bool isPhone = wifiConnected && WiFi.SSID() == WIFI_PHONE_SSID;
  bool serverActive = isFileServerOn();
  
  drawModernButton( 20, 50, 120, 60, UI_PRIMARY, "HOME\nWiFi", isHome, false);
  drawModernButton(160, 50, 120, 60, UI_PRIMARY, "PHONE\nWiFi", isPhone, false);
  drawModernButton(300, 50, 120, 60, UI_SUCCESS, "SERVER", serverActive, false);
  
  // NOUVEAU: Switch audio moderne avec design toggle
  int toggleX = 20, toggleY = 200;
  int toggleW = 200, toggleH = 40;
  
  gfx->fillRect(toggleX, toggleY, toggleW, toggleH, useExternalDAC ? UI_SUCCESS : UI_SURFACE);
  gfx->drawRect(toggleX, toggleY, toggleW, toggleH, useExternalDAC ? UI_SUCCESS : UI_ON_SURFACE);
  
  // Indicateur slide
  int switchX = useExternalDAC ? toggleX + toggleW - 60 : toggleX + 10;
  gfx->fillRect(switchX, toggleY + 5, 50, toggleH - 10, useExternalDAC ? WHITE : UI_ON_SURFACE);
  gfx->drawRect(switchX, toggleY + 5, 50, toggleH - 10, useExternalDAC ? UI_SUCCESS : RGB565(80, 85, 95));
  
  gfx->setTextColor(useExternalDAC ? BLACK : UI_ON_SURFACE, useExternalDAC ? UI_SUCCESS : UI_SURFACE);
  gfx->setCursor(toggleX + 15, toggleY + 25);
  gfx->print(useExternalDAC ? "PCM5102A" : "INTERNAL");

  // Status WiFi avec design moderne
  drawWiFiStatus();

  // Brightness avec contrôles modernes

  int bp = getBacklightPercent();
  gfx->setTextColor(UI_ACCENT, RGB565(15, 15, 20));
  gfx->setCursor(300, 120);
  gfx->print("Brightness: ");

  drawModernButton(300, 200, 60, 40, UI_PRIMARY, "-", true, false);
  drawProgressBar(370, 205, 80, 30, bp, UI_ACCENT);
  drawModernButton(460, 200, 60, 40, UI_PRIMARY, "+", true, false);  
  
  // NOUVEAU: BOUTON SLEEP
  // Détection mode batterie (ajustez selon votre hardware)
  bool onBattery = detectBatteryMode();
  
  if (onBattery) {
    // Bouton Sleep actif si sur batterie
    drawModernButton(250, 130, 120, 60, RGB565(100, 50, 150), "SLEEP\nMODE", true, false);
    
    // Info batterie
    gfx->setTextColor(UI_ON_SURFACE, RGB565(15, 15, 20));
    gfx->setCursor(250, 200);
    gfx->print("Battery Mode");
    
  } else {
    // Bouton grisé si USB
    drawModernButton(250, 130, 120, 60, RGB565(60, 60, 60), "SLEEP\nMODE", false, false);
    
    gfx->setTextColor(RGB565(120, 120, 120), RGB565(15, 15, 20));
    gfx->setCursor(250, 200);
    gfx->print("USB Mode");
  }

  // Back button moderne
  drawModernButton(400, 5, 70, 20, UI_DANGER, "BACK", true, false);
}

void openMenuView(){
  currentView = VIEW_MENU;
  lastAnimFrame = millis();
  wifiConnectAnim = 0;
  drawMenuView();
}

void handleTouchMenu(int x,int y){
  Serial.printf("[MENU] Touch at x=%d, y=%d\n", x, y);
  
  // Boutons WiFi/Server avec animations
  if (y >= 50 && y <= 110) {
    if (x >= 20 && x <= 140) { 
      drawModernButton(20, 50, 120, 60, UI_PRIMARY, "HOME\nWiFi", false, true);
      delay(100);
      Serial.println("[MENU] Connecting to home WiFi...");
      wifiConnectHome();  
      delay(1000); // Laisser le temps de connecter
      drawMenuView(); 
      return; 
    }
    if (x >= 160 && x <= 280) { 
      drawModernButton(160, 50, 120, 60, UI_PRIMARY, "PHONE\nWiFi", false, true);
      delay(100);
      Serial.println("[MENU] Connecting to phone WiFi...");
      wifiConnectPhone(); 
      delay(1000);
      drawMenuView(); 
      return; 
    }
    if (x >= 300 && x <= 420) {
      drawModernButton(300, 50, 120, 60, UI_SUCCESS, "SERVER", false, true);
      delay(100);
      if (isFileServerOn()) {
        Serial.println("[MENU] Stopping file server");
        stopFileServer(); 
      } else {
        Serial.println("[MENU] Starting file server");
        startFileServer();
      }
      drawMenuView(); 
      return;
    }
  }
  
  // Brightness avec feedback
  // NOUVEAU: Audio toggle avec animation slide
  if (y >= 200 && y <= 240 && x >= 20 && x <= 220) {
    bool oldDAC = useExternalDAC;
    useExternalDAC = !useExternalDAC;
    
    // Animation toggle slide
    for (int step = 0; step < 10; step++) {
      int progress = (step * 100) / 9;
      int switchX = useExternalDAC ? 
                   map(progress, 0, 100, 30, 160) : 
                   map(progress, 0, 100, 160, 30);
      
      // Redraw partiel zone toggle
      gfx->fillRect(20, 200, 200, 40, useExternalDAC ? UI_SUCCESS : UI_SURFACE);
      gfx->fillRect(switchX, 205, 50, 30, useExternalDAC ? WHITE : UI_ON_SURFACE);
      delay(20);
    }
    
    Serial.printf("[MENU] Audio switched to %s\n", 
                  useExternalDAC ? "External DAC" : "Internal HP");
    
    switchAudioOutput(useExternalDAC);
    drawMenuView();
    return;
  }
  
  // Brightness controls
  if (y >= 200 && y <= 240) {
    if (x >= 300 && x <= 360) {
      drawModernButton(300, 200, 60, 40, UI_PRIMARY, "-", true, true);
      delay(50);
      setBacklightPercent(getBacklightPercent() - 10);
      drawMenuView(); 
      return;
    }
    if (x >= 460 && x <= 520) {
      drawModernButton(460, 200, 60, 40, UI_PRIMARY, "+", true, true);
      delay(50);
      setBacklightPercent(getBacklightPercent() + 10);
      drawMenuView(); 
      return;
    }
  }

  // NOUVEAU: BOUTON SLEEP
  if (y >= 130 && y <= 190 && x >= 250 && x <= 370) {
    bool onBattery = detectBatteryMode();
    
    if (!onBattery) {
      // Mode USB - montrer message d'info
      showUSBSleepWarning();
      return;
    }
    
    // Mode batterie - demander confirmation
    Serial.println("[MENU] Sleep button pressed");
    
    drawModernButton(250, 130, 120, 60, RGB565(100, 50, 150), "SLEEP\nMODE", true, true);
    delay(100);
    
    showSleepConfirmation();
    return;
  }
  
  // Back avec animation de transition
  if (y >= 5 && y <= 25 && x >= 400 && x <= 470) { 
    Serial.println("[MENU] BACK pressed - returning to VIEW_MAIN");
    
    drawModernButton(400, 5, 70, 20, UI_DANGER, "BACK", true, true);
    
    // Effet fade out vers main
    for (int alpha = 0; alpha < 8; alpha++) {
      gfx->fillRect(0, 0, 480, 272, RGB565(alpha * 2, alpha * 2, alpha * 2));
      delay(30);
    }
 
    currentView = VIEW_MAIN;
    // CORRECTION: S'assurer que l'UI principale reprend proprement
    pauseMainUIRefresh();  // Reset état
    forceCompleteRedraw();
    resumeMainUIRefresh(); // Relancer
    return; 
  }
  
  // Animation continue pour WiFi en cours de connexion
  if (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED) {
    if (millis() - lastAnimFrame > 300) {
      drawWiFiStatus(); // Refresh animation
    }
  }
}