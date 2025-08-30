#include <ArduinoJson.h>
#include <SD.h>
#include "views.h"

// Déclaration avant-usage pour le pointeur global gfx
#include <vector>
class Arduino_GFX; 
extern Arduino_GFX *gfx;

extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);
 
extern uint16_t pattern[16];      // bitmask 16 steps / pad
extern int32_t  ROTvalue[16][8];  // SAM,INI,END,PIT,RVS,VOL,PAN,FIL
extern String   sound_names[];    // nom du sample pour affichage
extern View     currentView;


static int nextPatternNum(){
  if (!SD.exists("/patterns")) SD.mkdir("/patterns");
  File dir=SD.open("/patterns"); if (!dir) return 1;
  int maxN=0; while(true){ File f=dir.openNextFile(); if(!f) break;
    String nm=f.name(); if (nm.startsWith("/patterns/pattern_") && nm.endsWith(".json")){
      int n=nm.substring(18,nm.length()-5).toInt(); if (n>maxN) maxN=n;
    } f.close();
  } return maxN+1;
}

static void pattern_save_json(){
  if (!SD.exists("/patterns")) SD.mkdir("/patterns");
  String tmp = "/patterns/.tmp.json";
  String fin = "/patterns/pattern_"+String(nextPatternNum())+".json";
  File f=SD.open(tmp, FILE_WRITE); if (!f) return;

  DynamicJsonDocument doc(16384);
  auto pads = doc.createNestedArray("pads");
  for(int s=0;s<16;s++){
    auto po = pads.createNestedObject();
    po["name"]=sound_names[s];
    // params inline
    {
      auto pa = po.createNestedObject("params");
      pa["SAM"]=ROTvalue[s][0];
      pa["INI"]=ROTvalue[s][1];
      pa["END"]=ROTvalue[s][2];
      pa["PIT"]=ROTvalue[s][3];
      pa["RVS"]=ROTvalue[s][4];
      pa["VOL"]=ROTvalue[s][5];
      pa["PAN"]=ROTvalue[s][6];
      pa["FIL"]=ROTvalue[s][7];
    }
    // steps inline
    auto steps=po.createNestedArray("pattern");
    for(int st=0;st<16;st++) steps.add( (pattern[s] >> st) & 0x1 );
  }
  serializeJson(doc,f); f.close(); SD.rename(tmp, fin);
}

static void pattern_new(){ for(int s=0;s<16;s++) pattern[s]=0; }

static bool pattern_load_first(){
  File dir=SD.open("/patterns"); if (!dir) return false;
  File f=dir.openNextFile(); if (!f) return false;
  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc,f)) { f.close(); return false; }
  auto pads = doc["pads"].as<ArduinoJson::JsonArray>();
  for(int s=0;s<min(16,(int)pads.size());s++){
    auto po=pads[s].as<ArduinoJson::JsonObject>();
    sound_names[s]=po["name"].as<String>();
    // read params inline
    if (po.containsKey("params")){
      auto pa = po["params"].as<ArduinoJson::JsonObject>();
      if(pa.containsKey("SAM")) ROTvalue[s][0] = (int32_t) pa["SAM"];
      if(pa.containsKey("INI")) ROTvalue[s][1] = (int32_t) pa["INI"];
      if(pa.containsKey("END")) ROTvalue[s][2] = (int32_t) pa["END"];
      if(pa.containsKey("PIT")) ROTvalue[s][3] = (int32_t) pa["PIT"];
      if(pa.containsKey("RVS")) ROTvalue[s][4] = (int32_t) pa["RVS"];
      if(pa.containsKey("VOL")) ROTvalue[s][5] = (int32_t) pa["VOL"];
      if(pa.containsKey("PAN")) ROTvalue[s][6] = (int32_t) pa["PAN"];
      if(pa.containsKey("FIL")) ROTvalue[s][7] = (int32_t) pa["FIL"];
    }
    // read steps inline
    uint16_t m=0;
    if (po.containsKey("pattern")){
      auto steps=po["pattern"].as<ArduinoJson::JsonArray>();
      int n = min(16, (int)steps.size());
      for (int st=0; st<n; ++st) if (steps[st].as<int>()) m |= (1<<st);
    }
    pattern[s]=m;
  }
  f.close(); return true;
}

void openPatternView(){
  gfx->fillScreen(BLACK);
  drawTopBar("PATTERN", true);
  drawButtonBox(40,70,180,80, DARKGREY, "Save");
  drawButtonBox(240,70,180,80, DARKGREY, "New");
  drawButtonBox(440,70,180,80, DARKGREY, "Load");
  // Branches via DO_KEYPAD (boutons dédiés) ou dispatcher tactile si désiré
}

void handleTouchPattern(int x,int y){
  if (y>=70 && y<=150){
    if (x>=40 && x<=220){ pattern_save_json(); return; }
    if (x>=240 && x<=420){ pattern_new(); return; }
    // CORRIGÉ - coordonnées réalistes
    if (x>=440 && x<=480){ pattern_load_first(); return; }
  }
  // CORRIGÉ - zone Back plus large
  if (y>=4 && y<=20 && x>=420 && x<=480){ currentView = VIEW_MAIN; return; }
}
