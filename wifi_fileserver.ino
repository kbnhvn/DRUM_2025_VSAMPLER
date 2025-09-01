#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SD.h>
extern void buildCatalog();

#include "secrets.h"
#ifdef __has_include
  #if __has_include("secrets.h")
    #include "secrets.h"
  #endif
#endif
#ifndef WIFI_HOME_SSID
  #define WIFI_HOME_SSID "your-ssid"
  #define WIFI_HOME_PSK  "your-pass"
  #define WIFI_PHONE_SSID "phone-ssid"
  #define WIFI_PHONE_PSK  "phone-pass"
#endif

WebServer server(80);
static bool fileserverOn = false;
static unsigned long lastClientActivity = 0;  // NOUVEAU: Monitoring activité
static int uploadProgress = 0;                // NOUVEAU: Progress upload

bool isFileServerOn() { return fileserverOn; }
 
// NOUVEAU: Sécurisation noms de fichiers

static String safeName(String n){
  n.replace("\\", "/"); 
  int slash = n.lastIndexOf('/'); 
  if (slash >= 0) n = n.substring(slash + 1);
  n.replace("..", "");
  n.replace("/", "");    // NOUVEAU: Empêcher path traversal
  n.replace("\\", "");   // NOUVEAU: Protection Windows paths
  n.trim(); 
  
  // NOUVEAU: Limiter longueur et caractères spéciaux
  if (n.length() > 64) n = n.substring(0, 64);
  
  // Remplacer caractères problématiques
  n.replace(" ", "_");
  n.replace("(", "_");
  n.replace(")", "_");
  
  return n;
}

static String htmlEscape(const String& s) {
  String o; 
  o.reserve(s.length() * 2);
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '\"') o += "&quot;";
    else if (c == '\'') o += "&#39;";
    else o += c;
  }
  return o;
}

