#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SD.h>
#include "secrets_rename.h"
extern void buildCatalog();

WebServer server(80);
static bool fileserverOn=false;

static String safeName(String n){
  n.replace("\\","/"); int slash=n.lastIndexOf('/'); if (slash>=0) n=n.substring(slash+1);
  n.replace("..",""); n.trim(); return n;
}

void startFileServer(){
  if (fileserverOn) return;

  server.on("/", HTTP_GET, [](){
    String html = "<html><body><h3>/samples</h3><ul>";
    File root = SD.open("/samples");
    if (root){
      while (true){
        File f=root.openNextFile();
        if (!f) break;
        if (!f.isDirectory()){
          String nm=f.name();
          html += "<li>"+nm+" "
                  "<a href='/dl?f="+nm+"'>download</a> "
                  "<a href='/rm?f="+nm+"'>delete</a></li>";
        }
        f.close();
      }
    }
    html += "</ul><hr><form method='POST' action='/up' enctype='multipart/form-data'>"
            "<input type='file' name='file'><input type='submit' value='Upload'></form>"
            "<hr><form method='POST' action='/rn'>Rename: <input name='old'><input name='neo'>"
            "<input type='submit' value='OK'></form>"
            "<hr><a href='/rescan'>Rescan</a>"
            "</body></html>";
    server.send(200,"text/html",html);
  });

  server.on("/dl", HTTP_GET, [](){
    String f = safeName(server.arg("f"));
    File file = SD.open("/samples/"+f, FILE_READ);
    if (!file){ server.send(404,"text/plain","not found"); return; }
    server.streamFile(file, "application/octet-stream"); file.close();
  });

  server.on("/rm", HTTP_GET, [](){
    String f = safeName(server.arg("f"));
    bool ok = SD.remove(String("/samples/")+f);
    if (ok) { server.sendHeader("Location","/"); server.send(302); }
    else server.send(404,"text/plain","rm fail");
  });

  server.on("/rn", HTTP_POST, [](){
    String o = safeName(server.arg("old"));
    String n = safeName(server.arg("neo"));
    if (o.length()==0 || n.length()==0){ server.sendHeader("Location","/"); server.send(302); return; }
    SD.rename(String("/samples/")+o, String("/samples/")+n);
    server.sendHeader("Location","/"); server.send(302);
  });

  server.on("/up", HTTP_POST,
    [](){ server.sendHeader("Location","/rescan"); server.send(303); },
    [](){
      HTTPUpload& up = server.upload();
      static File uf; static String tmpPath, finalName;
      if (up.status == UPLOAD_FILE_START) {
        finalName = safeName(up.filename);
        if (!finalName.length()) return;
        if (!SD.exists("/samples")) SD.mkdir("/samples");
        tmpPath = String("/samples/.tmp.")+finalName;
        uf = SD.open(tmpPath, FILE_WRITE);
      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (uf) uf.write(up.buf, up.currentSize);
      } else if (up.status == UPLOAD_FILE_END) {
        if (uf) { uf.close(); SD.rename(tmpPath, String("/samples/")+finalName); }
      }
    }
  );

  server.on("/rescan", HTTP_GET, [](){ buildCatalog(); server.sendHeader("Location","/"); server.send(302); });
  server.begin(); fileserverOn=true;
}

void stopFileServer(){ if (!fileserverOn) return; server.stop(); fileserverOn=false; }
void loopWeb(){ if (fileserverOn) server.handleClient(); }

static void _wifiConnect(const char* ssid, const char* psk){
  WiFi.disconnect(true,true); WiFi.mode(WIFI_STA); WiFi.begin(ssid, psk);
  Serial.print("WiFi connecting...");
}
void wifiConnectHome(){ _wifiConnect(WIFI_HOME_SSID, WIFI_HOME_PSK); startFileServer(); }
void wifiConnectPhone(){ _wifiConnect(WIFI_PHONE_SSID, WIFI_PHONE_PSK); startFileServer(); }
