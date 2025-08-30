#include <ArduinoJson.h>
#include <SD.h>
#include "views.h"
extern View currentView;
extern Arduino_GFX *gfx;

extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);

static String songSeq[64]; static int songLen=0; static bool songLoop=true;

static int nextSongNum(){
  if (!SD.exists("/songs")) SD.mkdir("/songs");
  File dir=SD.open("/songs"); if (!dir) return 1;
  int maxN=0; while(true){ File f=dir.openNextFile(); if(!f) break;
    String nm=f.name(); if (nm.startsWith("/songs/song_") && nm.endsWith(".json")){
      int n=nm.substring(12,nm.length()-5).toInt(); if (n>maxN) maxN=n;
    } f.close();
  } return maxN+1;
}

static void song_save_json(){
  if (!SD.exists("/songs")) SD.mkdir("/songs");
  String tmp="/songs/.tmp.json", fin="/songs/song_"+String(nextSongNum())+".json";
  File f=SD.open(tmp, FILE_WRITE); if (!f) return;
  DynamicJsonDocument doc(8192);
  JsonArray seq = doc.createNestedArray("sequence");
  for (int i=0;i<songLen;i++) seq.add(songSeq[i]);
  doc["loop"]=songLoop;
  serializeJson(doc,f); f.close(); SD.rename(tmp, fin);
}

static bool song_load_first(){
  File dir=SD.open("/songs"); if (!dir) return false;
  File f=dir.openNextFile(); if (!f) return false;
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc,f)) { f.close(); return false; }
  JsonArray seq = doc["sequence"]; songLen=min(64,(int)seq.size());
  for (int i=0;i<songLen;i++) songSeq[i]=seq[i].as<String>();
  songLoop = doc["loop"] | true; f.close(); return true;
}

void openSongView(){
  gfx->fillScreen(BLACK);
  drawTopBar("SONG", true);
  drawButtonBox(40,70,180,80, DARKGREY, "Save");
  drawButtonBox(240,70,180,80, DARKGREY, "Load");
  drawButtonBox(440,70,180,80, DARKGREY, "Clear");
}

void handleTouchSong(int x,int y){
  if (y>=70 && y<=150){
    if (x>=40 && x<=220){ song_save_json(); return; }
    if (x>=240 && x<=420){ song_load_first(); return; }
    // CORRIGÉ - coordonnées réalistes pour écran 480px
    if (x>=440 && x<=480){ /* clear song */ songLen=0; return; }
  }
  // CORRIGÉ - coordonnées du bouton Back plus larges
  if (y>=4 && y<=20 && x>=420 && x<=480){ currentView = VIEW_MAIN; return; }
}
