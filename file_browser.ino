#include <SD.h>

extern void LCD_clearWorkArea(); extern void LCD_drawTitle(const char*);
extern void LCD_drawButtonsRow(std::initializer_list<const char*>);
extern void LCD_drawSmallText(int,int,const char*); extern void LCD_toast(const char*);
extern int LCD_buttonIndex(int); extern uint8_t trigger_on[48];
extern bool keyboardPrompt(char*,size_t,const char*);
extern bool loadWavToSlot(const char*, uint8_t);

static int fb_first=0, fb_sel=-1, fb_count=0;
static String fb_entries[64];

static void refreshList(){
  fb_count=0; File dir=SD.open("/samples"); if(!dir||!dir.isDirectory()){ SD.mkdir("/samples"); dir=SD.open("/samples"); }
  File e; while((e=dir.openNextFile())){
    if (!e.isDirectory()){
      String n=e.name(); if (n.endsWith(".wav")||n.endsWith(".WAV")){
        if (fb_count<64) fb_entries[fb_count++]=String("/samples/")+n;
      }
    }
    e.close();
  }
  dir.close();
}

static void drawList(){
  // simple : 8 lignes
  int y=120; for(int i=0;i<8;i++){
    int idx=fb_first+i; char line[70]="";
    if (idx<fb_count){ snprintf(line,sizeof(line),"%c %s", (idx==fb_sel)?'>':' ', fb_entries[idx].c_str()); }
    LCD_drawSmallText(12,y,line); y+=18;
  }
}

void Files_enter(){
  LCD_clearWorkArea(); LCD_drawTitle("FILES   [BACK]");
  LCD_drawButtonsRow({" OPEN "," RENAME"," DELETE"," COPY "});
  LCD_drawButtonsRow({" UP  "," DOWN "," LOADS1"," LOADS2"});
  refreshList(); drawList();
}

void Files_loop(){
  for(int i=0;i<48;i++){
    if (!trigger_on[i]) continue; trigger_on[i]=0;
    int bi = LCD_buttonIndex(i);
    if (bi==4){ fb_first=max(0, fb_first-8); drawList(); }
    else if (bi==5){ fb_first=min(max(0,fb_count-8), fb_first+8); drawList(); }
    else if (bi==0 && fb_sel>=0){ LCD_toast("Open (preview TBI)"); }
    else if (bi==1 && fb_sel>=0){
      char newName[48]="renamed.wav";
      if (keyboardPrompt(newName,sizeof(newName),"Rename")){
        String np=String("/samples/")+newName; SD.rename(fb_entries[fb_sel].c_str(), np.c_str()); refreshList(); fb_sel=-1; drawList();
      }
    } else if (bi==2 && fb_sel>=0){
      SD.remove(fb_entries[fb_sel].c_str()); refreshList(); fb_sel=-1; drawList();
    } else if (bi==3 && fb_sel>=0){
      char newName[48]="copy.wav";
      if (keyboardPrompt(newName,sizeof(newName),"Copy to")){
        String np=String("/samples/")+newName; File s=SD.open(fb_entries[fb_sel].c_str()); File d=SD.open(np.c_str(),FILE_WRITE);
        if (s&&d){ uint8_t buf[1024]; int n; while((n=s.read(buf,1024))>0) d.write(buf,n); } if(s)s.close(); if(d)d.close();
        refreshList(); drawList();
      }
    } else if ((bi==6||bi==7) && fb_sel>=0){
      uint8_t slot=(bi==6)?0:1;
      if (loadWavToSlot(fb_entries[fb_sel].c_str(),slot)) LCD_toast("Loaded"); else LCD_toast("Load failed");
    } else {
      // sélection (tape dans la zone liste : approx via cycle boutons -> simple incrément)
      // ici, pour simplifier, touche n'importe où fait avancer la sélection
      fb_sel = (fb_sel+1 < fb_count)? (fb_sel+1) : 0; drawList();
    }
  }
}
