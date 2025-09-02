// files_tools.ino — SD only, JSON patterns & sound-sets
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include "esp_task_wdt.h"

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
// ==== Utils améliorés ====
static bool ensureDir(const char* path) {
  if (!SD.exists(path)) {
    bool ok = SD.mkdir(path);
    if (ok) {
      Serial.printf("[FILES] Created directory: %s\n", path);
    } else {
      Serial.printf("[FILES] Failed to create directory: %s\n", path);
    }
    return ok;
  }
  return true;
}

static String twoDigits(byte n) {
  if (n < 10) return "0" + String(n);
  return String(n);
}

// NOUVEAU: Validation de fichier JSON
static bool validateJsonFile(const String& path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  
  DynamicJsonDocument testDoc(1024);
  bool valid = (deserializeJson(testDoc, f) == DeserializationError::Ok);
  f.close();
  
  return valid;
}

// ==== PATTERN ====
void save_pattern(byte idx) {
  if (!ensureDir("/patterns")) return;
 
  String path = "/patterns/pattern_" + twoDigits(idx) + ".json";

  // NOUVEAU: Sauvegarde atomique avec backup
  String tmp = "/patterns/.tmp.json";
  String backup = "/patterns/pattern_" + twoDigits(idx) + ".bak";
  
  // Backup de l'ancien fichier si existe
  if (SD.exists(path)) {
    SD.remove(backup); // Supprimer ancien backup
    SD.rename(path, backup);
  }
 
  File f = SD.open(tmp, FILE_WRITE); 
  if (!f) {
    Serial.printf("[PATTERN] Cannot create temp file for pattern %d\n", idx);
    return;
  }
  
  DynamicJsonDocument doc(6144);
  doc["version"]   = 2;
  doc["created"]   = millis();
  doc["bpm"]       = bpm;
  doc["firstStep"] = (int)firstStep;
  doc["lastStep"]  = (int)lastStep;
  doc["isMelodic"] = (int)isMelodic;

  // NOUVEAU: Métadonnées étendues
  auto meta = doc.createNestedObject("metadata");
  meta["total_steps"] = 16;
  meta["active_steps"] = lastStep - firstStep + 1;
  
  // Compter density
  int totalNotes = 0;
  for (int s = 0; s < 16; s++) {
    for (int step = 0; step < 16; step++) {
      if (bitRead(pattern[s], step)) totalNotes++;
    }
  }
  meta["note_density"] = totalNotes;

  auto steps = doc.createNestedArray("steps");
  for (int s = 0; s < 16; s++) {
    steps.add((uint16_t)pattern[s]);
  }

  // NOUVEAU: Vérification sérialisation
  if (serializeJson(doc, f) == 0) {
    Serial.printf("[PATTERN] JSON serialization failed for pattern %d\n", idx);
    f.close();
    SD.remove(tmp);
    if (SD.exists(backup)) SD.rename(backup, path); // Restaurer backup
    return;
  }
 
  f.close(); 
  // Renommer atomiquement
  if (!SD.rename(tmp, path)) {
    Serial.printf("[PATTERN] Rename failed for pattern %d\n", idx);
    SD.remove(tmp);
    if (SD.exists(backup)) SD.rename(backup, path); // Restaurer backup
    return;
  }
  
  // Nettoyer backup en cas de succès
  SD.remove(backup);
  
  Serial.printf("[PATTERN] Saved pattern %d -> %s (%d notes)\n", idx, path.c_str(), totalNotes);
}

