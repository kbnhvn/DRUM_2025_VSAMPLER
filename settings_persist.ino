#include <SD.h>
#include <ArduinoJson.h>

extern uint8_t g_midiChannel;   // déclaré dans midi.ino

static const char* kCfgDir  = "/config";
static const char* kCfgPath = "/config/settings.json";

// Charge g_midiChannel (défaut 1 si absent)
void Settings_load_midi()
{
  File f = SD.open(kCfgPath, FILE_READ);
  if (!f) { g_midiChannel = 1; return; }

  DynamicJsonDocument doc(512);
  DeserializationError e = deserializeJson(doc, f);
  f.close();
  if (e) { g_midiChannel = 1; return; }

  g_midiChannel = (uint8_t)(doc["midi_channel"] | 1);
  if (g_midiChannel < 1 || g_midiChannel > 16) g_midiChannel = 1;
}

// Sauvegarde g_midiChannel en fusionnant le JSON existant
void Settings_save_midi()
{
  // Lire existant (si présent)
  DynamicJsonDocument doc(512);
  {
    File fin = SD.open(kCfgPath, FILE_READ);
    if (fin) {
      DeserializationError e = deserializeJson(doc, fin);
      fin.close();
      if (e) doc.clear(); // si invalide, on repart d’un doc vide
    }
  }

  // Mettre à jour uniquement le champ MIDI
  doc["midi_channel"] = g_midiChannel;

  // S’assurer du dossier
  if (!SD.exists(kCfgDir)) SD.mkdir(kCfgDir);

  // Écrire le fichier
  File fout = SD.open(kCfgPath, FILE_WRITE);
  if (!fout) return;
  serializeJson(doc, fout);
  fout.close();
}
