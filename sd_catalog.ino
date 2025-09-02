#include <FS.h>
#include <SD.h>
#include <vector>
#include "sd_catalog.h"
#include "esp_task_wdt.h"

// CORRECTION: Déclaration manquante du CATALOG global
std::vector<SampleMeta> CATALOG;

#define BANK_SIZE 256
int16_t* SAMPLES[BANK_SIZE];
uint64_t ENDS[BANK_SIZE];
String   sound_names[BANK_SIZE];

static String stripExt(const String& n){ 
  int d=n.lastIndexOf('.'); 
  return (d>0)? n.substring(0,d):n; 
}

// CORRECTION: Parser header WAV streaming sécurisé
static WavHeader parseWavHeader(File& f) {
  WavHeader h = {0};
  h.valid = false;
  
  if (f.size() < 44) return h;
  
  f.seek(0);
  char buf[4];
  
  // RIFF header
  if (f.readBytes(buf, 4) != 4 || strncmp(buf, "RIFF", 4) != 0) return h;
  
  uint32_t fileSize;
  f.read((uint8_t*)&fileSize, 4);
  
  if (f.readBytes(buf, 4) != 4 || strncmp(buf, "WAVE", 4) != 0) return h;
  
  // Parse chunks streaming
  bool fmtFound = false, dataFound = false;
  
  while (f.available() >= 8 && (!fmtFound || !dataFound)) {
    if (f.readBytes(buf, 4) != 4) break;
    
    uint32_t chunkSize;
    f.read((uint8_t*)&chunkSize, 4);
    
    // Protection overflow
    if (chunkSize > f.size() || f.position() + chunkSize > f.size()) break;
    
    if (!strncmp(buf, "fmt ", 4) && chunkSize >= 16) {
      uint8_t fmtData[20];
      size_t toRead = min((size_t)chunkSize, sizeof(fmtData));
      f.read(fmtData, toRead);
      
      h.format = fmtData[0] | (fmtData[1] << 8);
      h.channels = fmtData[2] | (fmtData[3] << 8);
      h.sampleRate = fmtData[4] | (fmtData[5] << 8) | (fmtData[6] << 16) | (fmtData[7] << 24);
      h.bitsPerSample = fmtData[14] | (fmtData[15] << 8);
      
      if (chunkSize > toRead) f.seek(f.position() + chunkSize - toRead);
      fmtFound = true;
      
    } else if (!strncmp(buf, "data", 4)) {
      h.dataSize = chunkSize;
      h.dataOffset = f.position();
      f.seek(f.position() + chunkSize);
      dataFound = true;
    } else {
      f.seek(f.position() + chunkSize);
    }
  }
  
  // Validation complète
  h.valid = fmtFound && dataFound && 
            h.format == 1 && 
            h.bitsPerSample == 16 && 
            h.channels > 0 && h.channels <= 2 &&
            h.sampleRate > 0 && h.sampleRate <= 192000 &&
            h.dataSize > 0;
            
  return h;
}

static bool readWav16Mono(const char* path, int16_t** out, uint32_t* outlen){
  *out = nullptr;
  *outlen = 0;
  
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("[WAV] Cannot open: %s\n", path);
    return false;
  }
  
  WavHeader h = parseWavHeader(f);
  if (!h.valid) {
    Serial.printf("[WAV] Invalid format: %s\n", path);
    f.close();
    return false;
  }
  
  uint32_t totalSamples = h.dataSize / 2 / h.channels;
  size_t neededMem = totalSamples * sizeof(int16_t);
  
  // Vérification mémoire avec marge de sécurité
  size_t availMem = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  if (neededMem > availMem * 0.8f) {
    Serial.printf("[WAV] Insufficient SPIRAM: need %u, have %u\n", 
                  (unsigned)neededMem, (unsigned)availMem);
    f.close();
    return false;
  }
  
  int16_t* buffer = (int16_t*)heap_caps_malloc(neededMem, MALLOC_CAP_SPIRAM);
  if (!buffer) {
    Serial.printf("[WAV] SPIRAM allocation failed: %s\n", path);
    f.close();
    return false;
  }
  
  f.seek(h.dataOffset);
  
  // Lecture par chunks pour éviter timeouts
  const size_t CHUNK_SIZE = 4096;
  uint32_t samplesRead = 0;
  
  if (h.channels == 1) {
    // Mono direct par chunks
    while (samplesRead < totalSamples) {
      // CORRECTION: Harmonisation des types uint32_t
      uint32_t toRead = min((uint32_t)(CHUNK_SIZE / 2), totalSamples - samplesRead);
      size_t bytesRead = f.read((uint8_t*)(buffer + samplesRead), toRead * 2);
      
      if (bytesRead != toRead * 2) {
        Serial.printf("[WAV] Read error at sample %u\n", (unsigned)samplesRead);
        free(buffer);
        f.close();
        return false;
      }
      
      samplesRead += toRead;
      
      // Yield pour watchdog
      if (samplesRead % 8192 == 0) {
        yield();
      }
    }
  } else {
    // Stéréo -> mono avec mixdown
    while (samplesRead < totalSamples) {
      // CORRECTION: Harmonisation des types uint32_t
      uint32_t toRead = min((uint32_t)(CHUNK_SIZE / 4), totalSamples - samplesRead);
      
      for (uint32_t i = 0; i < toRead; i++) {
        int16_t L, R;
        if (f.read((uint8_t*)&L, 2) != 2 || f.read((uint8_t*)&R, 2) != 2) {
          Serial.printf("[WAV] Stereo read error at sample %u\n", (unsigned)(samplesRead + i));
          free(buffer);
          f.close();
          return false;
        }
        
        // Mixdown avec protection overflow
        int32_t mix = ((int32_t)L + (int32_t)R) / 2;
        buffer[samplesRead + i] = (int16_t)constrain(mix, INT16_MIN, INT16_MAX);
      }
      
      samplesRead += toRead;
      
      if (samplesRead % 4096 == 0) {
        yield();
      }
    }
  }
  
  f.close();
  
  *out = buffer;
  *outlen = totalSamples;
  
  Serial.printf("[WAV] Loaded %s: %u samples @ %u Hz (%.2fs)\n", 
                path, (unsigned)totalSamples, (unsigned)h.sampleRate,
                (float)totalSamples / h.sampleRate);
  
  return true;
}

