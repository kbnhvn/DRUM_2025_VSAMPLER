#include <Arduino.h>
#include "sd_catalog.h"
#include "views.h"

// UI moderne
extern void drawModernButton(int x, int y, int w, int h, int color, const char* text, bool active, bool pressed);
extern void flashButton(int x, int y, int w, int h, int color, const char* texto);
extern void forceCompleteRedraw();
extern Arduino_GFX *gfx;
#include "synth_api.h"

extern View currentView;

#ifndef BANK_SIZE
#define BANK_SIZE 256
#endif

extern byte    selected_sound;
extern int32_t ROTvalue[16][8];
extern void    synthESP32_TRIGGER(unsigned char voice);
extern void    setSound(byte voice);

// CORRECTION: Variables avec noms cohérents (éviter conflit avec fonction)
static int currentPreviewIdx = -1;        // CORRECTION: Nom différent de la fonction
static unsigned long lastPreviewTime = 0; // Gardé cohérent

static inline int tempSlot() { return 255; } // slot tampon de preview

// UI Layout moderne
static int listTopY = 40;
static int rowH = 20;
static int rowsVis = 9;     // Lignes visibles optimisées
static int buttonAreaY = 220;
static int scrollIx = 0;    // index de la première ligne affichée
static int selIx    = -1;   // dernier index cliqué (pour ASSIGN)
static unsigned long lastScrollTime = 0;
static bool smoothScrolling = false;

// helpers
static inline float secondsOf(const SampleMeta& m) {
  return sampleSeconds(m);
}

// Affichage moderne d'une ligne avec métadonnées
static void drawRow(int listIndex, int y, bool selected, bool isPreview = false) {
  const SampleMeta& m = CATALOG[listIndex];
  // Couleur de fond alternée avec sélection
  int bgColor;
  if (selected) {
    bgColor = RGB565(40, 80, 120);
  } else if (isPreview) {
    bgColor = RGB565(80, 40, 120); // Violet pour preview
  } else {
    bgColor = (listIndex % 2) ? RGB565(25, 28, 35) : RGB565(20, 23, 30);
  }
  
  gfx->fillRect(10, y - 1, 460, rowH, bgColor);
  
  // Bordures modernes
  if (selected) {
    gfx->drawRect(9, y - 2, 462, rowH + 2, UI_ACCENT);
    gfx->drawRect(10, y - 1, 460, rowH, UI_PRIMARY);
  }
  
  // Icône état du fichier
  const SampleMeta& meta = CATALOG[listIndex];
  bool isValid = (meta.len > 0); // Validation basique
  
  if (isValid) {
    gfx->fillCircle(20, y + rowH/2, 3, UI_SUCCESS);
  } else {
    gfx->fillCircle(20, y + rowH/2, 3, UI_DANGER);
  }
  
  // Icon play si preview actif
  if (isPreview) {
    gfx->fillTriangle(30, y + 4, 30, y + rowH - 4, 42, y + rowH/2, UI_ACCENT);
  }
  
  // Nom du fichier avec troncature intelligente
  gfx->setTextColor(selected ? WHITE : UI_ON_SURFACE, bgColor);
  gfx->setCursor(50, y + 14);
  
  String displayName = m.name;
  if (displayName.length() > 20) {
    displayName = displayName.substring(0, 17) + "...";
  }
  gfx->print(displayName);
  
  // Métadonnées à droite
  if (isValid) {
    // Durée
    gfx->setTextColor(selected ? UI_ACCENT : RGB565(140, 145, 155), bgColor);
    gfx->setCursor(300, y + 14);
    float dur = secondsOf(m);
    if (dur < 10) {
      gfx->printf("%.2fs", dur);
    } else {
      gfx->printf("%.1fs", dur);
    }
    
    // Indicateur qualité sample rate
    gfx->setCursor(360, y + 14);
    if (m.rate == 44100) {
      gfx->setTextColor(UI_SUCCESS, bgColor);
      gfx->print("CD");
    } else if (m.rate == 48000) {
      gfx->setTextColor(UI_SUCCESS, bgColor);
      gfx->print("HD");
    } else if (m.rate > 44100) {
      gfx->setTextColor(UI_WARNING, bgColor);
      gfx->print("HI");
    } else {
      gfx->setTextColor(UI_WARNING, bgColor);
      gfx->print("LO");
    }
    
    // Taille approximative
    gfx->setCursor(390, y + 14);
    gfx->setTextColor(selected ? RGB565(180, 185, 195) : RGB565(120, 125, 135), bgColor);
    float sizeMB = (m.len * 2.0f) / (1024.0f * 1024.0f);
    if (sizeMB < 1.0f) {
      gfx->printf("%.0fK", (m.len * 2.0f) / 1024.0f);
    } else {
      gfx->printf("%.1fM", sizeMB);
    }
  } else {
    gfx->setTextColor(UI_DANGER, bgColor);
    gfx->setCursor(300, y + 14);
    gfx->print("INVALID");
  }
}

