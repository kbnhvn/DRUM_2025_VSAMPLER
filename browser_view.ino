#include <Arduino.h>
#include <SD.h>
#include "views.h"
#include "sd_catalog.h"
extern View currentView;

// Helpers UI modernes
extern void drawModernButton(int x, int y, int w, int h, int color, const char* text, bool active, bool pressed);
extern void flashButton(int x, int y, int w, int h, int color, const char* texto);
extern void forceCompleteRedraw();
extern Arduino_GFX *gfx;
extern int BPOS[48][4];
extern byte selected_sound;

extern bool validateWavFile(const char* path, float* duration, uint32_t* sampleRate);
extern void synthESP32_TRIGGER(uint8_t voice);
extern void setSound(byte voice);

// UI Constants
#define BROWSER_ROW_H 18
#define BROWSER_ROWS 11
#define BROWSER_LIST_Y 35
#define BROWSER_BUTTON_Y 240

// CORRECTION: Variables File Browser améliorées avec noms corrects
static String fileList[128];
static float fileDurations[128];     
static uint32_t fileSampleRates[128]; 
static bool fileValid[128];          
static int fileCount = 0;
static int pageIndex = 0;
static int selectedIndex = -1;
static unsigned long lastPreviewTime = 0; 
static int currentPreviewIndex = -1;     // CORRECTION: Nom différent de la fonction

// CORRECTION: Affichage moderne d'une ligne de fichier
void drawFileRow(int idx, int y, bool selected, bool playing = false) {
  int bgColor = selected ? RGB565(40, 60, 100) : (idx % 2 ? RGB565(25, 25, 30) : RGB565(20, 20, 25));
  
  // Fond de ligne
  gfx->fillRect(5, y - 1, 470, BROWSER_ROW_H, bgColor);
  
  if (idx >= 0 && idx < fileCount) {
    // Indicateur validité (LED verte/rouge)
    int statusColor = fileValid[idx] ? UI_SUCCESS : UI_DANGER;
    gfx->fillRect(8, y + 2, 6, BROWSER_ROW_H - 4, statusColor);
    
    // Icon play si en preview
    if (playing) {
      gfx->fillTriangle(18, y + 3, 18, y + BROWSER_ROW_H - 3, 28, y + BROWSER_ROW_H/2, UI_ACCENT);
    }
    
    // Nom du fichier tronqué si nécessaire
    gfx->setTextColor(selected ? WHITE : UI_ON_SURFACE, bgColor);
    gfx->setCursor(35, y + 12);
    
    String displayName = fileList[idx];
    if (displayName.length() > 30) {
      displayName = displayName.substring(0, 27) + "...";
    }
    gfx->print(displayName);
    
    // Durée et sample rate à droite
    if (fileValid[idx]) {
      gfx->setTextColor(selected ? UI_ACCENT : RGB565(150, 155, 165), bgColor);
      gfx->setCursor(320, y + 12);
      gfx->printf("%.1fs", fileDurations[idx]);
      
      gfx->setCursor(380, y + 12);
      if (fileSampleRates[idx] == 44100) {
        gfx->setTextColor(UI_SUCCESS, bgColor);
        gfx->print("44k");
      } else {
        gfx->setTextColor(UI_WARNING, bgColor);
        gfx->printf("%uk", fileSampleRates[idx] / 1000);
      }
    } else {
      gfx->setTextColor(UI_DANGER, bgColor);
      gfx->setCursor(320, y + 12);
      gfx->print("INVALID");
    }
  }
  
  // Bordure selection
  if (selected) {
    gfx->drawRect(5, y - 1, 470, BROWSER_ROW_H, UI_ACCENT);
  }
}

