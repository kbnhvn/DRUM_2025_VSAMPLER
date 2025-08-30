// files_tools.ino — SD only, JSON patterns & sound-sets
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <ArduinoJson.h>

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
extern void     select_rot();          // rafraichit l’UI barres si besoin

// Optionnel : pour UI (si tu affiches les noms)
// Si inexistant, laisse commenté — ce n'est pas nécessaire pour le fonctionnement.
// extern String   sound_names[];

// ==== Utils ====
static void ensureDir(const char* path) {
  if (!SD.exists(path)) SD.mkdir(path);
}

static bool writeJsonAtomic(const String& finalPath, DynamicJsonDocument& doc) {
  String tmpPath = finalPath + ".tmp";
  File f = SD.open(tmpPath, FILE_WRITE);
  if (!f) return false;
  bool ok = (serializeJson(doc, f) > 0);
  f.close();
  if (!ok) { SD.remove(tmpPath); return false; }
  SD.remove(finalPath);          // idempotent
  return SD.rename(tmpPath, finalPath);
}

static bool readJson(const String& path, DynamicJsonDocument& doc) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  return !err;
}

static String twoDigits(byte n) {
  if (n < 10) return "0" + String(n);
  return String(n);
}

// ==== PATTERN ====
// Chemin: /patterns/pattern_XX.json
// Contenu:
// {
//   "version": 1,
//   "firstStep": <int>,
//   "lastStep":  <int>,
//   "isMelodic": <int>,
//   "steps": [ uint16_t x16 ]
// }

void save_pattern(byte idx) {
  ensureDir("/patterns");
  String path = "/patterns/pattern_" + twoDigits(idx) + ".json";

  DynamicJsonDocument doc(4096);
  doc["version"]   = 1;
  doc["firstStep"] = (int)firstStep;
  doc["lastStep"]  = (int)lastStep;
  doc["isMelodic"] = (int)isMelodic;

  JsonArray steps = doc.createNestedArray("steps");
  for (int s = 0; s < 16; s++) steps.add((uint16_t)pattern[s]);

  bool ok = writeJsonAtomic(path, doc);
  Serial.println(ok ? "[PATTERN] Saved -> " + path : "[PATTERN] Save FAILED -> " + path);
}

void load_pattern(byte idx) {
  String path = "/patterns/pattern_" + twoDigits(idx) + ".json";
  DynamicJsonDocument doc(4096);
  if (!readJson(path, doc)) {
    Serial.println("[PATTERN] Load FAILED -> " + path);
    return;
  }

  if (doc.containsKey("firstStep")) firstStep = (byte)((int)doc["firstStep"]);
  if (doc.containsKey("lastStep"))  lastStep  = (byte)((int)doc["lastStep"]);
  if (doc.containsKey("isMelodic")) isMelodic = (uint16_t)((int)doc["isMelodic"]);

  JsonArray steps = doc["steps"];
  if (!steps.isNull() && steps.size() >= 16) {
    for (int s = 0; s < 16; s++) {
      pattern[s] = (uint16_t)steps[s].as<uint16_t>();
    }
  }

  // Le séquenceur appliquera ces valeurs, l’UI se rafraichira via flags existants.
  Serial.println("[PATTERN] Loaded <- " + path);
}

// ==== SOUND-SET ====
// Chemin: /sounds/sound_XX.json
// Contenu:
// {
//   "version": 1,
//   "globals": { "bpm":..., "master_vol":..., "master_filter":..., "octave":... },
//   "voices": [
//      { "params":[SAM,INI,END,PIT,RVS,VOL,PAN,FIL] } x16
//   ]
// }

void save_sound(byte idx) {
  ensureDir("/sounds");
  String path = "/sounds/sound_" + twoDigits(idx) + ".json";

  DynamicJsonDocument doc(8192);
  doc["version"] = 1;

  JsonObject gl = doc.createNestedObject("globals");
  gl["bpm"]          = bpm;
  gl["master_vol"]   = master_vol;
  gl["master_filter"]= master_filter;
  gl["octave"]       = octave;

  JsonArray voices = doc.createNestedArray("voices");
  for (int v = 0; v < 16; v++) {
    JsonObject vo = voices.createNestedObject();
    JsonArray pa  = vo.createNestedArray("params");
    for (int p = 0; p < 8; p++) pa.add((int)ROTvalue[v][p]);
  }

  bool ok = writeJsonAtomic(path, doc);
  Serial.println(ok ? "[SOUND] Saved -> " + path : "[SOUND] Save FAILED -> " + path);
}

void load_sound(byte idx) {
  String path = "/sounds/sound_" + twoDigits(idx) + ".json";
  DynamicJsonDocument doc(8192);
  if (!readJson(path, doc)) {
    Serial.println("[SOUND] Load FAILED -> " + path);
    return;
  }

  JsonObject gl = doc["globals"];
  if (!gl.isNull()) {
    if (gl.containsKey("bpm"))           bpm = (int)gl["bpm"];
    if (gl.containsKey("master_vol"))    master_vol = (int)gl["master_vol"];
    if (gl.containsKey("master_filter")) master_filter = (int)gl["master_filter"];
    if (gl.containsKey("octave"))        octave = (int)gl["octave"];
  }

  JsonArray voices = doc["voices"];
  if (!voices.isNull() && voices.size() >= 16) {
    for (int v = 0; v < 16; v++) {
      JsonObject vo = voices[v];
      JsonArray pa = vo["params"];
      if (!pa.isNull() && pa.size() >= 8) {
        for (int p = 0; p < 8; p++) {
          ROTvalue[v][p] = (int32_t)pa[p].as<int>();
        }
      }
    }
  }

  // Réapplique les params à l’audio (assignation, gains, filtres…)
  for (byte v = 0; v < 16; v++) setSound(v);
  select_rot(); // si tu veux rafraîchir la sélection de barres

  Serial.println("[SOUND] Loaded <- " + path);
}
