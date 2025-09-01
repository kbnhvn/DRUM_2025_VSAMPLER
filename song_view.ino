#include <ArduinoJson.h>
#include <SD.h>
#include "views.h"

extern Arduino_GFX *gfx;
extern View currentView;
extern void drawModernButton(int x, int y, int w, int h, int color, const char* text, bool active, bool pressed);
extern void flashButton(int x, int y, int w, int h, int color, const char* texto);
extern void forceCompleteRedraw();
extern void drawProgressBar(int x, int y, int w, int h, int percent, int color);

// NOUVEAU: Variables étendues pour song management
static String songSeq[64];
static String songNames[16];    // Noms des songs sauvées
static int songFilesCount = 0;  // Nombre de songs sur SD
static int songLen=0; 
static bool songLoop=true;

static int selectedSongFile = -1;
static int songPlayPosition = -1; // Position de lecture actuelle

// NOUVEAU: Scanner les songs existantes
static void scanSongs() {
  songFilesCount = 0;
  if (!SD.exists("/songs")) return;
  
  File dir = SD.open("/songs");
  if (!dir) return;
  
  while (true) {
    File f = dir.openNextFile();
    if (!f) break;
    
    String name = f.name();
    if (name.startsWith("song_") && name.endsWith(".json")) {
      songNames[songFilesCount] = name;
      songFilesCount++;
      if (songFilesCount >= 16) break;
    }
    f.close();
  }
  dir.close();
  
  Serial.printf("[SONG] Found %d song files\n", songFilesCount);
}

// NOUVEAU: Visualisation de la séquence de patterns
static void drawSongSequence() {
  gfx->fillRect(280, 70, 190, 150, RGB565(25, 30, 40));
  gfx->drawRect(280, 70, 190, 150, UI_ON_SURFACE);
  
  gfx->setTextColor(UI_ACCENT, RGB565(25, 30, 40));
  gfx->setCursor(290, 85);
  gfx->print("SONG SEQUENCE");
  
  if (songLen == 0) {
    gfx->setTextColor(RGB565(100, 105, 115), RGB565(25, 30, 40));
    gfx->setCursor(290, 130);
    gfx->print("Empty sequence");
    gfx->setCursor(290, 145);
    gfx->print("Add patterns to");
    gfx->setCursor(290, 160);
    gfx->print("create a song");
    return;
  }
  
  // Affichage des patterns dans la séquence
  int maxVisible = 8;
  int startIdx = max(0, songPlayPosition - 3);
  
  for (int i = 0; i < min(maxVisible, songLen); i++) {
    int seqIdx = startIdx + i;
    if (seqIdx >= songLen) break;
    
    int py = 100 + i * 16;
    bool isCurrent = (seqIdx == songPlayPosition);
    bool isNext = (seqIdx == songPlayPosition + 1);
    
    // Couleur selon état
    int bgColor = RGB565(35, 40, 50);
    int textColor = UI_ON_SURFACE;
    
    if (isCurrent) {
      bgColor = UI_ACCENT;
      textColor = BLACK;
    } else if (isNext) {
      bgColor = UI_WARNING;
      textColor = BLACK;
    }
    
    gfx->fillRect(290, py, 170, 14, bgColor);
    
    // Numéro de pattern
    gfx->setTextColor(textColor, bgColor);
    gfx->setCursor(295, py + 10);
    gfx->printf("%02d:", seqIdx + 1);
    
    // Nom/numéro du pattern
    gfx->setCursor(320, py + 10);
    String patName = songSeq[seqIdx];
    if (patName.length() > 15) {
      patName = patName.substring(0, 12) + "...";
    }
    gfx->print(patName);
    
    // Indicateur loop
    if (songLoop && seqIdx == songLen - 1) {
      gfx->setCursor(430, py + 10);
      gfx->print("↻");
    }
  }
  
  // Progress bar de la song
  if (songPlayPosition >= 0 && songLen > 0) {
    int progress = (songPlayPosition * 100) / songLen;
    drawProgressBar(290, 205, 170, 8, progress, UI_ACCENT);
  }
}

