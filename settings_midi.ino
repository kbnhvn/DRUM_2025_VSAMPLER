extern int BPOS[48][4];
extern Arduino_GFX *gfx;
extern int LCD_W, LCD_H;
extern uint8_t trigger_on[48];

#include "ui_helpers.ino"

extern volatile PageId currentPage;
extern uint8_t g_midiChannel;

// persist
void Settings_save_midi(); // depuis settings_persist.ino

static int _smbtn(int x,int y,int w,int h,const char* l){
  int idx=-1; for(int i=0;i<48;i++){ if(BPOS[i][2]==0){ idx=i;break;} }
  if (idx<0) return -1;
  BPOS[idx][0]=x; BPOS[idx][1]=y; BPOS[idx][2]=w; BPOS[idx][3]=h;
  gfx->drawRect(x,y,w,h,0xFFFF);
  gfx->setTextColor(0xFFFF); gfx->setCursor(x+8,y+h/2+4); gfx->print(l);
  return idx;
}
static int _ord(int ti){ int o=0; for(int i=0;i<48;i++){ if(BPOS[i][2]>0){ if(i==ti) return o; o++; } } return -1; }

static void _drawValue(){
  gfx->fillRect(10,80, LCD_W-20, 40, 0x0000);
  gfx->drawRect(10,80, LCD_W-20, 40, 0xFFFF);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(20, 106);
  gfx->print("MIDI Channel: ");
  gfx->print(g_midiChannel);
}

void SettingsMIDI_enter(){
  LCD_clearWorkArea(); LCD_drawTitle("SETTINGS > MIDI CHANNEL");
  int pad=10, bw=(LCD_W - pad*4)/3, bh=44, y=140;
  _smbtn(pad,            y, bw, bh, " CH- ");
  _smbtn(pad+bw+pad,     y, bw, bh, " CH+ ");
  _smbtn(pad+(bw+pad)*2, y, bw, bh, " BACK ");
  _drawValue();
}

void SettingsMIDI_loop(){
  for (int i=0;i<48;i++){
    if (!trigger_on[i]) continue; trigger_on[i]=0;
    int o = _ord(i);
    if (o==0){ if (g_midiChannel>1) g_midiChannel--; _drawValue(); }
    else if (o==1){ if (g_midiChannel<16) g_midiChannel++; _drawValue(); }
    else if (o==2){
      Settings_save_midi();
      LCD_toast("MIDI channel saved");
      currentPage=PAGE_SETTINGS; // retourne sur Settings généraux si tu as une page
      // si tu n'as pas de page Settings, remets PAGE_MAIN :
      // currentPage=PAGE_MAIN;
    }
  }
}
