#include <Arduino.h>
#include <SD.h>
#include "views.h"
extern View currentView;
// helpers rendus par LCD_tools.ino
extern void drawBT(byte bt, int color, String texto);
extern Arduino_GFX *gfx;
extern int BPOS[48][4];
// assign depuis SD
void assignSampleToSlot(int catIndex, int slot);
extern byte selected_sound;

// Variables File Browser
static String fileList[128];   // max 128 fichiers
static int fileCount = 0;
static int pageIndex = 0;      // index du fichier en haut de page
static int selectedIndex = -1; // fichier sélectionné

// Afficher la page courante
static void drawFilePage() {
  gfx->fillRect(0, 0, 480, 272, BLACK);

  gfx->setTextColor(WHITE, BLACK);
  gfx->setCursor(10, 10);
  gfx->print("File Browser - /samples");

  // Liste 10 fichiers max
  for (int i = 0; i < 10; i++) {
    int idx = pageIndex + i;
    if (idx >= fileCount) break;

    int y = 30 + i * 20;
    if (idx == selectedIndex) {
      gfx->setTextColor(BLACK, GREEN); // highlight
      gfx->fillRect(5, y - 2, 470, 18, GREEN);
    } else {
      gfx->setTextColor(WHITE, BLACK);
    }
    gfx->setCursor(10, y);
    gfx->print(fileList[idx]);
  }

  // Boutons en bas
  drawBT(24, DARKGREY, "Prev");
  drawBT(25, DARKGREY, "Next");
  drawBT(26, DARKGREY, "Assign");
  drawBT(27, ZRED,     "Back");
}

// Scanner le dossier /samples
static void scanSamples() {
  fileCount = 0;
  File root = SD.open("/samples");
  if (!root || !root.isDirectory()) {
    Serial.println("No /samples directory");
    return;
  }

  File f;
  while ((f = root.openNextFile())) {
    if (!f.isDirectory()) {
      String name = f.name();
      if (name.endsWith(".wav") || name.endsWith(".WAV")) {
        fileList[fileCount++] = name;
        if (fileCount >= 128) break;
      }
    }
    f.close();
  }
  root.close();
  pageIndex = 0;
  selectedIndex = (fileCount > 0) ? 0 : -1;
}

// Handler tactile
void handleTouchBrowser(int x, int y) {
  // Sélection dans la liste
  for (int i = 0; i < 10; i++) {
    int idx = pageIndex + i;
    if (idx >= fileCount) break;

    int bx = 0, by = 30 + i * 20, bw = 480, bh = 20;
    if (x >= bx && x < bx + bw && y >= by && y < by + bh) {
      selectedIndex = idx;
      Serial.printf("Selected %s\n", fileList[idx].c_str());

      // Preview non destructif → slot tampon 255
      assignSampleToSlot(selectedIndex, 255);
      drawFilePage();
      return;
    }
  }

  // Boutons
  if (x >= BPOS[24][0] && y >= BPOS[24][1]) { // Prev
    if (pageIndex >= 10) pageIndex -= 10;
    drawFilePage();
  } else if (x >= BPOS[25][0] && y >= BPOS[25][1]) { // Next
    if (pageIndex + 10 < fileCount) pageIndex += 10;
    drawFilePage();
  } else if (x >= BPOS[26][0] && y >= BPOS[26][1]) { // Assign
    if (selectedIndex >= 0) {
      assignSampleToSlot(selectedIndex, selected_sound);
      Serial.printf("Assigned %s to pad %d\n", fileList[selectedIndex].c_str(), selected_sound);
    }
  } else if (x >= BPOS[27][0] && y >= BPOS[27][1]) { currentView = VIEW_MAIN; return; }
}

// Entrée dans la vue
void openBrowserView() {
  currentView = VIEW_BROWSER;
  scanSamples();
  drawFilePage();
}
