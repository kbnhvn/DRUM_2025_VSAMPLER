#include <ArduinoJson.h>
#include <SD.h>
#include "views.h"

extern Arduino_GFX *gfx;
extern void drawModernButton(int x, int y, int w, int h, int color, const char* text, bool active, bool pressed);
extern void flashButton(int x, int y, int w, int h, int color, const char* texto);
extern void forceCompleteRedraw();
extern void drawProgressBar(int x, int y, int w, int h, int percent, int color);
extern uint16_t pattern[16];
extern int32_t  ROTvalue[16][8];
extern String   sound_names[];
extern View     currentView;
extern int      bpm;

// Variables pour liste des patterns
static String patternList[32];
static int patternCount = 0;
static int selectedPatternFile = -1;

// Scanner les patterns existants
static void scanPatterns() {
  patternCount = 0;
  if (!SD.exists("/patterns")) return;
  
  File dir = SD.open("/patterns");
  if (!dir) return;
  
  while (true) {
    File f = dir.openNextFile();
    if (!f) break;
    
    String name = f.name();
    if (name.startsWith("pattern_") && name.endsWith(".json")) {
      patternList[patternCount] = name;
      patternCount++;
      if (patternCount >= 32) break;
    }
    f.close();
  }
  dir.close();
  
  Serial.printf("[PATTERN] Found %d pattern files\n", patternCount);
}

// Affichage moderne avec preview du pattern actuel
static void drawPatternPreview() {
  gfx->fillScreen(RGB565(15, 15, 20));
  // Zone preview moderne
  gfx->fillRect(300, 70, 170, 120, RGB565(25, 30, 40));
  gfx->drawRect(300, 70, 170, 120, UI_ON_SURFACE);
  
  gfx->setTextColor(UI_ACCENT, RGB565(25, 30, 40));
  gfx->setCursor(310, 85);
  gfx->print("CURRENT PATTERN");
  
  // Grille 4x4 pour les 16 steps
  for (int step = 0; step < 16; step++) {
    int px = 310 + (step % 4) * 35;
    int py = 95 + (step / 4) * 20;
    
    // Compter combien de pads jouent sur ce step
    int activeCount = 0;
    for (int pad = 0; pad < 16; pad++) {
      if (bitRead(pattern[pad], step)) activeCount++;
    }
    
    // Couleur selon densité
    int stepColor = RGB565(40, 45, 55);
    if (activeCount > 0) {
      stepColor = map(activeCount, 1, 16, RGB565(80, 120, 80), RGB565(150, 255, 150));
    }
    
    gfx->fillRect(px, py, 30, 15, stepColor);
    gfx->drawRect(px, py, 30, 15, UI_ON_SURFACE);
    
    // Numéro du step
    gfx->setTextColor(activeCount > 0 ? BLACK : UI_ON_SURFACE, stepColor);
    gfx->setCursor(px + 12, py + 10);
    gfx->print(step + 1);
  }
}

static int nextPatternNum(){
  if (!SD.exists("/patterns")) SD.mkdir("/patterns");
  File dir=SD.open("/patterns"); if (!dir) return 1;
  int maxN=0; while(true){ File f=dir.openNextFile(); if(!f) break;
    String nm=f.name(); if (nm.startsWith("pattern_") && nm.endsWith(".json")){
      // Extraction correcte du numéro
      int startIdx = nm.lastIndexOf("_") + 1;
      int endIdx = nm.lastIndexOf(".");
      int n = nm.substring(startIdx, endIdx).toInt(); 
      if (n > maxN) maxN = n;
    } f.close();
  } 
  dir.close();
  return maxN+1;
}