static int nextSongNum(){
  if (!SD.exists("/songs")) SD.mkdir("/songs");
  File dir=SD.open("/songs"); if (!dir) return 1;
  int maxN=0; while(true){ File f=dir.openNextFile(); if(!f) break;
    String nm=f.name(); if (nm.startsWith("/songs/song_") && nm.endsWith(".json")){
      // CORRECTION: Extraction correcte du numéro
      int startIdx = nm.lastIndexOf("_") + 1;
      int endIdx = nm.lastIndexOf(".");
      int n = nm.substring(startIdx, endIdx).toInt();
      if (n > maxN) maxN = n;    }; 
      f.close();
  } return maxN+1;
}

// NOUVEAU: Sauvegarde avec métadonnées
static void song_save_json() {
  if (!SD.exists("/songs")) SD.mkdir("/songs");
  
  int songNum = nextSongNum();
  String tmp = "/songs/.tmp.json";
  String fin = "/songs/song_" + String(songNum) + ".json";
  
  File f = SD.open(tmp, FILE_WRITE); 
  if (!f) {
    Serial.println("[SONG] Save failed - cannot create file");
    return;
  }
 
  DynamicJsonDocument doc(8192);

  // Métadonnées song
  doc["version"] = 2;
  doc["created"] = millis();
  doc["bpm"] = bpm;
  doc["length"] = songLen;
  doc["loop"] = songLoop;

  JsonArray seq = doc.createNestedArray("sequence");
  for (int i = 0; i < songLen; i++) {
    seq.add(songSeq[i]);
  }
  
  if (serializeJson(doc, f) == 0) {
    Serial.println("[SONG] JSON serialization failed");
    f.close();
    SD.remove(tmp);
    return;
  }
  
  f.close();
  
  if (!SD.rename(tmp, fin)) {
    Serial.println("[SONG] Rename failed");
    SD.remove(tmp);
    return;
  }
  
  Serial.printf("[SONG] Saved as %s with %d patterns\n", fin.c_str(), songLen);
}

static bool song_load_first(){
  File dir=SD.open("/songs"); if (!dir) return false;
  File f=dir.openNextFile(); if (!f) return false;
  
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, f)) { 
    f.close(); 
    Serial.println("[SONG] JSON parse failed");
    return false; 
  }
  
  // Charger métadonnées si disponibles
  if (doc.containsKey("bpm")) {
    bpm = doc["bpm"];
    Serial.printf("[SONG] Loaded BPM: %d\n", bpm);
  }
  
  JsonArray seq = doc["sequence"]; 
  songLen = min(64, (int)seq.size());
  for (int i = 0; i < songLen; i++) {
    songSeq[i] = seq[i].as<String>();
  }
  
  songLoop = doc["loop"] | true;
  songPlayPosition = -1; // Reset position
  
  f.close(); 
  Serial.printf("[SONG] Loaded with %d patterns, loop=%s\n", 
                songLen, songLoop ? "ON" : "OFF");
  return true;
}

// NOUVEAU: Interface moderne complète
void openSongView() {
  gfx->fillScreen(RGB565(15, 15, 20));
  
  // Header avec gradient
  for (int y = 0; y < 35; y++) {
    int shade = map(y, 0, 35, 80, 25);
    gfx->drawLine(0, y, 480, y, RGB565(shade + 10, shade + 20, shade + 30));
  }
  
  gfx->setTextColor(UI_ACCENT, RGB565(60, 80, 110));
  gfx->setCursor(20, 25);
  gfx->print("SONG MANAGER");
  
  // Scan des songs existantes
  scanSongs();
  
  // Boutons principaux
  drawModernButton( 20, 70, 120, 50, UI_SUCCESS, "SAVE\nSONG", true, false);
  drawModernButton( 20, 130, 120, 40, UI_PRIMARY, "LOAD", songFilesCount > 0, false);
  drawModernButton( 20, 180, 120, 40, UI_WARNING, "CLEAR", songLen > 0, false);
  
  // Toggle loop avec design moderne
  int toggleX = 160, toggleY = 130;
  gfx->fillRect(toggleX, toggleY, 100, 30, songLoop ? UI_SUCCESS : UI_SURFACE);
  gfx->drawRect(toggleX, toggleY, 100, 30, songLoop ? UI_SUCCESS : UI_ON_SURFACE);
  
  gfx->setTextColor(songLoop ? BLACK : UI_ON_SURFACE, songLoop ? UI_SUCCESS : UI_SURFACE);
  gfx->setCursor(toggleX + 20, toggleY + 20);
  gfx->print(songLoop ? "LOOP ON" : "LOOP OFF");
  
  // Visualisation de la séquence
  drawSongSequence();
  
  // Info en bas
  gfx->setTextColor(UI_ON_SURFACE, RGB565(15, 15, 20));
  gfx->setCursor(20, 240);
  gfx->printf("Sequence: %d patterns | Files: %d | BPM: %d", 
              songLen, songFilesCount, bpm);
  
  // Back button
  drawModernButton(400, 5, 70, 25, UI_DANGER, "BACK", true, false);
}