void load_pattern(byte idx) {
  String path = "/patterns/pattern_" + twoDigits(idx) + ".json";
  
  if (!SD.exists(path)) {
    Serial.printf("[PATTERN] File not found: %s\n", path.c_str());
    return;
  }
  
  // NOUVEAU: Validation avant chargement
  if (!validateJsonFile(path)) {
    Serial.printf("[PATTERN] Invalid JSON file: %s\n", path.c_str());
    return;
  }
 
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("[PATTERN] Cannot open: %s\n", path.c_str());
    return;
  }
  
  DynamicJsonDocument doc(6144);
  if (deserializeJson(doc, f)) { 
    f.close(); 
    Serial.printf("[PATTERN] Parse failed: %s\n", path.c_str());
    return; 
  }

  // NOUVEAU: Validation de version
  int version = doc["version"] | 1;
  if (version > 2) {
    Serial.printf("[PATTERN] Unsupported version %d in %s\n", version, path.c_str());
    f.close();
    return;
  }

  // Charger paramètres avec validation
  if (doc.containsKey("firstStep")) firstStep = (byte)((int)doc["firstStep"]);
  if (doc.containsKey("lastStep"))  lastStep  = (byte)((int)doc["lastStep"]);
  if (doc.containsKey("isMelodic")) isMelodic = (uint16_t)((int)doc["isMelodic"]);
  
  // NOUVEAU: Validation des bornes
  if (firstStep > 15) firstStep = 0;
  if (lastStep > 15) lastStep = 15;
  if (lastStep < firstStep) lastStep = firstStep;

  auto steps = doc["steps"];
  if (!steps.isNull() && steps.size() >= 16) {
    for (int s = 0; s < 16; s++) {
      pattern[s] = (uint16_t)steps[s].as<uint16_t>();
    }
  } else {
    Serial.printf("[PATTERN] Warning: incomplete steps data in %s\n", path.c_str());
  }
  // NOUVEAU: Charger BPM si disponible
  if (doc.containsKey("bpm")) {
    int newBpm = doc["bpm"];
    if (newBpm >= 60 && newBpm <= 400) {
      bpm = newBpm;
      Serial.printf("[PATTERN] Loaded BPM: %d\n", bpm);
    }
  }
 
  f.close();
  
  // Compter notes chargées
  int loadedNotes = 0;
  for (int s = 0; s < 16; s++) {
    for (int step = 0; step < 16; step++) {
      if (bitRead(pattern[s], step)) loadedNotes++;
    }
  }
  
  Serial.printf("[PATTERN] Loaded pattern %d <- %s (%d notes)\n", 
                idx, path.c_str(), loadedNotes);
}

// ==== SOUND-SET ====
void save_sound(byte idx) {
  if (!ensureDir("/sounds")) return;
 
  String path = "/sounds/sound_" + twoDigits(idx) + ".json";

  // NOUVEAU: Sauvegarde atomique avec backup
  String tmp = "/sounds/.tmp.json";
  String backup = "/sounds/sound_" + twoDigits(idx) + ".bak";
  
  // Backup si existe
  if (SD.exists(path)) {
    SD.remove(backup);
    SD.rename(path, backup);
  }
 
  File f = SD.open(tmp, FILE_WRITE); 
  if (!f) {
    Serial.printf("[SOUND] Cannot create temp file for sound %d\n", idx);
    return;
  }
  
  DynamicJsonDocument doc(12288);
  doc["version"] = 2;
  doc["created"] = millis();

  auto gl = doc.createNestedObject("globals");
  gl["bpm"]          = bpm;
  gl["master_vol"]   = master_vol;
  gl["master_filter"]= master_filter;
  gl["octave"]       = octave;
  
  // NOUVEAU: Métadonnées globales étendues
  gl["firstStep"]    = firstStep;
  gl["lastStep"]     = lastStep;
  gl["isMelodic"]    = isMelodic;

  auto voices = doc.createNestedArray("voices");
  for (int v = 0; v < 16; v++) {
    auto vo = voices.createNestedObject();
    vo["id"] = v;
    vo["sample_name"] = sound_names[v]; // NOUVEAU: Nom du sample
    auto pa = vo.createNestedArray("params");
    for (int p = 0; p < 8; p++) {
      pa.add((int)ROTvalue[v][p]);
    }
  }

  if (serializeJson(doc, f) == 0) {
    Serial.printf("[SOUND] JSON serialization failed for sound %d\n", idx);
    f.close();
    SD.remove(tmp);
    if (SD.exists(backup)) SD.rename(backup, path);
    return;
  }
 
  f.close(); 
  if (!SD.rename(tmp, path)) {
    Serial.printf("[SOUND] Rename failed for sound %d\n", idx);
    SD.remove(tmp);
    if (SD.exists(backup)) SD.rename(backup, path);
    return;
  }
  
  SD.remove(backup);
  Serial.printf("[SOUND] Saved sound %d -> %s\n", idx, path.c_str());
}