// CORRECTION: Une seule fonction pattern_save_json
static void pattern_save_json(){
  if (!SD.exists("/patterns")) SD.mkdir("/patterns");
  
  int patNum = nextPatternNum();
  String tmp = "/patterns/.tmp.json";
  String fin = "/patterns/pattern_" + String(patNum) + ".json";
  
  File f = SD.open(tmp, FILE_WRITE); 
  if (!f) {
    Serial.println("[PATTERN] Save failed - cannot create file");
    return;
  }

  DynamicJsonDocument doc(16384);
  
  // Métadonnées étendues
  doc["version"] = 2;
  doc["created"] = millis(); // Timestamp relatif
  doc["bpm"] = bpm;
  // doc["firstStep"] = firstStep; // Si disponible
  // doc["lastStep"] = lastStep;   // Si disponible
  
  // Pattern complet avec noms de samples
  auto pads = doc.createNestedArray("pads");
  for(int s = 0; s < 16; s++){
    auto po = pads.createNestedObject();
    po["id"] = s;
    po["sample"] = sound_names[s];
    
    // Paramètres audio inline
    auto pa = po.createNestedObject("params");
    pa["SAM"] = ROTvalue[s][0]; pa["INI"] = ROTvalue[s][1];
    pa["END"] = ROTvalue[s][2]; pa["PIT"] = ROTvalue[s][3];
    pa["RVS"] = ROTvalue[s][4]; pa["VOL"] = ROTvalue[s][5];
    pa["PAN"] = ROTvalue[s][6]; pa["FIL"] = ROTvalue[s][7];
    
    // Steps avec format compact
    auto steps = po.createNestedArray("steps");
    for(int st = 0; st < 16; st++) {
      steps.add((pattern[s] >> st) & 0x1);
    }
  }
  
  if (serializeJson(doc, f) == 0) {
    Serial.println("[PATTERN] JSON serialization failed");
    f.close();
    SD.remove(tmp);
    return;
  }
  
  f.close(); 
  
  if (!SD.rename(tmp, fin)) {
    Serial.println("[PATTERN] Rename failed");
    SD.remove(tmp);
    return;
  }
  
  Serial.printf("[PATTERN] Saved successfully as %s\n", fin.c_str());
}

// CORRECTION: Une seule fonction pattern_new
static void pattern_new() { 
  // Animation de clear
  for (int s = 0; s < 16; s++) {
    pattern[s] = 0;
    // Petit délai pour effet visuel
    if (s % 4 == 0) delay(50);
  }
  
  Serial.println("[PATTERN] New pattern created");
}

static bool pattern_load_first(){
  File dir=SD.open("/patterns"); 
  if (!dir) return false;
  
  File f=dir.openNextFile(); 
  if (!f) {
    dir.close();
    return false;
  }
  
  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc,f)) { 
    f.close(); 
    dir.close();
    return false; 
  }
  
  auto pads = doc["pads"].as<ArduinoJson::JsonArray>();
  for(int s=0;s<min(16,(int)pads.size());s++){
    auto po=pads[s].as<ArduinoJson::JsonObject>();
    sound_names[s]=po["sample"].as<String>();
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
    } else if (po.containsKey("steps")) {
      // Support nouveau format
      auto steps=po["steps"].as<ArduinoJson::JsonArray>();
      int n = min(16, (int)steps.size());
      for (int st=0; st<n; ++st) if (steps[st].as<int>()) m |= (1<<st);
    }
    pattern[s]=m;
  }
  f.close(); 
  dir.close();
  return true;
}

void openPatternView(){
  gfx->fillScreen(RGB565(15, 15, 20));
  
  // Header avec gradient
  for (int y = 0; y < 35; y++) {
    int shade = map(y, 0, 35, 80, 25);
    gfx->drawLine(0, y, 480, y, RGB565(shade, shade + 15, shade + 25));
  }
  
  gfx->setTextColor(UI_ACCENT, RGB565(60, 75, 95));
  gfx->setCursor(20, 25);
  gfx->print("PATTERN MANAGER");
  
  // Scan des patterns existants
  scanPatterns();
  
  // Boutons principaux avec design moderne
  drawModernButton( 20, 70, 120, 60, UI_SUCCESS, "SAVE\nPATTERN", true, false);
  drawModernButton( 20, 140, 120, 40, UI_WARNING, "NEW", true, false);
  drawModernButton(160, 70, 120, 110, UI_PRIMARY, "LOAD\nPATTERN", patternCount > 0, false);
  
  // Preview du pattern actuel
  drawPatternPreview();
  
  // Liste des patterns existants
  if (patternCount > 0) {
    gfx->setTextColor(UI_ON_SURFACE, RGB565(15, 15, 20));
    gfx->setCursor(20, 200);
    gfx->printf("Available patterns: %d", patternCount);
    
    // Afficher quelques noms de patterns
    for (int i = 0; i < min(3, patternCount); i++) {
      gfx->setCursor(20, 215 + i * 12);
      gfx->setTextColor(RGB565(120, 125, 135), RGB565(15, 15, 20));
      String shortName = patternList[i];
      if (shortName.length() > 25) {
        shortName = shortName.substring(0, 22) + "...";
      }
      gfx->print(shortName);
    }
    
    if (patternCount > 3) {
      gfx->setCursor(20, 251);
      gfx->setTextColor(UI_ACCENT, RGB565(15, 15, 20));
      gfx->printf("... and %d more", patternCount - 3);
    }
  } else {
    gfx->setTextColor(RGB565(100, 105, 115), RGB565(15, 15, 20));
    gfx->setCursor(20, 200);
    gfx->print("No saved patterns found");
  }
  
  // Back button moderne
  drawModernButton(400, 5, 70, 25, UI_DANGER, "BACK", true, false);
}