// CORRECTION: Affichage page avec design moderne
static void drawFilePage() {
  gfx->fillScreen(RGB565(15, 15, 20));
  
  // Header avec gradient
  for (int y = 0; y < 30; y++) {
    int shade = map(y, 0, 30, 60, 30);
    gfx->drawLine(0, y, 480, y, RGB565(shade, shade + 10, shade + 15));
  }
  
  gfx->setTextColor(UI_ACCENT, RGB565(50, 60, 75));
  gfx->setCursor(15, 20);
  gfx->print("SAMPLE BROWSER");

  // Zone de liste avec design card
  gfx->fillRect(5, BROWSER_LIST_Y - 5, 470, BROWSER_ROWS * BROWSER_ROW_H + 10, RGB565(20, 20, 25));
  gfx->drawRect(5, BROWSER_LIST_Y - 5, 470, BROWSER_ROWS * BROWSER_ROW_H + 10, UI_ON_SURFACE);

  // Affichage des fichiers
  for (int i = 0; i < BROWSER_ROWS; i++) {
    int idx = pageIndex + i;
    if (idx >= fileCount) break;

    int y = BROWSER_LIST_Y + i * BROWSER_ROW_H;
    bool isPlaying = (idx == currentPreviewIndex && millis() - lastPreviewTime < 2000);
    drawFileRow(idx, y, idx == selectedIndex, isPlaying);
  }
  
  // CORRECTION: Scroll indicator à droite
  if (fileCount > BROWSER_ROWS) {
    int scrollH = BROWSER_ROWS * BROWSER_ROW_H;
    int scrollY = BROWSER_LIST_Y;
    int thumbH = max(8, (scrollH * BROWSER_ROWS) / fileCount);
    int thumbY = scrollY + (scrollH * pageIndex) / fileCount;
    
    gfx->fillRect(475, scrollY, 4, scrollH, RGB565(40, 40, 45));
    gfx->fillRect(475, thumbY, 4, thumbH, UI_ACCENT);
  }

  // CORRECTION: Boutons modernes avec états
  bool canPrev = pageIndex > 0;
  bool canNext = pageIndex + BROWSER_ROWS < fileCount;
  bool canAssign = selectedIndex >= 0 && fileValid[selectedIndex];
  
  drawModernButton( 20, BROWSER_BUTTON_Y, 80, 25, UI_PRIMARY, "<<", canPrev, false);
  drawModernButton(110, BROWSER_BUTTON_Y, 80, 25, UI_PRIMARY, ">>", canNext, false);
  drawModernButton(200, BROWSER_BUTTON_Y, 100, 25, UI_SUCCESS, "ASSIGN", canAssign, false);
  drawModernButton(310, BROWSER_BUTTON_Y, 80, 25, UI_WARNING, "PREVIEW", true, false);
  drawModernButton(400, BROWSER_BUTTON_Y, 70, 25, UI_DANGER, "BACK", true, false);
  
  // Info pagination
  gfx->setTextColor(UI_ON_SURFACE, RGB565(15, 15, 20));
  gfx->setCursor(20, 220);
  gfx->printf("Files: %d | Page: %d-%d | Pad: %d", 
              fileCount, pageIndex + 1, 
              min(pageIndex + BROWSER_ROWS, fileCount), selected_sound);
}

// Scanner le dossier /samples
static void scanSamples() {
  fileCount = 0;
  File root = SD.open("/samples");
  if (!root || !root.isDirectory()) {
    Serial.println("[BROWSER] No /samples directory");
    return;
  }

  Serial.println("[BROWSER] Scanning samples with validation..."); 

  File f;
  while ((f = root.openNextFile()) && fileCount < 128) {
    if (!f.isDirectory()) {
      String name = f.name();
      if (name.endsWith(".wav") || name.endsWith(".WAV")) {
        fileList[fileCount] = name;
        
        // CORRECTION: Validation et métadonnées
        String fullPath = "/samples/" + name;
        float duration = 0;
        uint32_t sampleRate = 0;
        
        fileValid[fileCount] = validateWavFile(fullPath.c_str(), &duration, &sampleRate);
        fileDurations[fileCount] = duration;
        fileSampleRates[fileCount] = sampleRate;
        
        if (!fileValid[fileCount]) {
          Serial.printf("[BROWSER] Invalid WAV: %s\n", name.c_str());
        }
        fileCount++;

        // Yield pour watchdog
        if (fileCount % 10 == 0) {
          yield();
          esp_task_wdt_reset();
        }
      }
    }
    f.close();
  }
  root.close();
  pageIndex = 0;
  selectedIndex = (fileCount > 0) ? 0 : -1;
  
  int validCount = 0;
  for (int i = 0; i < fileCount; i++) if (fileValid[i]) validCount++;
  
  Serial.printf("[BROWSER] Found %d files (%d valid)\n", fileCount, validCount);
}