void load_sound(byte idx) {
  String path = "/sounds/sound_" + twoDigits(idx) + ".json";
  
  if (!SD.exists(path)) {
    Serial.printf("[SOUND] File not found: %s\n", path.c_str());
    return;
  }
  
  if (!validateJsonFile(path)) {
    Serial.printf("[SOUND] Invalid JSON file: %s\n", path.c_str());
    return;
  }
 
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("[SOUND] Cannot open: %s\n", path.c_str());
    return;
  }
  
  DynamicJsonDocument doc(12288);
  if (deserializeJson(doc, f)) { 
    f.close(); 
    Serial.printf("[SOUND] Parse failed: %s\n", path.c_str());
    return; 
  }

  // Validation version
  int version = doc["version"] | 1;
  if (version > 2) {
    Serial.printf("[SOUND] Unsupported version %d in %s\n", version, path.c_str());
    f.close();
    return;
  }


  auto gl = doc["globals"];
  if (!gl.isNull()) {
    // NOUVEAU: Validation des valeurs globales
    if (gl.containsKey("bpm")) {
      int newBpm = (int)gl["bpm"];
      if (newBpm >= 60 && newBpm <= 400) {
        bpm = newBpm;
      } else {
        Serial.printf("[SOUND] Invalid BPM %d, keeping %d\n", newBpm, bpm);
      }
    }
    
    if (gl.containsKey("master_vol")) {
      int newVol = (int)gl["master_vol"];
      if (newVol >= 0 && newVol <= 127) {
        master_vol = newVol;
      }
    }
    
    if (gl.containsKey("master_filter")) {
      int newFilter = (int)gl["master_filter"];
      if (newFilter >= 0 && newFilter <= 127) {
        master_filter = newFilter;
      }
    }
    
    if (gl.containsKey("octave")) {
      int newOct = (int)gl["octave"];
      if (newOct >= 0 && newOct <= 10) {
        octave = newOct;
      }
    }
    
    // NOUVEAU: Charger paramètres pattern si disponibles
    if (gl.containsKey("firstStep")) {
      int newFirst = (int)gl["firstStep"];
      if (newFirst >= 0 && newFirst <= 15) firstStep = newFirst;
    }
    if (gl.containsKey("lastStep")) {
      int newLast = (int)gl["lastStep"];
      if (newLast >= 0 && newLast <= 15) lastStep = newLast;
    }
    if (gl.containsKey("isMelodic")) {
      isMelodic = (uint16_t)((int)gl["isMelodic"]);
    }
  }

  auto voices = doc["voices"];
  if (!voices.isNull() && voices.size() >= 16) {
    for (int v = 0; v < 16; v++) {
      auto vo = voices[v];
      auto pa = vo["params"];
      if (!pa.isNull() && pa.size() >= 8) {
        for (int p = 0; p < 8; p++) {
          int32_t newVal = (int32_t)pa[p].as<int>();
          
          // NOUVEAU: Validation des paramètres selon leurs bornes
          if (p < 16) { // Si on a des bornes définies
            extern const int min_values[16];
            extern const int max_values[16];
            if (newVal >= min_values[p] && newVal <= max_values[p]) {
              ROTvalue[v][p] = newVal;
            } else {
              Serial.printf("[SOUND] Invalid param[%d][%d]=%d, keeping %d\n", 
                           v, p, (int)newVal, (int)ROTvalue[v][p]);
            }
          } else {
            ROTvalue[v][p] = newVal;
          }
        }
      } else {
        Serial.printf("[SOUND] Warning: incomplete params for voice %d\n", v);
      }
      
      // NOUVEAU: Charger nom de sample si disponible
      if (vo.containsKey("sample_name")) {
        sound_names[v] = vo["sample_name"].as<String>();
      }
      
      // Yield périodique pour watchdog
      if (v % 4 == 0) {
        yield();
      }
    }
  } else {
    Serial.printf("[SOUND] Warning: incomplete voices data in %s\n", path.c_str());
  }

  f.close();
  
  // NOUVEAU: Réappliquer les paramètres avec validation
  Serial.printf("[SOUND] Applying loaded parameters...\n");
  for (byte v = 0; v < 16; v++) {
    setSound(v);
    // Yield pour éviter watchdog timeout
    if (v % 4 == 0) {
      yield();
    }
  }
  
  // Mettre à jour le moteur avec les nouveaux globaux
  synthESP32_setMVol((uint8_t)master_vol);
  synthESP32_setMFilter((uint8_t)master_filter);
  
  select_rot();
  
  Serial.printf("[SOUND] Loaded sound %d <- %s (BPM:%d, Vol:%d)\n", 
    idx, path.c_str(), bpm, master_vol);
}