void handleTouchPattern(int x,int y){
  Serial.printf("[PATTERN] Touch at x=%d, y=%d\n", x, y);
  
  // Boutons avec animations et feedback amélioré
  // Save Pattern
  if (y >= 70 && y <= 130 && x >= 20 && x <= 140) {
      drawModernButton(20, 70, 120, 60, UI_SUCCESS, "SAVE\nPATTERN", true, true);
      delay(100);
      Serial.println("[PATTERN] Saving pattern...");
      pattern_save_json(); 
      
      // Animation de confirmation
      gfx->fillRect(20, 190, 200, 30, UI_SUCCESS);
      gfx->setTextColor(BLACK, UI_SUCCESS);
      gfx->setCursor(30, 210);
      gfx->printf("Pattern saved as #%d!", nextPatternNum() - 1);
      delay(1500);
      openPatternView();
      return; 
  }
  
  // New Pattern
  if (y >= 140 && y <= 180 && x >= 20 && x <= 140) {
      drawModernButton(20, 140, 120, 40, UI_WARNING, "NEW", true, true);
      delay(100);
      Serial.println("[PATTERN] Creating new pattern...");
      pattern_new(); 
      
      // Animation de clear avec effet visuel
      for (int i = 0; i < 16; i++) {
        gfx->fillRect(310 + (i % 4) * 35, 95 + (i / 4) * 20, 30, 15, UI_WARNING);
        delay(30);
      }
      
      gfx->fillRect(160, 190, 200, 30, UI_WARNING);
      gfx->setTextColor(BLACK, UI_WARNING);
      gfx->setCursor(170, 210);
      gfx->print("Pattern cleared!");
      delay(1200);
      openPatternView();
      return; 
  }
  
  // Load Pattern
  if (y >= 70 && y <= 180 && x >= 160 && x <= 280) {
      drawModernButton(160, 70, 120, 110, UI_PRIMARY, "LOAD\nPATTERN", patternCount > 0, true);
      delay(100);
   
      Serial.println("[PATTERN] Loading pattern...");
      bool loaded = pattern_load_first(); 
      
      // Feedback avec couleur selon résultat
      int feedbackColor = loaded ? UI_SUCCESS : UI_DANGER;
      gfx->fillRect(160, 190, 200, 30, feedbackColor);
      gfx->setTextColor(BLACK, feedbackColor);
      gfx->setCursor(170, 210);
      gfx->print(loaded ? "Pattern loaded!" : "Load failed!");
      delay(1200);
      
      openPatternView();
      return; 
  }
  
  // Back avec transition moderne
  if (y >= 5 && y <= 30 && x >= 400 && x <= 470) {  
    Serial.println("[PATTERN] BACK to main");
    
    drawModernButton(400, 5, 70, 25, UI_DANGER, "BACK", true, true);
    
    // Effet wipe de gauche à droite
    for (int x = 0; x < 480; x += 40) {
      gfx->fillRect(x, 0, 40, 272, RGB565(x/12, x/12, x/12));
      delay(15);
    }
   
    currentView = VIEW_MAIN;
    // CORRECTION: S'assurer que l'UI principale reprend proprement
    pauseMainUIRefresh();  // Reset état
    forceCompleteRedraw();
    resumeMainUIRefresh(); // Relancer
    return; 
  }
  
  Serial.printf("[PATTERN] Touch ignored at x=%d, y=%d\n", x, y);
}