// CORRECTION: Validation rapide sans chargement complet
bool validateWavFile(const char* path, float* duration, uint32_t* sampleRate) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  
  WavHeader h = parseWavHeader(f);
  f.close();
  
  if (h.valid && duration) {
    uint32_t samples = h.dataSize / 2 / h.channels;
    *duration = (float)samples / h.sampleRate;
  }
  if (h.valid && sampleRate) {
    *sampleRate = h.sampleRate;
  }
  
  return h.valid;
}

static void scanDir(const String& d){
  File root = SD.open(d);
  if (!root) return;
  while (true){
    File f = root.openNextFile();
    if (!f) break;
    String nm = String(f.name());
    bool isDir = f.isDirectory();
    f.close();
    if (isDir) scanDir(d + "/" + nm);
    else if (nm.endsWith(".wav") || nm.endsWith(".WAV")){
      SampleMeta m; 
      m.path = d + "/" + nm; 
      m.name = stripExt(nm);

      // Validation et métadonnées à la volée
      float dur = 0;
      uint32_t rate = 0;
      if (validateWavFile(m.path.c_str(), &dur, &rate)) {
        m.rate = rate;
      }
     
      CATALOG.push_back(m);
    }
  }
}

// CORRECTION: Fermer la fonction initSD proprement
void initSD(){
  if (!SD.begin()) { 
    Serial.println("[SD] init failed"); 
    return; 
  }
  if (!SD.exists("/samples")) {
    SD.mkdir("/samples");
    Serial.println("[SD] Created /samples directory");
  }
  Serial.println("[SD] Initialized successfully");
} // CORRECTION: Accolade fermante ajoutée

void buildCatalog(){
  CATALOG.clear();
  scanDir("/samples");
  Serial.printf("[SD] catalog: %u files\n", (unsigned)CATALOG.size());
  for (int i=0;i<BANK_SIZE;i++){ 
    SAMPLES[i]=nullptr; 
    ENDS[i]=0; 
    sound_names[i]=""; 
  }
  int n = min(BANK_SIZE, (int)CATALOG.size());
  for (int i=0;i<n;i++){ 
    sound_names[i]=CATALOG[i].name; 
  }
}

// CORRECTION: Fonction complète avec return
bool assignSampleToSlot(int catIndex, int slot){
  if (slot < 0 || slot >= BANK_SIZE) {
    Serial.printf("[CATALOG] Invalid slot: %d\n", slot);
    return false;
  }
  if (catIndex < 0 || catIndex >= (int)CATALOG.size()) {
    Serial.printf("[CATALOG] Invalid catalog index: %d\n", catIndex);
    return false;
  }
  
  // Libération mémoire sécurisée AVANT allocation
  if (SAMPLES[slot]) {
    Serial.printf("[CATALOG] Freeing slot %d (%u bytes)\n", 
                  slot, (unsigned)(ENDS[slot] * sizeof(int16_t)));
    free(SAMPLES[slot]);
    SAMPLES[slot] = nullptr;
    ENDS[slot] = 0;
    sound_names[slot] = "";
  }
  
  // Vérification mémoire disponible
  size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  Serial.printf("[CATALOG] SPIRAM free: %u bytes\n", (unsigned)freeHeap);

  int16_t* b=nullptr; 
  uint32_t l=0;
  
  if (!readWav16Mono(CATALOG[catIndex].path.c_str(), &b, &l)) {
    Serial.printf("[CATALOG] Failed to load: %s\n", CATALOG[catIndex].path.c_str());
    return false;
  }
  
  if (!b || l == 0) {
    Serial.println("[CATALOG] Invalid sample data");
    return false;
  }
  
  SAMPLES[slot] = b; 
  ENDS[slot] = (l>0) ? (l-1) : 0;
  sound_names[slot] = CATALOG[catIndex].name;
  
  // Mettre à jour les métadonnées dans CATALOG
  CATALOG[catIndex].buf = b;
  CATALOG[catIndex].len = l;
  
  Serial.printf("[CATALOG] Loaded slot %d: %s (%u samples)\n", 
                slot, CATALOG[catIndex].name.c_str(), (unsigned)l);
  
  return true;
}

bool loadSampleBuffer(int catIndex, int slot) {
  return assignSampleToSlot(catIndex, slot);
}