// Interface moderne complète
static void redrawPickerList() {
  gfx->fillScreen(RGB565(15, 15, 20));
  
  // Header avec gradient et info contexte
  for (int y = 0; y < 35; y++) {
    int shade = map(y, 0, 35, 80, 25);
    gfx->drawLine(0, y, 480, y, RGB565(shade, shade + 10, shade + 20));
  }
  
  gfx->setTextColor(UI_ACCENT, RGB565(60, 70, 90));
  gfx->setCursor(15, 22);
  gfx->printf("SAMPLE PICKER - Pad %d", selected_sound);
  
  // Zone liste avec design moderne
  gfx->fillRect(8, listTopY - 3, 464, rowsVis * rowH + 6, RGB565(18, 21, 28));
  gfx->drawRect(8, listTopY - 3, 464, rowsVis * rowH + 6, UI_ON_SURFACE);
  
  // Affichage des lignes
  int y = listTopY;
  for (int i = 0; i < rowsVis; ++i) {
    int ix = scrollIx + i;
    if (ix >= 0 && ix < (int)CATALOG.size()) {
      // CORRECTION: Utiliser currentPreviewIdx au lieu de previewIndex
      bool isPreview = (ix == currentPreviewIdx && millis() - lastPreviewTime < 3000);
      drawRow(ix, y, ix == selIx, isPreview);
      y += rowH;
    }
  }
  
  // Scroll bar moderne à droite
  if ((int)CATALOG.size() > rowsVis) {
    int scrollBarH = rowsVis * rowH;
    int thumbH = max(12, (scrollBarH * rowsVis) / (int)CATALOG.size());
    int thumbY = listTopY + (scrollBarH * scrollIx) / (int)CATALOG.size();
    
    // Rail de scroll
    gfx->fillRect(475, listTopY, 4, scrollBarH, RGB565(40, 45, 50));
    // Thumb
    gfx->fillRect(475, thumbY, 4, thumbH, UI_ACCENT);
    gfx->fillRect(474, thumbY + 1, 6, thumbH - 2, UI_PRIMARY);
  }
  
  // Buttons avec états intelligents
  bool canScrollUp = scrollIx > 0;
  bool canScrollDown = scrollIx + rowsVis < (int)CATALOG.size();
  bool canAssign = selIx >= 0 && selIx < (int)CATALOG.size();
  bool hasSelection = selIx >= 0;
  
  drawModernButton( 20, buttonAreaY, 80, 30, UI_PRIMARY, "UP", canScrollUp, false);
  drawModernButton(110, buttonAreaY, 80, 30, UI_PRIMARY, "DOWN", canScrollDown, false);
  drawModernButton(200, buttonAreaY, 100, 30, UI_SUCCESS, "ASSIGN", canAssign, false);
  drawModernButton(310, buttonAreaY, 80, 30, UI_WARNING, "PREVIEW", hasSelection, false);
  drawModernButton(400, buttonAreaY, 70, 30, UI_DANGER, "BACK", true, false);
  
  // Info status en bas
  gfx->setTextColor(UI_ON_SURFACE, RGB565(15, 15, 20));
  gfx->setCursor(20, 260);
  gfx->printf("Samples: %d | Showing: %d-%d", 
              (int)CATALOG.size(), 
              scrollIx + 1, 
              min(scrollIx + rowsVis, (int)CATALOG.size()));
              
  if (selIx >= 0) {
    gfx->setCursor(300, 260);
    gfx->setTextColor(UI_ACCENT, RGB565(15, 15, 20));
    gfx->printf("Selected: %.1fs", secondsOf(CATALOG[selIx]));
  }
}

// CORRECTION: Preview non destructif amélioré - FONCTION renommée
static void previewSample(int catIndex) {
  if (catIndex < 0 || catIndex >= (int)CATALOG.size()) return;
  
  // Sauvegarder état original
  int originalSlot = ROTvalue[selected_sound][0];
  const int tmpSlot = tempSlot();
  
  // Charger temporairement
  if (assignSampleToSlot(catIndex, tmpSlot)) {
    ROTvalue[selected_sound][0] = tmpSlot;
    setSound(selected_sound);
    synthESP32_TRIGGER(selected_sound);
    
    // Marquer pour affichage
    currentPreviewIdx = catIndex;  // CORRECTION: Variable renommée
    lastPreviewTime = millis();
    
    Serial.printf("[PICKER] Preview: %s (%.1fs)\n", 
                  CATALOG[catIndex].name.c_str(), secondsOf(CATALOG[catIndex]));
  }
  
  // Restaurer mapping original
  ROTvalue[selected_sound][0] = originalSlot;
  setSound(selected_sound);
}

// Assignation définitive avec feedback
static void assignIndexToPad(int catIndex) {
  if (catIndex < 0 || catIndex >= (int)CATALOG.size()) return;
  
  int slot = (int)ROTvalue[selected_sound][0];
  if (slot < 0 || slot >= BANK_SIZE) slot = selected_sound; // Fallback
  
  if (assignSampleToSlot(catIndex, slot)) {
    setSound(selected_sound);
    Serial.printf("[PICKER] Assigned %s to pad %d (slot %d)\n", 
                  CATALOG[catIndex].name.c_str(), selected_sound, slot);
  }
}

