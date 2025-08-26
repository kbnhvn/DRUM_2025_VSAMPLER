#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

extern void LCD_clearWorkArea(); extern void LCD_drawTitle(const char*);
extern void LCD_drawButtonsRow(std::initializer_list<const char*>);
extern void LCD_drawSmallText(int,int,const char*); extern void LCD_toast(const char*);
extern int LCD_buttonIndex(int); extern uint8_t trigger_on[48];
extern bool keyboardPrompt(char*,size_t,const char*);

static bool wifiRunning=false; static WebServer server(80);
static String wifi_ssids[20]; static int wifi_count=0;

static void http_list(){
  String html="<html><body><h3>/samples</h3><ul>";
  File dir=SD.open("/samples"); File e;
  while((e=dir.openNextFile())){ if(!e.isDirectory()){ html+="<li><a href=\"/dl?f=/samples/"; html+=e.name(); html+="\">"; html+=e.name(); html+="</a></li>"; } e.close(); }
  dir.close();
  html += "</ul><hr><form method=POST enctype='multipart/form-data' action='/up'>"
          "<input type=file name='data'><input type=submit value='Upload'></form></body></html>";
  server.send(200,"text/html",html);
}
static void http_dl(){ String f=server.hasArg("f")?server.arg("f"):"";
  if(!f.length()){ server.send(400,"text/plain","missing f"); return; }
  File file=SD.open(f.c_str()); if(!file){ server.send(404,"text/plain","not found"); return; }
  server.streamFile(file,"application/octet-stream"); file.close(); }
static void http_up(){
  HTTPUpload& up=server.upload();
  if(up.status==UPLOAD_FILE_START){ File f=SD.open((String("/samples/")+up.filename).c_str(),FILE_WRITE); if(f) f.close(); }
  else if(up.status==UPLOAD_FILE_WRITE){ File f=SD.open((String("/samples/")+up.filename).c_str(),FILE_APPEND); if(f){ f.write(up.buf, up.currentSize); f.close(); } }
  else if(up.status==UPLOAD_FILE_END){ server.send(200,"text/plain","ok"); }
}
static void startServer(){
  server.on("/", http_list); server.on("/dl", http_dl); server.on("/up", HTTP_POST, [](){ server.send(200); }, http_up); server.begin();
}

void WIFI_enter(){
  LCD_clearWorkArea(); LCD_drawTitle("WIFI   [BACK]");
  LCD_drawButtonsRow({" SCAN "," CONNECT"," SERVER","  OFF "});
  LCD_drawSmallText(10,120, wifiRunning? "WiFi: ON":"WiFi: OFF");
}
void WIFI_loop(){
  for(int i=0;i<48;i++){
    if(!trigger_on[i]) continue; trigger_on[i]=0;
    int bi=LCD_buttonIndex(i);
    if (bi==0){ WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(100);
      int n=WiFi.scanNetworks(); wifi_count=min(n,20); for(int k=0;k<wifi_count;k++) wifi_ssids[k]=WiFi.SSID(k);
      LCD_toast("Scan done"); }
    else if (bi==1){ char ssid[33]=""; char pass[65]="";
      if(!keyboardPrompt(ssid,sizeof(ssid),"SSID")) continue;
      if(!keyboardPrompt(pass,sizeof(pass),"Password")) strcpy(pass,"");
      WiFi.mode(WIFI_STA); WiFi.begin(ssid,pass);
      uint32_t t0=millis(); while(WiFi.status()!=WL_CONNECTED && millis()-t0<8000) delay(100);
      if(WiFi.status()==WL_CONNECTED){ LCD_toast(WiFi.localIP().toString().c_str()); if(!wifiRunning){ startServer(); wifiRunning=true; } }
      else LCD_toast("Conn failed");
    }
    else if (bi==2){ if(!wifiRunning){ if(WiFi.status()==WL_CONNECTED){ startServer(); wifiRunning=true; LCD_toast("Server ON"); } else LCD_toast("Not connected"); }
                     else { server.stop(); wifiRunning=false; LCD_toast("Server OFF"); } }
    else if (bi==3){ if(wifiRunning){ server.stop(); wifiRunning=false; } WiFi.disconnect(true,true); WiFi.mode(WIFI_OFF); LCD_toast("WiFi OFF"); }
  }
  if (wifiRunning) server.handleClient();
}

// à appeler quand une page audio est active pour éviter les baisses de perfs :
void WIFI_pause(){ if(wifiRunning){ server.stop(); } WiFi.mode(WIFI_OFF); }
void WIFI_resume(){ /* relance optionnelle si besoin */ }
