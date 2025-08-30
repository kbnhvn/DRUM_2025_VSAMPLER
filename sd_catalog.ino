#include <FS.h>
#include <SD.h>
#include <vector>
#include "sd_catalog.h"

#define BANK_SIZE 256
int16_t* SAMPLES[BANK_SIZE];
uint64_t ENDS[BANK_SIZE];
String   sound_names[BANK_SIZE];

static String stripExt(const String& n){ int d=n.lastIndexOf('.'); return (d>0)? n.substring(0,d):n; }

static bool readWav16Mono(const char* path, int16_t** out, uint32_t* outlen){
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  char h[4]; f.readBytes(h,4); if (strncmp(h,"RIFF",4)!=0){ f.close(); return false; }
  f.seek(8); f.readBytes(h,4); if (strncmp(h,"WAVE",4)!=0){ f.close(); return false; }
  bool ok=false; uint16_t fmt=0,ch=0,bps=0; uint32_t dsz=0,doff=0;
  while (f.available()){
    char id[4]; if (f.readBytes(id,4)!=4) break; uint32_t sz=0; f.read((uint8_t*)&sz,4);
    if (!strncmp(id,"fmt ",4)){ uint8_t b[16]; if (sz<16){ f.close(); return false; }
      f.read(b,16); fmt=b[0]|(b[1]<<8); ch=b[2]|(b[3]<<8); bps=b[14]|(b[15]<<8); if (sz>16) f.seek(f.position()+sz-16);
      ok=true;
    } else if (!strncmp(id,"data",4)){ dsz=sz; doff=f.position(); f.seek(f.position()+sz); }
    else { f.seek(f.position()+sz); }
  }
  if (!ok || fmt!=1 || bps!=16 || dsz==0){ f.close(); return false; }
  uint32_t total = dsz/2/(ch?ch:1);
  int16_t* buf = (int16_t*)heap_caps_malloc(total*sizeof(int16_t), MALLOC_CAP_8BIT|MALLOC_CAP_SPIRAM);
  if(!buf){ f.close(); return false; }
  f.seek(doff);
  if(ch==1){ f.read((uint8_t*)buf, total*2); }
  else{ for(uint32_t i=0;i<total;i++){ int16_t L,R; f.read((uint8_t*)&L,2); f.read((uint8_t*)&R,2); buf[i]=(int16_t)(((int32_t)L+(int32_t)R)/2); } }
  f.close();
  *out=buf; *outlen=total; return true;
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
      SampleMeta m; m.path = d + "/" + nm; m.name = stripExt(nm);
      CATALOG.push_back(m);
    }
  }
}

void initSD(){
  if (!SD.begin()) { Serial.println("[SD] init failed"); return; }
  if (!SD.exists("/samples")) SD.mkdir("/samples");
}

void buildCatalog(){
  CATALOG.clear();
  scanDir("/samples");
  Serial.printf("[SD] catalog: %u files\n", (unsigned)CATALOG.size());
  for (int i=0;i<BANK_SIZE;i++){ SAMPLES[i]=nullptr; ENDS[i]=0; sound_names[i]=""; }
  int n = min(BANK_SIZE, (int)CATALOG.size());
  for (int i=0;i<n;i++){ sound_names[i]=CATALOG[i].name; }
}

// Charge un sample dans un slot donné
void assignSampleToSlot(int catIndex, int slot){
  if (slot<0 || slot>=BANK_SIZE) return;
  if (catIndex<0 || catIndex>=(int)CATALOG.size()) return;
  if (!SAMPLES[slot]){
    int16_t* b=nullptr; uint32_t l=0;
    if (!readWav16Mono(CATALOG[catIndex].path.c_str(), &b, &l)) return;
    SAMPLES[slot]=b; ENDS[slot]=(l>0)?(l-1):0;
  }
  sound_names[slot]=CATALOG[catIndex].name;
}
