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