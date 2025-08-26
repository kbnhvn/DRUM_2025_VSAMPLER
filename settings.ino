#include <SD.h>
#include <ArduinoJson.h>

extern void AudioOut_setRoute(AudioOut r);

extern void LCD_clearWorkArea(); extern void LCD_drawTitle(const char*);
extern void LCD_drawButtonsRow(std::initializer_list<const char*>);
extern void LCD_drawSmallText(int,int,const char*); extern void LCD_toast(const char*);
extern int LCD_buttonIndex(int); extern uint8_t trigger_on[48];

extern void AudioOut_setVolume(uint8_t);
extern uint8_t g_midiChannel;
extern void WIFI_enter(); // si besoin

static bool g_wifiEnable=false;
static uint8_t g_audioVolume=80;  // 0..100
static uint8_t g_audioRoute=0;    // 0=JACK 1=SPK

static void Settings_save(){
  DynamicJsonDocument d(256);
  File f=SD.open("/config/settings.json",FILE_READ);
  if (f){ deserializeJson(d,f); f.close(); }
  d["wifi_enable"]=g_wifiEnable;
  d["audio_volume"]=g_audioVolume;
  d["audio_route"]=g_audioRoute;
  if (!SD.exists("/config")) SD.mkdir("/config");
  File o=SD.open("/config/settings.json",FILE_WRITE); if(!o) return; serializeJson(d,o); o.close();
}
static void Settings_load(){
  File f=SD.open("/config/settings.json",FILE_READ);
  if (!f) return;
  DynamicJsonDocument d(256); if (deserializeJson(d,f)) { f.close(); return; } f.close();
  g_wifiEnable = d["wifi_enable"]|false;
  g_audioVolume= d["audio_volume"]|80;
  g_audioRoute = d["audio_route"]|0;
}

void Settings_enter(){
  Settings_load();
  LCD_clearWorkArea(); LCD_drawTitle("SETTINGS   [BACK]");
  LCD_drawButtonsRow({" WIFI "," SAVE "," VOL- "," VOL+ "});
  LCD_drawButtonsRow({" JACK ","  SPK ","     ","     "});
  char line[64]; sprintf(line,"MIDI CH:%u  VOL:%u  OUT:%s", g_midiChannel, g_audioVolume, g_audioRoute?"SPK":"JACK");
  LCD_drawSmallText(10,120,line);
  AudioOut_setRoute(g_audioRoute); AudioOut_setVolume(g_audioVolume);
}
void Settings_loop(){
  for(int i=0;i<48;i++){
    if(!trigger_on[i]) continue; trigger_on[i]=0;
    int bi=LCD_buttonIndex(i);
    if (bi==0){ g_wifiEnable=!g_wifiEnable; LCD_toast(g_wifiEnable?"WiFi ON":"WiFi OFF"); }
    if (bi==1){ Settings_save(); LCD_toast("Saved"); }
    if (bi==2){ if(g_audioVolume>0) g_audioVolume--; AudioOut_setVolume(g_audioVolume); }
    if (bi==3){ if(g_audioVolume<100) g_audioVolume++; AudioOut_setVolume(g_audioVolume); }
    if (bi==4){ g_audioRoute=0; AudioOut_setRoute(g_audioRoute); }
    if (bi==5){ g_audioRoute=1; AudioOut_setRoute(g_audioRoute); }
    char line[64]; sprintf(line,"MIDI CH:%u  VOL:%u  OUT:%s", g_midiChannel, g_audioVolume, g_audioRoute?"SPK":"JACK");
    LCD_drawSmallText(10,120,line);
  }
}