// NOUVEAU: Interface web moderne avec CSS
static String getModernCSS() {
  return R"(
<style>
body { font-family: 'Segoe UI', sans-serif; background: #1a1a1a; color: #e0e0e0; margin: 0; padding: 20px; }
.container { max-width: 800px; margin: 0 auto; }
.header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 20px; border-radius: 12px; margin-bottom: 20px; }
.header h1 { margin: 0; color: white; text-shadow: 0 2px 4px rgba(0,0,0,0.3); }
.card { background: #2a2a2a; border-radius: 8px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); }
.file-list { list-style: none; padding: 0; }
.file-item { background: #333; margin: 8px 0; padding: 12px; border-radius: 6px; display: flex; justify-content: space-between; align-items: center; }
.file-item:hover { background: #404040; }
.file-name { font-weight: 500; }
.file-actions { display: flex; gap: 10px; }
.btn { padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; color: white; font-size: 14px; }
.btn-primary { background: #667eea; }
.btn-danger { background: #e74c3c; }
.btn:hover { opacity: 0.8; }
.upload-area { border: 2px dashed #555; border-radius: 8px; padding: 30px; text-align: center; margin: 20px 0; }
.upload-area.dragover { border-color: #667eea; background: #252538; }
.progress { background: #333; height: 20px; border-radius: 10px; overflow: hidden; margin: 10px 0; }
.progress-bar { background: linear-gradient(90deg, #667eea, #764ba2); height: 100%; transition: width 0.3s; }
.stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; margin: 20px 0; }
.stat-card { background: #333; padding: 15px; border-radius: 6px; text-align: center; }
.stat-value { font-size: 24px; font-weight: bold; color: #667eea; }
</style>
)";
}

void startFileServer() {
  if (fileserverOn) return;

  // NOUVEAU: Page d'accueil moderne avec statistiques
  server.on("/", HTTP_GET, []() {
    lastClientActivity = millis();
    
    // Compter les fichiers et calculer stats
    int totalFiles = 0, totalSize = 0, validFiles = 0;    File root = SD.open("/samples");
    File root = SD.open("/samples");
    
    String fileListHtml = "";
    if (root) {
      while (true) {
        File f = root.openNextFile();
        if (!f) break;
        if (!f.isDirectory()) {
          String nm = f.name(); 
          String en = htmlEscape(nm);
          int size = f.size();
          
          totalFiles++;
          totalSize += size;
          
          // Validation WAV
          bool isValid = nm.endsWith(".wav") || nm.endsWith(".WAV");
          if (isValid) validFiles++;
          
          String sizeStr = (size < 1024) ? String(size) + "B" :
                          (size < 1048576) ? String(size/1024) + "KB" :
                          String(size/1048576) + "MB";
          
          String statusIcon = isValid ? "✓" : "⚠";
          String statusColor = isValid ? "#2ecc71" : "#e74c3c";
          
          fileListHtml += "<div class='file-item'>";
          fileListHtml += "<div><span style='color:" + statusColor + "'>" + statusIcon + "</span> ";
          fileListHtml += "<span class='file-name'>" + en + "</span> <small>(" + sizeStr + ")</small></div>";
          fileListHtml += "<div class='file-actions'>";
          fileListHtml += "<a href='/dl?f=" + en + "' class='btn btn-primary'>Download</a>";
          fileListHtml += "<a href='/rm?f=" + en + "' class='btn btn-danger' onclick='return confirm(\"Delete " + en + "?\")'>Delete</a>";
          fileListHtml += "</div></div>";
        }
        f.close();
      }
      root.close();
    }
    
    String totalSizeStr = (totalSize < 1048576) ? String(totalSize/1024) + " KB" :
                         String(totalSize/1048576) + " MB";
    
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 Drum Sampler</title>";
    html += getModernCSS();
    html += "</head><body><div class='container'>";
    html += "<div class='header'><h1>🥁 ESP32 Drum Sampler</h1><p>Sample Management Interface</p></div>";
    
    // Stats dashboard
    html += "<div class='stats'>";
    html += "<div class='stat-card'><div class='stat-value'>" + String(totalFiles) + "</div><div>Total Files</div></div>";
    html += "<div class='stat-card'><div class='stat-value'>" + String(validFiles) + "</div><div>Valid Samples</div></div>";
    html += "<div class='stat-card'><div class='stat-value'>" + totalSizeStr + "</div><div>Total Size</div></div>";
    html += "<div class='stat-card'><div class='stat-value'>" + String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/1024) + "KB</div><div>Free RAM</div></div>";
    html += "</div>";
    
    // Upload area
    html += "<div class='card'><h3>Upload Samples</h3>";
    html += "<form method='POST' action='/up' enctype='multipart/form-data'>";
    html += "<div class='upload-area' ondrop='dropHandler(event)' ondragover='dragOverHandler(event)'>";
    html += "📁 Drag & drop WAV files here or <input type='file' name='file' accept='.wav,.WAV' multiple style='display:inline'>";
    html += "</div><button type='submit' class='btn btn-primary'>Upload</button></form></div>";
    
    // File management
    html += "<div class='card'><h3>Sample Library</h3>";
    if (totalFiles > 0) {
      html += "<div class='file-list'>" + fileListHtml + "</div>";
    } else {
      html += "<p>No samples found. Upload some WAV files to get started!</p>";
    }
    html += "</div>";
    
    // Tools
    html += "<div class='card'><h3>Tools</h3>";
    html += "<form method='POST' action='/rn' style='margin-bottom:15px'>";
    html += "Rename: <input name='old' placeholder='old name' style='margin:0 10px'>";
    html += "<input name='neo' placeholder='new name' style='margin:0 10px'>";
    html += "<button type='submit' class='btn btn-primary'>Rename</button></form>";
    html += "<a href='/rescan' class='btn btn-primary'>🔄 Rescan Library</a>";
    html += "</div>";
    
    // JavaScript pour drag & drop
    html += "<script>";
    html += "function dragOverHandler(ev) { ev.preventDefault(); ev.target.classList.add('dragover'); }";
    html += "function dropHandler(ev) { ev.preventDefault(); ev.target.classList.remove('dragover'); }";
    html += "</script>";
    
    html += "</div></body></html>";
    server.send(200, "text/html", html);  
  });

  // NOUVEAU: Download avec logging
  server.on("/dl", HTTP_GET, []() {
    lastClientActivity = millis();
    String f = safeName(server.arg("f"));
    File file = SD.open("/samples/"+f, FILE_READ);
    if (!file) { 
      Serial.printf("[WEB] Download failed: %s\n", f.c_str());
      server.send(404, "text/plain", "File not found"); 
      return; 
    }
    Serial.printf("[WEB] Downloading: %s (%d bytes)\n", f.c_str(), file.size());
    server.streamFile(file, "application/octet-stream"); 
    file.close();
  });

  // NOUVEAU: Delete avec confirmation et logging  
  server.on("/rm", HTTP_GET, []() {
    lastClientActivity = millis();
    String f = safeName(server.arg("f"));
    String path = "/samples/" + f;
    
    if (!SD.exists(path)) {
      server.send(404, "text/plain", "File not found");
      return;
    }
    
    bool ok = SD.remove(path);
    if (ok) { 
      Serial.printf("[WEB] Deleted: %s\n", f.c_str());
      server.sendHeader("Location", "/"); 
      server.send(302); 
    } else {
      Serial.printf("[WEB] Delete failed: %s\n", f.c_str());
      server.send(500, "text/plain", "Delete failed");
    }
  });

  // NOUVEAU: Rename avec validation
  server.on("/rn", HTTP_POST, []() {
    lastClientActivity = millis();
    String o = safeName(server.arg("old"));
    String n = safeName(server.arg("neo"));
    
    if (o.length() == 0 || n.length() == 0) { 
      server.sendHeader("Location", "/"); 
      server.send(302); 
      return; 
    }
    
    String oldPath = "/samples/" + o;
    String newPath = "/samples/" + n;
    
    if (!SD.exists(oldPath)) {
      server.send(404, "text/plain", "Original file not found");
      return;
    }
    
    if (SD.exists(newPath)) {
      server.send(409, "text/plain", "Target file already exists");
      return;
    }
    
    bool ok = SD.rename(oldPath, newPath);
    if (ok) {
      Serial.printf("[WEB] Renamed: %s -> %s\n", o.c_str(), n.c_str());
    } else {
      Serial.printf("[WEB] Rename failed: %s -> %s\n", o.c_str(), n.c_str());
    }
    
    server.sendHeader("Location", "/"); 
    server.send(302);
  });

  // NOUVEAU: Upload avec progress et validation
  const size_t MAX_UPLOAD = 50UL * 1024UL * 1024UL; // 50 MB
  server.on("/up", HTTP_POST,
    []() { 
      // Réponse avec feedback
      String html = "<!DOCTYPE html><html><head>" + getModernCSS() + "</head><body>";
      html += "<div class='container'><div class='card'>";
      html += "<h3>Upload Complete!</h3>";
      html += "<p>Files uploaded successfully. Rescanning library...</p>";
      html += "<script>setTimeout(() => window.location='/', 2000);</script>";
      html += "</div></div></body></html>";
      server.send(200, "text/html", html);
    },
    []() {
      lastClientActivity = millis();
      HTTPUpload& up = server.upload();
      static File uf; 
      static String tmpPath, finalName;
      static size_t totalUploaded = 0;

      if (up.status == UPLOAD_FILE_START) {
        finalName = safeName(up.filename);
        if (!finalName.length()) {
          Serial.println("[WEB] Upload rejected - invalid filename");
          return;
        }
        
        // Vérifier extension
        if (!finalName.endsWith(".wav") && !finalName.endsWith(".WAV")) {
          Serial.printf("[WEB] Upload rejected - not WAV: %s\n", finalName.c_str());
          return;
        }
 
        if (!SD.exists("/samples")) SD.mkdir("/samples");
        tmpPath = "/samples/.tmp." + finalName;
        uf = SD.open(tmpPath, FILE_WRITE);
        totalUploaded = 0;
        uploadProgress = 0;
        
        Serial.printf("[WEB] Upload started: %s\n", finalName.c_str());

      } else if (up.status == UPLOAD_FILE_WRITE) {
        if (uf) {
          if (totalUploaded + up.currentSize > MAX_UPLOAD) { 
            Serial.printf("[WEB] Upload too large: %u bytes\n", totalUploaded + up.currentSize);
            uf.close(); 
            SD.remove(tmpPath); 
            return; 
          }
 
          uf.write(up.buf, up.currentSize);
          totalUploaded += up.currentSize;
          uploadProgress = (totalUploaded * 100) / MAX_UPLOAD;
          
          // Log progress chaque 100KB
          if (totalUploaded % (100 * 1024) == 0) {
            Serial.printf("[WEB] Upload progress: %u KB\n", totalUploaded / 1024);
          }
        }
      } else if (up.status == UPLOAD_FILE_END) {
        if (uf) { 
          uf.close(); 
          String finalPath = "/samples/" + finalName;
          bool ok = SD.rename(tmpPath, finalPath);
          
          if (ok) {
            Serial.printf("[WEB] Upload completed: %s (%u bytes)\n", 
                         finalName.c_str(), totalUploaded);
          } else {
            Serial.printf("[WEB] Upload failed during rename: %s\n", finalName.c_str());
            SD.remove(tmpPath);
          }
        }
      }
    }
  );

  // NOUVEAU: Rescan avec feedback
  server.on("/rescan", HTTP_GET, []() { 
    lastClientActivity = millis();
    Serial.println("[WEB] Catalog rescan requested");
    buildCatalog(); 
    server.sendHeader("Location", "/"); 
    server.send(302); 
  });
  
  // NOUVEAU: API status pour monitoring
  server.on("/api/status", HTTP_GET, []() {
    lastClientActivity = millis();
    String json = "{";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"spiram_free\":" + String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)) + ",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"files_count\":" + String(totalFiles) + ",";
    json += "\"last_activity\":" + String(lastClientActivity);
    json += "}";
    server.send(200, "application/json", json);
  });
  
  server.begin(); 
  fileserverOn = true;
  lastClientActivity = millis();
  Serial.println("[WEB] File server started with modern interface");
 }

void stopFileServer() { 
  if (!fileserverOn) return; 
  server.stop(); 
  fileserverOn = false;
  Serial.println("[WEB] File server stopped");
}

void loopWeb() { 
  if (fileserverOn) {
    server.handleClient(); 
    
    // NOUVEAU: Auto-stop après inactivité (optionnel)
    // if (millis() - lastClientActivity > 3600000) { // 1 heure
    //   Serial.println("[WEB] Auto-stopping server after inactivity");
    //   stopFileServer();
    // }
  }
}

// NOUVEAU: Connexion WiFi avec retry et feedback
static void _wifiConnect(const char* ssid, const char* psk) {
  WiFi.disconnect(true, true); 
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, psk);
  Serial.print("WiFi connecting...");
  
  // Attente avec timeout
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected to %s\n", ssid);
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] Signal: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.printf("\n[WiFi] Connection failed to %s\n", ssid);
  }
}

void wifiConnectHome() { 
  _wifiConnect(WIFI_HOME_SSID, WIFI_HOME_PSK); 
  if (WiFi.status() == WL_CONNECTED) {
    startFileServer(); 
  }
}

void wifiConnectPhone() { 
  _wifiConnect(WIFI_PHONE_SSID, WIFI_PHONE_PSK); 
  if (WiFi.status() == WL_CONNECTED) {
    startFileServer();
  }
}

void stopWiFiAutoReconnect() {
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true, true);
  Serial.println("[WiFi] Auto-reconnect disabled");
}
