#include <Arduino.h>
#include <WiFi.h>
#include "views.h"

extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt, bool pressed);  // SUPPRIMER = false
extern void flashButton(int x, int y, int w, int h, int color, const char* texto);
extern void setBacklightPercent(int p);
extern int  getBacklightPercent();
extern void wifiConnectHome();
extern void wifiConnectPhone();
extern void startFileServer();
extern void stopFileServer();
extern bool isFileServerOn();
extern Arduino_GFX *gfx;

View currentView = VIEW_MAIN;

void drawMenuView(){
  gfx->fillScreen(BLACK);
  drawTopBar("MENU", true);

  drawButtonBox( 20, 60, 140, 50, DARKGREY, "WiFi HOME");
  drawButtonBox(180, 60, 140, 50, DARKGREY, "WiFi PHONE");
  drawButtonBox(340, 60, 140, 50, isFileServerOn() ? GREEN : DARKGREY, "Server");

  gfx->setCursor(20, 130);
  gfx->setTextColor(WHITE, BLACK);
  gfx->print("IP: ");
  gfx->print(WiFi.localIP());
  
  gfx->setCursor(20, 145);
  gfx->setTextColor(WHITE, BLACK);
  gfx->print("Status: ");
  gfx->print(WiFi.isConnected() ? "Connected" : "Disconnected");

  int bp = getBacklightPercent();
  gfx->setCursor(20, 180);
  gfx->setTextColor(WHITE, BLACK);
  gfx->print("Brightness: ");
  gfx->print(bp);
  gfx->print("%");

  drawButtonBox( 20, 200,  80, 40, DARKGREY, "-");
  drawButtonBox(120, 200,  80, 40, DARKGREY, "+");
  
  // Zone de debug pour Back button
  gfx->drawRect(400, 0, 80, 30, RED);
  gfx->setCursor(405, 15);
  gfx->setTextColor(RED, BLACK);
  gfx->print("BACK");
}

void openMenuView(){
  currentView = VIEW_MENU;
  drawMenuView();
}

void handleTouchMenu(int x,int y){
  Serial.printf("[MENU] Touch at x=%d, y=%d\n", x, y);
  
  // Boutons Wi-Fi/Server avec feedback
  if (y>=60 && y<=110){
    if (x>= 20 && x<=160){ 
      flashButton(20, 60, 140, 50, DARKGREY, "WiFi HOME");
      Serial.println("[MENU] Connecting to home WiFi...");
      wifiConnectHome();  
      delay(1000); // Laisser le temps de connecter
      drawMenuView(); 
      return; 
    }
    if (x>=180 && x<=320){ 
      flashButton(180, 60, 140, 50, DARKGREY, "WiFi PHONE");
      Serial.println("[MENU] Connecting to phone WiFi...");
      wifiConnectPhone(); 
      delay(1000);
      drawMenuView(); 
      return; 
    }
    if (x>=340 && x<=480){
      if (isFileServerOn()) {
        flashButton(340, 60, 140, 50, GREEN, "Server");
        Serial.println("[MENU] Stopping file server");
        stopFileServer(); 
      } else {
        flashButton(340, 60, 140, 50, DARKGREY, "Server");
        Serial.println("[MENU] Starting file server");
        startFileServer();
      }
      drawMenuView(); 
      return;
    }
  }
  
  // Brightness avec feedback
  if (y>=200 && y<=240){
    if (x>=20 && x<=100){
      flashButton(20, 200, 80, 40, DARKGREY, "-");
      setBacklightPercent(getBacklightPercent()-10);
      drawMenuView(); 
      return;
    }
    if (x>=120 && x<=200){
      flashButton(120, 200, 80, 40, DARKGREY, "+");
      setBacklightPercent(getBacklightPercent()+10);
      drawMenuView(); 
      return;
    }
  }
  
  // BACK - Zone très large pour debug
  if (y>=0 && y<=30 && x>=400 && x<=480){ 
    Serial.println("[MENU] BACK pressed - returning to VIEW_MAIN");
    // Flash du bouton back
    gfx->fillRect(400, 0, 80, 30, WHITE);
    gfx->setCursor(405, 15);
    gfx->setTextColor(BLACK, WHITE);
    gfx->print("BACK");
    delay(200);
    currentView = VIEW_MAIN; 
    return; 
  }
  
  Serial.printf("[MENU] Touch ignored at x=%d, y=%d\n", x, y);
}