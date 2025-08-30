// files_tools.ino — SD only, JSON patterns & sound-sets
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>

// ==== Externs (doivent correspondre EXACTEMENT aux signatures/globals du projet) ====
extern uint16_t pattern[16];          // 16 bitmasks, 1 par pad
extern uint16_t isMelodic;            // bitmask 16 bits
extern int32_t  ROTvalue[16][8];      // SAM,INI,END,PIT,RVS,VOL,PAN,FIL
extern int      bpm;
extern int      master_vol;
extern int      master_filter;
extern int      octave;

extern byte     firstStep;
extern byte     lastStep;
extern byte     newLastStep;

extern void     setSound(byte voice);  // re-applique params vers moteur
extern void     select_rot();          // rafraichit l'UI barres si besoin

// ==== Utils ====
static void ensureDir(const char* path) {
  if (!SD.exists(path)) SD.mkdir(path);
}

static String twoDigits(byte n) {
  if (n < 10) return "0" + String(n);
  return String(n);
}

// ==== PATTERN ====
void save_pattern(byte idx) {
  ensureDir("/patterns");
  String path = "/patterns/pattern_" + twoDigits(idx) + ".json";

  // Code inline comme dans song_view.ino
  String tmp = "/patterns/.tmp.json";
  File f = SD.open(tmp, FILE_WRITE); 
  if (!f) return;
  
  DynamicJsonDocument doc(4096);
  doc["version"]   = 1;
  doc["firstStep"] = (int)firstStep;
  doc["lastStep"]  = (int)lastStep;
  doc["isMelodic"] = (int)isMelodic;

  auto steps = doc.createNestedArray("steps");
  for (int s = 0; s < 16; s++) steps.add((uint16_t)pattern[s]);

  serializeJson(doc, f); 
  f.close(); 
  SD.rename(tmp, path);
  
  Serial.println("[PATTERN] Saved -> " + path);
}

void load_pattern(byte idx) {
  String path = "/patterns/pattern_" + twoDigits(idx) + ".json";
  
  // Code inline comme dans song_view.ino  
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.println("[PATTERN] Load FAILED -> " + path);
    return;
  }
  
  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, f)) { 
    f.close(); 
    Serial.println("[PATTERN] Parse FAILED -> " + path);
    return; 
  }

  if (doc.containsKey("firstStep")) firstStep = (byte)((int)doc["firstStep"]);
  if (doc.containsKey("lastStep"))  lastStep  = (byte)((int)doc["lastStep"]);
  if (doc.containsKey("isMelodic")) isMelodic = (uint16_t)((int)doc["isMelodic"]);

  auto steps = doc["steps"];
  if (!steps.isNull() && steps.size() >= 16) {
    for (int s = 0; s < 16; s++) {
      pattern[s] = (uint16_t)steps[s].as<uint16_t>();
    }
  }
  
  f.close();
  Serial.println("[PATTERN] Loaded <- " + path);
}

// ==== SOUND-SET ====
void save_sound(byte idx) {
  ensureDir("/sounds");
  String path = "/sounds/sound_" + twoDigits(idx) + ".json";

  // Code inline 
  String tmp = "/sounds/.tmp.json";
  File f = SD.open(tmp, FILE_WRITE); 
  if (!f) return;
  
  DynamicJsonDocument doc(8192);
  doc["version"] = 1;

  auto gl = doc.createNestedObject("globals");
  gl["bpm"]          = bpm;
  gl["master_vol"]   = master_vol;
  gl["master_filter"]= master_filter;
  gl["octave"]       = octave;

  auto voices = doc.createNestedArray("voices");
  for (int v = 0; v < 16; v++) {
    auto vo = voices.createNestedObject();
    auto pa = vo.createNestedArray("params");
    for (int p = 0; p < 8; p++) pa.add((int)ROTvalue[v][p]);
  }

  serializeJson(doc, f); 
  f.close(); 
  SD.rename(tmp, path);
  
  Serial.println("[SOUND] Saved -> " + path);
}

void load_sound(byte idx) {
  String path = "/sounds/sound_" + twoDigits(idx) + ".json";
  
  // Code inline
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.println("[SOUND] Load FAILED -> " + path);
    return;
  }
  
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, f)) { 
    f.close(); 
    Serial.println("[SOUND] Parse FAILED -> " + path);
    return; 
  }

  auto gl = doc["globals"];
  if (!gl.isNull()) {
    if (gl.containsKey("bpm"))           bpm = (int)gl["bpm"];
    if (gl.containsKey("master_vol"))    master_vol = (int)gl["master_vol"];
    if (gl.containsKey("master_filter")) master_filter = (int)gl["master_filter"];
    if (gl.containsKey("octave"))        octave = (int)gl["octave"];
  }

  auto voices = doc["voices"];
  if (!voices.isNull() && voices.size() >= 16) {
    for (int v = 0; v < 16; v++) {
      auto vo = voices[v];
      auto pa = vo["params"];
      if (!pa.isNull() && pa.size() >= 8) {
        for (int p = 0; p < 8; p++) {
          ROTvalue[v][p] = (int32_t)pa[p].as<int>();
        }
      }
    }
  }

  // Réapplique les params à l'audio
  for (byte v = 0; v < 16; v++) setSound(v);
  select_rot();
  
  f.close();
  Serial.println("[SOUND] Loaded <- " + path);
}