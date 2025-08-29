#include <Arduino.h>
#include <WiFi.h>
#include "views.h"

// Helpers UI (déjà présents ailleurs)
extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);

// Backlight
extern void setBacklightPercent(int p);
extern int  getBacklightPercent();

// File server / Wi-Fi
extern void wifiConnectHome();
extern void wifiConnectPhone();
extern void startFileServer();
extern void stopFileServer();
extern bool isFileServerOn();

// GFX global (Arduino_GFX*), couleurs/positions (depuis LCD_tools.ino)
extern Arduino_GFX *gfx;
extern int BPOS[48][4];

// Routeur de vues (définition ici, extern ailleurs)
View currentView = VIEW_MAIN;

// Dimensions écran (JC4827W543 = 480×272)
static const int SCREEN_W = 480;
static const int SCREEN_H = 272;

// Zone du bouton Back dessiné par drawTopBar()
// (on le place coin haut-droit ~32×16 px)
static const int BACK_X0 = SCREEN_W - 40;
static const int BACK_X1 = SCREEN_W - 8;
static const int BACK_Y0 = 4;
static const int BACK_Y1 = 20;

void drawMenuView(){
  gfx->fillScreen(BLACK);
  drawTopBar("MENU", true);

  // Ligne 1 : Wi-Fi + Server
  drawButtonBox( 20, 60, 140, 50, DARKGREY, "WiFi HOME");
  drawButtonBox(180, 60, 140, 50, DARKGREY, "WiFi PHONE");
  drawButtonBox(340, 60, 140, 50, DARKGREY, "Server ON/OFF");

  gfx->setCursor(20, 130);
  gfx->setTextColor(WHITE, BLACK);
  gfx->print("IP: ");
  gfx->print(WiFi.localIP());

  // Ligne 2 : Brightness
  int bp = getBacklightPercent();
  gfx->setCursor(20, 180);
  gfx->setTextColor(WHITE, BLACK);
  gfx->print("Brightness: ");
  gfx->print(bp);
  gfx->print("%");

  drawButtonBox( 20, 200,  80, 40, DARKGREY, "-");
  drawButtonBox(120, 200,  80, 40, DARKGREY, "+");
}

void openMenuView(){
  currentView = VIEW_MENU;
  drawMenuView();
}

// Handler tactile du MENU
void handleTouchMenu(int x,int y){
  // Boutons Wi-Fi/Server
  if (y>=60 && y<=110){
    if (x>= 20 && x<=160){ wifiConnectHome();  drawMenuView(); return; }
    if (x>=180 && x<=320){ wifiConnectPhone(); drawMenuView(); return; }
    if (x>=340 && x<=480){
      if (isFileServerOn()) stopFileServer(); else startFileServer();
      drawMenuView(); return;
    }
  }
  // Brightness -
  if (y>=200 && y<=240 && x>=20  && x<=100){
    setBacklightPercent(getBacklightPercent()-10);
    drawMenuView(); return;
  }
  // Brightness +
  if (y>=200 && y<=240 && x>=120 && x<=200){
    setBacklightPercent(getBacklightPercent()+10);
    drawMenuView(); return;
  }
  // Back (coin haut-droit)
  if (y>=4 && y<=20 && x>=440 && x<=472){ currentView = VIEW_MAIN; return; }
  }
}