void openSamplePicker() {
  currentView = VIEW_PICKER;
  
  // Animation d'ouverture avec loading
  gfx->fillScreen(RGB565(15, 15, 20));
  gfx->setTextColor(UI_ACCENT, RGB565(15, 15, 20));
  gfx->setCursor(160, 130);
  gfx->print("Loading samples...");
  
  // Reset sélection et scroll
  selIx = -1;
  scrollIx = 0;
  currentPreviewIdx = -1;  // CORRECTION: Variable renommée
  lastPreviewTime = 0;
  redrawPickerList();
}

void handleTouchPicker(int x, int y) {
  // click sur une ligne -> preview + surlignage
  if (y >= listTopY && y < listTopY + rowsVis*rowH) {
    int row = (y - listTopY) / rowH;
    int ix = scrollIx + row;
    if (ix >= 0 && ix < (int)CATALOG.size()) {
      selIx = ix;
      
      // Animation de sélection
      int rowY = listTopY + row * rowH;
      gfx->drawRect(9, rowY - 2, 462, rowH + 2, UI_ACCENT);
      delay(80);
      
      // Preview automatique
      previewSample(ix);  // CORRECTION: Utiliser la fonction renommée
      redrawPickerList();
      return;
    }
  }
  
  // Boutons avec animations et logique améliorée
  if (y >= buttonAreaY && y <= buttonAreaY + 30) {
    // Scroll UP
    if (x >= 20 && x <= 100 && scrollIx > 0) {
      drawModernButton(20, buttonAreaY, 80, 30, UI_PRIMARY, "UP", true, true);
      delay(80);
      
      scrollIx = max(0, scrollIx - rowsVis);
      if (selIx >= 0 && selIx < scrollIx) {
        selIx = scrollIx; // Garder sélection visible
      }
      
      redrawPickerList();
      Serial.printf("[PICKER] Scroll up to %d\n", scrollIx);
      return;
    }
    
    // Scroll DOWN
    if (x >= 110 && x <= 190 && scrollIx + rowsVis < (int)CATALOG.size()) {
      drawModernButton(110, buttonAreaY, 80, 30, UI_PRIMARY, "DOWN", true, true);
      delay(80);
      
      scrollIx = min((int)CATALOG.size() - rowsVis, scrollIx + rowsVis);
      if (selIx >= 0 && selIx >= scrollIx + rowsVis) {
        selIx = scrollIx + rowsVis - 1; // Garder sélection visible
      }
      
      redrawPickerList();
      Serial.printf("[PICKER] Scroll down to %d\n", scrollIx);
      return;
    }
  
    // ASSIGN avec validation et feedback
    if (x >= 200 && x <= 300 && selIx >= 0) {
      drawModernButton(200, buttonAreaY, 100, 30, UI_SUCCESS, "ASSIGN", true, true);
      delay(100);
      
      assignIndexToPad(selIx);
      
      // Animation de confirmation réussie
      gfx->fillRect(150, 180, 180, 25, UI_SUCCESS);
      gfx->setTextColor(BLACK, UI_SUCCESS);
      gfx->setCursor(160, 195);
      gfx->printf("Assigned to pad %d!", selected_sound);
      delay(1200);
      
      Serial.printf("[PICKER] Assigned sample %d to pad %d\n", selIx, selected_sound);
      redrawPickerList();
      return;
    }
    
    // PREVIEW manuel
    if (x >= 310 && x <= 390 && selIx >= 0) {
      drawModernButton(310, buttonAreaY, 80, 30, UI_WARNING, "PREVIEW", false, true);
      delay(80);
      
      previewSample(selIx);  // CORRECTION: Utiliser la fonction renommée
      redrawPickerList();
      return;
    }

    // BACK avec transition animée
    if (x >= 400 && x <= 470) {
      drawModernButton(400, buttonAreaY, 70, 30, UI_DANGER, "BACK", true, true);
      
      // Effet de sortie
      for (int i = 0; i < 6; i++) {
        gfx->fillRect(0, 0, 480, 272, RGB565(i * 3, i * 3, i * 3));
        delay(25);
      }
      
      Serial.println("[PICKER] Returning to main view");
      currentView = VIEW_MAIN;
      forceCompleteRedraw();
      return;
    }
  }
  
  // Scroll rapide sur la barre de défilement
  if (x >= 470 && x <= 480 && y >= listTopY && y < listTopY + rowsVis * rowH) {
    // Calcul position relative dans la barre
    int relY = y - listTopY;
    int newScrollIx = (relY * (int)CATALOG.size()) / (rowsVis * rowH);
    newScrollIx = constrain(newScrollIx, 0, max(0, (int)CATALOG.size() - rowsVis));
    
    if (newScrollIx != scrollIx) {
      scrollIx = newScrollIx;
      redrawPickerList();
      Serial.printf("[PICKER] Quick scroll to %d\n", scrollIx);
    }
    return;
  }
}