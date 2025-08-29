extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);
extern void setBacklightPercent(int p);
extern int  getBacklightPercent();
void wifiConnectHome();
void wifiConnectPhone();

enum View { VIEW_MAIN, VIEW_MENU, VIEW_BROWSER, VIEW_WIFI, VIEW_PICKER, VIEW_PATTERN, VIEW_SONG };
View currentView = VIEW_MAIN;

void openMenuView(){ currentView = VIEW_MENU; drawMenuView(); }

void drawMenuView(){
  gfx->fillScreen(BLACK);
  drawTopBar("MENU", true);

  drawButtonBox(20, 60, 140, 50, DARKGREY, "WiFi HOME");
  drawButtonBox(180,60, 140, 50, DARKGREY, "WiFi PHONE");
  drawButtonBox(340,60, 140, 50, DARKGREY, "Server ON");
  gfx->setCursor(20,130); gfx->print("IP: "); gfx->print(WiFi.localIP());

  int bp = getBacklightPercent();
  gfx->setCursor(20, 180); gfx->print("Brightness: "); gfx->print(bp); gfx->print("%");
  drawButtonBox(20, 200, 80, 40, DARKGREY, "-");
  drawButtonBox(120,200, 80, 40, DARKGREY, "+");
}

// (si tu veux gérer les touches brutes ici, on ajoutera un dispatcher tactile)