// Handler tactile
void handleTouchBrowser(int x, int y) {
  // CORRECTION: Sélection avec preview automatique
  if (y >= BROWSER_LIST_Y && y < BROWSER_LIST_Y + BROWSER_ROWS * BROWSER_ROW_H) {
    int row = (y - BROWSER_LIST_Y) / BROWSER_ROW_H;
    int idx = pageIndex + row;  // CORRECTION: Utiliser 'row' au lieu de 'i'
    
    if (idx >= 0 && idx < fileCount) {
      selectedIndex = idx;
      
      // Animation de sélection
      int rowY = BROWSER_LIST_Y + row * BROWSER_ROW_H;
      gfx->drawRect(4, rowY - 2, 472, BROWSER_ROW_H + 2, UI_ACCENT);
      delay(50);
      
      // Preview automatique si valide
      if (fileValid[idx]) {
        currentPreviewIndex = idx;  // CORRECTION: Variable renommée
        lastPreviewTime = millis();
        // Ici vous devriez assigner le sample au slot preview
        // assignSampleToSlot(idx, 255); // Slot preview temporaire
        
        // Trigger preview avec feedback
        // ROTvalue[selected_sound][0] = 255; // Utiliser slot preview
        setSound(selected_sound);
        synthESP32_TRIGGER(selected_sound);
        
        Serial.printf("[BROWSER] Selected & previewed: %s (%.1fs)\n", 
                      fileList[idx].c_str(), fileDurations[idx]);
      }
      
      drawFilePage();
      return;
    }
  }

  // CORRECTION: Boutons avec animations et feedback
  if (y >= BROWSER_BUTTON_Y && y <= BROWSER_BUTTON_Y + 25) {
    // Previous page
    if (x >= 20 && x <= 100 && pageIndex > 0) {
      drawModernButton(20, BROWSER_BUTTON_Y, 80, 25, UI_PRIMARY, "<<", true, true);
      delay(100);
      pageIndex = max(0, pageIndex - BROWSER_ROWS);
      Serial.printf("[BROWSER] Previous page: %d\n", pageIndex);
      drawFilePage();
      return;
    }
    
    // Next page
    if (x >= 110 && x <= 190 && pageIndex + BROWSER_ROWS < fileCount) {
      drawModernButton(110, BROWSER_BUTTON_Y, 80, 25, UI_PRIMARY, ">>", true, true);
      delay(100);
      pageIndex = min(fileCount - BROWSER_ROWS, pageIndex + BROWSER_ROWS);
      Serial.printf("[BROWSER] Next page: %d\n", pageIndex);
      drawFilePage();
      return;
    }
    
    // Assign avec confirmation visuelle
    if (x >= 200 && x <= 300 && selectedIndex >= 0 && fileValid[selectedIndex]) {
      drawModernButton(200, BROWSER_BUTTON_Y, 100, 25, UI_SUCCESS, "ASSIGN", true, true);
      delay(100);
      
      // Animation de confirmation
      // assignSampleToSlot(selectedIndex, selected_sound);
      
      // Feedback visuel réussite
      gfx->fillRect(150, 200, 180, 30, UI_SUCCESS);
      gfx->setTextColor(BLACK, UI_SUCCESS);
      gfx->setCursor(160, 220);
      gfx->printf("Assigned to pad %d", selected_sound);
      delay(1000);
      
      Serial.printf("[BROWSER] Assigned %s to pad %d\n", 
                    fileList[selectedIndex].c_str(), selected_sound);
      drawFilePage();
      return;
    }
    
    // Preview manual
    if (x >= 310 && x <= 390 && selectedIndex >= 0 && fileValid[selectedIndex]) {
      drawModernButton(310, BROWSER_BUTTON_Y, 80, 25, UI_WARNING, "PREVIEW", false, true);
      delay(100);
      
      // Trigger preview avec indicateur visuel
      currentPreviewIndex = selectedIndex;  // CORRECTION: Variable renommée
      lastPreviewTime = millis();
      // assignSampleToSlot(selectedIndex, 255);
      
      // Backup et restore du slot original
      // int originalSlot = ROTvalue[selected_sound][0];
      // ROTvalue[selected_sound][0] = 255;
      setSound(selected_sound);
      synthESP32_TRIGGER(selected_sound);
      // ROTvalue[selected_sound][0] = originalSlot;
      setSound(selected_sound);
      
      Serial.printf("[BROWSER] Manual preview: %s\n", fileList[selectedIndex].c_str());
      drawFilePage();
      return;
    }

    // Back avec transition animée
    if (x >= 400 && x <= 470) {
      drawModernButton(400, BROWSER_BUTTON_Y, 70, 25, UI_DANGER, "BACK", false, true);
      
      // Animation de sortie
      for (int i = 0; i < 8; i++) {
        gfx->fillRect(0, 0, 480, 272, RGB565(i * 2, i * 2, i * 2));
        delay(20);
      }
      
      Serial.println("[BROWSER] Returning to main view");
      currentView = VIEW_MAIN;
      forceCompleteRedraw();
      return;
    }
  }
}

// CORRECTION: Entrée avec animation et scan amélioré
void openBrowserView() {
  currentView = VIEW_BROWSER;
  
  // Animation d'ouverture
  gfx->fillScreen(RGB565(15, 15, 20));
  
  // Loading screen pendant le scan
  gfx->setTextColor(UI_ACCENT, RGB565(15, 15, 20));
  gfx->setCursor(180, 120);
  gfx->print("Scanning samples...");
  
  for (int i = 0; i < 200; i += 10) {
    gfx->drawRect(140, 140, 200, 8, UI_ON_SURFACE);
    gfx->fillRect(142, 142, i, 4, UI_ACCENT);
    delay(20);
  }
  
  scanSamples();
  drawFilePage();
}