void handleTouchSong(int x,int y){
  Serial.printf("[SONG] Touch at x=%d, y=%d\n", x, y);
  
  // NOUVEAU: Boutons avec animations et feedback
  // Save Song
  if (y >= 70 && y <= 120 && x >= 20 && x <= 140) {
      drawModernButton(20, 70, 120, 50, UI_SUCCESS, "SAVE\nSONG", true, true);
      delay(100);
   
      song_save_json(); 
      
      // Animation de sauvegarde
      gfx->fillRect(20, 190, 200, 25, UI_SUCCESS);
      gfx->setTextColor(BLACK, UI_SUCCESS);
      gfx->setCursor(30, 205);
      gfx->printf("Song saved as #%d!", nextSongNum() - 1);
      delay(1500);
 
      openSongView();
      return; 
  }
  
  // Load Song
  if (y >= 130 && y <= 170 && x >= 20 && x <= 140 && songFilesCount > 0) {
      drawModernButton(20, 130, 120, 40, UI_PRIMARY, "LOAD", true, true);
      delay(100);
      
      bool loaded = song_load_first(); 
      
      // Feedback visuel selon résultat
      int feedbackColor = loaded ? UI_SUCCESS : UI_DANGER;
      gfx->fillRect(20, 190, 200, 25, feedbackColor);
      gfx->setTextColor(BLACK, feedbackColor);
      gfx->setCursor(30, 205);
      gfx->print(loaded ? "Song loaded!" : "Load failed!");
      delay(1200);
      openSongView();
      return; 
  }
  
  // Clear Song
  if (y >= 180 && y <= 220 && x >= 20 && x <= 140 && songLen > 0) {
      drawModernButton(20, 180, 120, 40, UI_WARNING, "CLEAR", true, true);
      delay(100);
      
      // Animation de clear
      songLen=0; 
      
      songPlayPosition = -1;
      
      // Effet visuel de vidage
      for (int i = 0; i < 8; i++) {
        gfx->fillRect(290, 100 + i * 16, 170, 14, UI_WARNING);
        delay(40);
      }
      gfx->fillRect(20, 190, 200, 25, UI_WARNING);
      gfx->setTextColor(BLACK, UI_WARNING);
      gfx->setCursor(30, 205);
      gfx->print("Song sequence cleared!");
      delay(1200);
      openSongView();
      return; 
  }
  
  // NOUVEAU: Toggle loop
  if (y >= 130 && y <= 160 && x >= 160 && x <= 260) {
    songLoop = !songLoop;
    
    // Animation toggle
    for (int step = 0; step < 5; step++) {
      gfx->fillRect(160, 130, 100, 30, songLoop ? UI_SUCCESS : UI_SURFACE);
      delay(30);
    }
    
    Serial.printf("[SONG] Loop toggled: %s\n", songLoop ? "ON" : "OFF");
    openSongView();
    return;
  }
  
  // NOUVEAU: Clic sur séquence pour navigation
  if (y >= 100 && y <= 200 && x >= 290 && x <= 460 && songLen > 0) {
    int clickedRow = (y - 100) / 16;
    int newPos = max(0, songPlayPosition - 3) + clickedRow;
    
    if (newPos >= 0 && newPos < songLen) {
      songPlayPosition = newPos;
      Serial.printf("[SONG] Jump to position %d\n", newPos);
      drawSongSequence();
      return;
    }
  }
  
  // Back avec transition
  if (y >= 5 && y <= 30 && x >= 400 && x <= 470) { 
    Serial.println("[SONG] BACK to main");
    
    drawModernButton(400, 5, 70, 25, UI_DANGER, "BACK", true, true);
    
    // Transition en spirale
    int centerX = 240, centerY = 136;
    for (int radius = 0; radius < 300; radius += 20) {
      gfx->drawCircle(centerX, centerY, radius, RGB565(radius/8, radius/8, radius/8));
      delay(20);
    }
   
    currentView = VIEW_MAIN;
    forceCompleteRedraw();
    return; 
  }
}