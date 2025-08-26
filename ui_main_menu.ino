extern int BPOS[48][4];
extern Arduino_NV3041A *gfx;
extern int LCD_W, LCD_H;
extern uint8_t trigger_on[48];

// #include "ui_helpers.ino"

// Enum pages (déjà ajoutée via diff dans DRUM_2025_VSAMPLER.ino)
extern volatile PageId currentPage;

// Entrées des autres pages
extern void Recorder_enter();
extern void Files_enter();
extern void Settings_enter();
extern void WIFI_enter();
extern void SettingsMIDI_enter();
extern void Sampler_enter(); // ton sampler existant

// Map BPOS → ordre de boutons (0..5)
static int _btnOrderFromTouch(int ti){
  int ord=0;
  for (int i=0;i<48;i++){
    if (BPOS[i][2]>0){ if (i==ti) return ord; ord++; }
  }
  return -1;
}

void MainMenu_enter(){
  LCD_clearWorkArea();
  LCD_drawTitle("MAIN MENU");
  int pad=10, cols=3, rows=2;
  int bw=(LCD_W - pad*(cols+1))/cols;
  int bh=(LCD_H-24 - pad*(rows+1))/rows;
  int y=24+pad;

  // Ligne 1
  _allocButton(pad,             y, bw, bh, " SAMPLER ");
  _allocButton(pad+bw+pad,      y, bw, bh, " RECORDER ");
  _allocButton(pad+(bw+pad)*2,  y, bw, bh, " FILES ");
  // Ligne 2
  y += bh+pad;
  _allocButton(pad,             y, bw, bh, " SETTINGS ");
  _allocButton(pad+bw+pad,      y, bw, bh, " MIDI ");
  _allocButton(pad+(bw+pad)*2,  y, bw, bh, " WIFI ");
}

void MainMenu_loop(){
  for (int i=0;i<48;i++){
    if (!trigger_on[i]) continue; trigger_on[i]=0;
    int bi = _btnOrderFromTouch(i);
    switch (bi){
      case 0: currentPage=PAGE_SAMPLER; if (Sampler_enter) Sampler_enter(); break;
      case 1: Recorder_enter(); break;
      case 2: Files_enter(); break;
      case 3: Settings_enter(); break;
      case 4: SettingsMIDI_enter(); break;
      case 5: WIFI_enter(); break;
    }
  }
}
