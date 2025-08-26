extern int BPOS[48][4];
extern Arduino_NV3041A *gfx;
extern int LCD_W, LCD_H;
extern uint8_t trigger_on[48];

static String kb_buf;
static bool keyboard_open=false, keyboard_ok=false;

static void _kbClear(){
  gfx->fillRect(0,0,LCD_W,LCD_H,0x0000);
  for (int i=0;i<48;i++){ BPOS[i][0]=BPOS[i][1]=BPOS[i][2]=BPOS[i][3]=0; }
}
static void _kbTitle(const char* t){
  gfx->fillRect(0,0,LCD_W,24,0xFFFF);
  gfx->setTextColor(0x0000);
  gfx->setCursor(8,17); gfx->print(t);
}
static int _btn(int x,int y,int w,int h,const char* l){
  int idx=-1; for(int i=0;i<48;i++){ if(BPOS[i][2]==0){ idx=i;break; } }
  if (idx<0) return -1;
  BPOS[idx][0]=x; BPOS[idx][1]=y; BPOS[idx][2]=w; BPOS[idx][3]=h;
  gfx->drawRect(x,y,w,h,0xFFFF);
  gfx->setTextColor(0xFFFF); gfx->setCursor(x+8,y+h/2+4); gfx->print(l);
  return idx;
}
static const char* KB_ROWS[3] = { "1234567890", "QWERTYUIOP", "ASDFGHJKL_" };
static const char* KB_ROW4     = "ZXCVBNM.-  ";

static void _drawBuffer(){
  gfx->fillRect(8, 30, LCD_W-16, 26, 0x0000);
  gfx->drawRect(8, 30, LCD_W-16, 26, 0xFFFF);
  gfx->setCursor(12, 48); gfx->setTextColor(0xFFFF); gfx->print(kb_buf);
}
static void _drawKeyboard(){
  int pad=8, bw=(LCD_W - pad*11)/10, bh=34, y=70;
  for(int r=0;r<3;r++){
    int x=pad;
    for(int c=0;c<10;c++){
      char lab[2]={KB_ROWS[r][c],0};
      _btn(x,y,bw,bh,lab); x+=bw+pad;
    }
    y+=bh+6;
  }
  int x=pad;
  for(int c=0;c<10;c++){
    char lab[2]={KB_ROW4[c],0};
    _btn(x,y,bw,bh,lab); x+=bw+pad;
  }
  y+=bh+10;
  _btn(pad, y, 100, 36, "  OK  ");
  _btn(pad+110, y, 120, 36, "  DEL ");
  _btn(pad+240, y, 140, 36, "CANCEL");
}
static int _orderOf(int ti){ int o=0; for(int i=0;i<48;i++){ if(BPOS[i][2]>0){ if(i==ti) return o; o++; } } return -1; }

bool keyboardPrompt(char* out, size_t maxlen, const char* title){
  kb_buf=""; keyboard_open=true; keyboard_ok=false;
  _kbClear(); _kbTitle(title); _drawBuffer(); _drawKeyboard();

  while (keyboard_open){
    for (int i=0;i<48;i++){
      if (!trigger_on[i]) continue; trigger_on[i]=0;
      int ord = _orderOf(i);
      if (ord<0) continue;
      if (ord<40){
        char c;
        if (ord<10) c=KB_ROWS[0][ord];
        else if (ord<20) c=KB_ROWS[1][ord-10];
        else if (ord<30) c=KB_ROWS[2][ord-20];
        else c=KB_ROW4[ord-30];
        if ((int)kb_buf.length() < (int)maxlen-1){ kb_buf+=c; _drawBuffer(); }
      } else if (ord==40){ keyboard_ok=true; keyboard_open=false; }
      else if (ord==41){ if (kb_buf.length()){ kb_buf.remove(kb_buf.length()-1); _drawBuffer(); } }
      else if (ord==42){ keyboard_open=false; }
    }
    delay(6);
  }
  if (keyboard_ok){ strncpy(out, kb_buf.c_str(), maxlen); out[maxlen-1]='\0'; return true; }
  return false;
}
