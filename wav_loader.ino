#include <SD.h>

extern "C" bool loadWavToSlot(const char* path, uint8_t slot);

struct SampleSlot {
  int16_t* data=nullptr;
  uint32_t frames=0;
  uint8_t  channels=1;
  float    speed=1.0f, pitch=1.0f;
  uint32_t startFrame=0, stopFrame=0;
  String   name;
};
SampleSlot SAMPLE_SLOTS[50];

static bool parseWavHeader(File& f, uint16_t& ch, uint32_t& sr, uint16_t& bps, uint32_t& dataOfs, uint32_t& dataLen){
  char tag[5]={0}; uint32_t pos=0; f.read((uint8_t*)tag,4); if(strncmp(tag,"RIFF",4)) return false; pos=8;
  f.seek(pos); f.read((uint8_t*)tag,4); if(strncmp(tag,"WAVE",4)) return false; pos=12;
  uint16_t af=0; ch=0; sr=0; bps=0; dataOfs=0; dataLen=0;
  while (pos<f.size()){
    f.seek(pos); f.read((uint8_t*)tag,4); uint32_t len; f.read((uint8_t*)&len,4); pos+=8;
    if (!strncmp(tag,"fmt ",4)){ f.read((uint8_t*)&af,2); f.read((uint8_t*)&ch,2); f.read((uint8_t*)&sr,4);
      uint32_t br; f.read((uint8_t*)&br,4); uint16_t ba; f.read((uint8_t*)&ba,2); f.read((uint8_t*)&bps,2); pos+=len-16; }
    else if (!strncmp(tag,"data",4)){ dataOfs=pos; dataLen=len; break; }
    else pos+=len;
  }
  return (af==1 && bps==16 && dataOfs>0);
}

bool loadWavToSlot(const char* path, uint8_t slot){
  File f=SD.open(path,FILE_READ); if(!f) return false;
  uint16_t ch,bps; uint32_t sr,ofs,len;
  if(!parseWavHeader(f,ch,sr,bps,ofs,len)){ f.close(); return false; }
  f.seek(ofs);
  int16_t* buf=(int16_t*)heap_caps_malloc(len, MALLOC_CAP_8BIT); if(!buf){ f.close(); return false; }
  f.read((uint8_t*)buf,len); f.close();
  uint32_t frames=(len/2)/ch;
  if (SAMPLE_SLOTS[slot].data) free(SAMPLE_SLOTS[slot].data);
  SAMPLE_SLOTS[slot].data=buf; SAMPLE_SLOTS[slot].frames=frames; SAMPLE_SLOTS[slot].channels=ch;
  SAMPLE_SLOTS[slot].speed=1.0f; SAMPLE_SLOTS[slot].pitch=1.0f; SAMPLE_SLOTS[slot].startFrame=0; SAMPLE_SLOTS[slot].stopFrame=frames;
  SAMPLE_SLOTS[slot].name=String(path);
  return true;
}
