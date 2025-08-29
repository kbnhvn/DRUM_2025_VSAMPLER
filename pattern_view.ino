#include <ArduinoJson.h>
#include <SD.h>
#include "views.h"
class Arduino_GFX; extern Arduino_GFX *gfx;

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

template<typename T>
static void jsonWriteParams(T obj, int s){
  obj["SAM"]=ROTvalue[s][0];
  obj["INI"]=ROTvalue[s][1];
  obj["END"]=ROTvalue[s][2];
  obj["PIT"]=ROTvalue[s][3];
  obj["RVS"]=ROTvalue[s][4];
  obj["VOL"]=ROTvalue[s][5];
  obj["PAN"]=ROTvalue[s][6];
  obj["FIL"]=ROTvalue[s][7];
}
template<typename T>
static void jsonReadParams(T obj, int s){
  if(obj.containsKey("SAM")) ROTvalue[s][0]=(int32_t)obj["SAM"];
  if(obj.containsKey("INI")) ROTvalue[s][1]=(int32_t)obj["INI"];
  if(obj.containsKey("END")) ROTvalue[s][2]=(int32_t)obj["END"];
  if(obj.containsKey("PIT")) ROTvalue[s][3]=(int32_t)obj["PIT"];
  if(obj.containsKey("RVS")) ROTvalue[s][4]=(int32_t)obj["RVS"];
  if(obj.containsKey("VOL")) ROTvalue[s][5]=(int32_t)obj["VOL"];
  if(obj.containsKey("PAN")) ROTvalue[s][6]=(int32_t)obj["PAN"];
  if(obj.containsKey("FIL")) ROTvalue[s][7]=(int32_t)obj["FIL"];
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
    auto pa = po.createNestedObject("params");
    jsonWriteParams(pa, s);
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
    auto pa = po["params"]; jsonReadParams(pa, s);
    auto steps=po["pattern"].as<ArduinoJson::JsonArray>();
    uint16_t m = 0;
    int n = min(16, (int)steps.size());
    for (int st=0; st<n; ++st) if (steps[st].as<int>()) m |= (1<<st);
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
    if (x>=440 && x<=620){ pattern_load_first(); return; }
  }
  // Back
  if (y>=4 && y<=20 && x>=440 && x<=472){ currentView = VIEW_MAIN; return; }
}
