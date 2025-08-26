#include <Arduino.h>

// Dépendances déjà présentes dans ton projet
extern int BPOS[48][4];     // [x,y,w,h]
extern Arduino_GFX *gfx;    // instance d'affichage
extern int LCD_W, LCD_H;    // 480 x 272

// Reset zone et BPOS
void LCD_clearWorkArea() {
  gfx->fillRect(0, 0, LCD_W, LCD_H, 0x0000);
  for (int i=0;i<48;i++) { BPOS[i][0]=BPOS[i][1]=BPOS[i][2]=BPOS[i][3]=0; }
}

// Bande titre simple
void LCD_drawTitle(const char* txt){
  gfx->fillRect(0, 0, LCD_W, 24, 0xFFFF);
  gfx->setTextColor(0x0000);
  gfx->setCursor(8, 17);
  gfx->print(txt);
}

// Alloue une case dans BPOS et dessine un bouton rectangulaire
static int _allocButton(int x,int y,int w,int h,const char* label){
  int idx=-1;
  for (int i=0;i<48;i++){
    if (BPOS[i][2]==0 && BPOS[i][3]==0){ idx=i; break; }
  }
  if (idx<0) return -1;
  BPOS[idx][0]=x; BPOS[idx][1]=y; BPOS[idx][2]=w; BPOS[idx][3]=h;
  gfx->drawRect(x,y,w,h,0xFFFF);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(x+8, y+h/2+4);
  gfx->print(label);
  return idx;
}

// Dessine une rangée de boutons (1..4 labels)
void LCD_drawButtonsRow(std::initializer_list<const char*> labels){
  int n = labels.size(); if (n<1) return;
  int pad=10;
  int bw = (LCD_W - (pad*(n+1)))/n;
  static int nextY = 28;
  int bh = 40;
  int x = pad;
  for (auto &lab : labels){
    _allocButton(x, nextY, bw, bh, lab);
    x += bw + pad;
  }
  nextY += bh + 10;
}

// Renvoie l’index de bouton (0..N-1) selon ordre de placement BPOS
int LCD_buttonIndex(int touchIndex){
  int ord=0;
  for (int i=0;i<48;i++){
    if (BPOS[i][2]>0 && BPOS[i][3]>0){
      if (i==touchIndex) return ord;
      ord++;
    }
  }
  return -1;
}

// Utilitaires texte/cadres
void LCD_drawSmallText(int x,int y,const char* t){
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(x,y);
  gfx->print(t);
}

// Encadrement d’une zone nommée
void LCD_drawFrameArea(const char* title,int x,int y,int w,int h){
  gfx->drawRect(x,y,w,h,0xFFFF);
  gfx->setCursor(x+4, y+12);
  gfx->print(title);
}

// Mini VU (0..255)
void LCD_drawVU(const char* label,int x,int y,int val){
  int w=64,h=10;
  gfx->drawRect(x,y,w,h,0xFFFF);
  int fill = map(val,0,255,0,w-2);
  gfx->fillRect(x+1,y+1,fill,h-2,0xFFFF);
  gfx->setCursor(x,y-2);
  gfx->print(label);
}

// Waveform light
void LCD_drawStartStopCursors(uint32_t startFrame,uint32_t stopFrame,uint32_t totalFrames){
  int x=10,y=100,w=LCD_W-20,h=80;
  gfx->drawRect(x,y,w,h,0xFFFF);
  if (totalFrames<2) return;
  int xs = x+1 + (int)((w-2)*(double)startFrame/(double)totalFrames);
  int xe = x+1 + (int)((w-2)*(double)stopFrame /(double)totalFrames);
  gfx->drawLine(xs,y, xs,y+h-1, 0xFFFF);
  gfx->drawLine(xe,y, xe,y+h-1, 0xFFFF);
}

// Toast simple
void LCD_toast(const char* t){
  int w= LCD_W-160, x=80, h=26, y=LCD_H-32;
  gfx->fillRect(x,y,w,h,0x0000);
  gfx->drawRect(x,y,w,h,0xFFFF);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(x+8, y+h-8);
  gfx->print(t);
}
