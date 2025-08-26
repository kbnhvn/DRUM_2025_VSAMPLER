#include <SD.h>
#include "driver/i2s.h"

// --- WAV header ---
struct WavHeader { uint32_t sampleRate; uint16_t bitsPerSample; uint16_t numChannels; uint32_t dataBytes; };
static void writeWavHeader(File& f, const WavHeader& h) {
    f.write(reinterpret_cast<const uint8_t*>("RIFF"), 4);
    uint32_t cs = 36 + h.dataBytes;
    f.write(reinterpret_cast<const uint8_t*>(&cs), 4);
    f.write(reinterpret_cast<const uint8_t*>("WAVE"), 4);
    f.write(reinterpret_cast<const uint8_t*>("fmt "), 4);
    uint32_t sc1 = 16;
    f.write(reinterpret_cast<const uint8_t*>(&sc1), 4);
    uint16_t fmt = 1;
    f.write(reinterpret_cast<const uint8_t*>(&fmt), 2);
    f.write(reinterpret_cast<const uint8_t*>(&h.numChannels), 2);
    f.write(reinterpret_cast<const uint8_t*>(&h.sampleRate), 4);
    uint32_t br = h.sampleRate * h.numChannels * (h.bitsPerSample / 8);
    f.write(reinterpret_cast<const uint8_t*>(&br), 4);
    uint16_t ba = h.numChannels * (h.bitsPerSample / 8);
    f.write(reinterpret_cast<const uint8_t*>(&ba), 2);
    f.write(reinterpret_cast<const uint8_t*>(&h.bitsPerSample), 2);
    f.write(reinterpret_cast<const uint8_t*>("data"), 4);
    f.write(reinterpret_cast<const uint8_t*>(&h.dataBytes), 4);
}

// --- State ---
static bool     recActive=false;
static uint32_t recBytes=0, recStartMs=0, recDurMs=0;
static File     recFile;
static char     recTempPath[64]="/samples/rec_tmp.wav";
static int16_t  rxBuf[256*2]; // stéréo

struct SampleEdit { String path; uint32_t frames=0; uint8_t channels=2; uint32_t startFrame=0, stopFrame=0; float speed=1.0f, pitch=1.0f; } g_recEdit;

// --- UI helpers minimalistes (utilise ui_helpers.ino déjà fourni) ---
extern void LCD_clearWorkArea(); extern void LCD_drawTitle(const char*); extern void LCD_drawButtonsRow(std::initializer_list<const char*>);
extern void LCD_drawFrameArea(const char*,int,int,int,int); extern void LCD_drawVU(const char*,int,int,int);
extern void LCD_drawSmallText(int,int,const char*); extern void LCD_drawStartStopCursors(uint32_t,uint32_t,uint32_t);
extern int LCD_buttonIndex(int); extern void LCD_toast(const char*); extern uint8_t trigger_on[48];

// --- Recorder page ---
static void drawRecUI(bool running,int vuL,int vuR,uint32_t ms){
  LCD_clearWorkArea(); LCD_drawTitle("RECORDER   [BACK]");
  LCD_drawButtonsRow({"  REC  ","  STOP "," SAVEAS"," EDIT  "});
  LCD_drawFrameArea("WAVE",10,64,460,80);
  LCD_drawVU("VU L",10,150,vuL); LCD_drawVU("VU R",84,150,vuR);
  char t[32]; sprintf(t,"dur: %02u:%02u.%03u",(ms/60000)%60,(ms/1000)%60,ms%1000);
  LCD_drawSmallText(180,150,t); if (running) LCD_drawSmallText(260,150,"● REC");
}

void Recorder_enter(){ recActive=false; recBytes=0; recDurMs=0; g_recEdit={}; g_recEdit.path=""; drawRecUI(false,0,0,0); }
void Recorder_loop(){
  for(int i=0;i<48;i++){
    if (!trigger_on[i]) continue; trigger_on[i]=0;
    if (LCD_buttonIndex(i)==0){ // REC
      if (!recActive){ recFile=SD.open(recTempPath,FILE_WRITE); if(!recFile){LCD_toast("SD open err");continue;}
        WavHeader hdr{44100,16,2,0}; writeWavHeader(recFile,hdr); recBytes=0; recActive=true; recStartMs=millis(); }
    } else if (LCD_buttonIndex(i)==1){ // STOP
      if (recActive){ recActive=false; recFile.seek(40); recFile.write((uint8_t*)&recBytes,4);
        recFile.seek(0); WavHeader hdr{44100,16,2,recBytes}; writeWavHeader(recFile,hdr); recFile.close();
        recDurMs=millis()-recStartMs; g_recEdit.path=recTempPath; g_recEdit.frames=(recBytes/4); g_recEdit.channels=2; g_recEdit.startFrame=0; g_recEdit.stopFrame=g_recEdit.frames; }
    } else if (LCD_buttonIndex(i)==2){ // SAVE AS
      if (!g_recEdit.path.length()){ LCD_toast("Nothing"); continue; }
      extern bool keyboardPrompt(char*,size_t,const char*); char out[48]="rec_001.wav";
      if (keyboardPrompt(out,sizeof(out),"Save as")){ char newP[80]; snprintf(newP,sizeof(newP),"/samples/%s",out);
        if (SD.exists(newP)) SD.remove(newP); SD.rename(recTempPath,newP); g_recEdit.path=String(newP); LCD_toast("Saved"); }
    } else if (LCD_buttonIndex(i)==3){ // EDIT view
      LCD_clearWorkArea(); LCD_drawTitle("REC-EDIT  [BACK]");
      LCD_drawButtonsRow({" S-  "," S+  "," E-  "," E+  "});
      LCD_drawButtonsRow({" SPD-"," SPD+"," PIT-"," PIT+"});
      LCD_drawButtonsRow({" PLAY"," SAVE"," RELOAD","WAVE"});
      LCD_drawStartStopCursors(g_recEdit.startFrame,g_recEdit.stopFrame,g_recEdit.frames);
    }
  }
  // VU + capture
  size_t rb=0; int vuL=0, vuR=0;
  if (recActive){
    i2s_read(I2S_NUM_0,(void*)rxBuf,sizeof(rxBuf),&rb,10);
    if (rb>0){ recFile.write((uint8_t*)rxBuf,rb); recBytes+=rb;
      int n=rb/4; int32_t accL=0,accR=0; int16_t* p=(int16_t*)rxBuf;
      for(int i=0;i<n;i++){ int16_t L=*p++,R=*p++; accL+=L*L; accR+=R*R; }
        vuL = (uint16_t)(sqrtf((float)accL / n)) >> 8;
        vuR = (uint16_t)(sqrtf((float)accR / n)) >> 8;
      recDurMs=millis()-recStartMs;
    }
  }
  drawRecUI(recActive,vuL,vuR,recDurMs);
}

// appelé côté audio-task si tu veux drainer RX même hors UI
void recorder_poll(){ if(!recActive) return; size_t rb=0; i2s_read(I2S_NUM_0,(void*)rxBuf,sizeof(rxBuf),&rb,0); if(rb>0){ recFile.write((uint8_t*)rxBuf,rb); recBytes+=rb; } }
