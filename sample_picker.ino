#include <Arduino.h>
#include <vector>  // <-- nécessaire pour std::vector
extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);
extern Arduino_GFX *gfx;
#include "synth_api.h"
#include "sd_catalog.h"
extern View currentView;

extern byte    selected_sound;
extern int32_t ROTvalue[16][8];
extern void    synthESP32_TRIGGER(unsigned char voice);
extern void    setSound(byte voice);

static inline int tempSlot() { return 255; } // slot tampon de preview

// --- UI layout ---
static int listTopY = 30;
static int rowH     = 18;
static int rowsVis  = 12;   // ~ 12 lignes lisibles à l’écran
static int scrollIx = 0;    // index de la première ligne affichée
static int selIx    = -1;   // dernier index cliqué (pour ASSIGN)

// helpers
static inline float secondsOf(const SampleMeta& m) { return sampleSeconds(m); }

static void drawRow(int listIndex, int y, bool selected) {
  const SampleMeta& m = CATALOG[listIndex];
  if (selected) {
    gfx->fillRect(0, y-1, 480, rowH, RGB565(32,32,48));
  }
  gfx->setCursor(10, y);
  gfx->setTextColor(WHITE, selected ? RGB565(32,32,48) : BLACK);
  // chemin complet + durée en secondes
  gfx->print(m.path);
  gfx->print("   ");
  gfx->print(secondsOf(m), 2);
  gfx->print("s");
}

static void redrawPickerList() {
  // zone liste
  gfx->fillRect(0, listTopY-2, 480, rowsVis*rowH+4, BLACK);
  int y = listTopY;
  for (int i = 0; i < rowsVis; ++i) {
    int ix = scrollIx + i;
    if (ix >= 0 && ix < (int)CATALOG.size()) {
      drawRow(ix, y, ix == selIx);
      y += rowH;
    }
  }
  // bas de page : boutons
  // ASSIGN     BACK
  int bx = 20, by = listTopY + rowsVis*rowH + 10;
  gfx->drawRect(bx, by, 120, 34, DARKGREY);
  gfx->setCursor(bx+10, by+22); gfx->print("ASSIGN");
  gfx->drawRect(bx+140, by, 120, 34, DARKGREY);
  gfx->setCursor(bx+150, by+22); gfx->print("BACK");
}

void openSamplePicker() {
  currentView = VIEW_PICKER;
  gfx->fillScreen(BLACK);
  // barre de titre (même style que drawTopBar)
  gfx->setCursor(10, 14);
  gfx->setTextColor(CYAN, BLACK);
  gfx->print("SAMPLE PICKER");
  // reset selection
  selIx = -1;
  scrollIx = 0;
  redrawPickerList();
}

// Preview non destructif : charge CATALOG[index] sur slot temp, commute SAM -> temp, TRIGGER, puis restaure
static void previewIndex(int catIndex) {
  if (catIndex < 0 || catIndex >= (int)CATALOG.size()) return;
  int oldSlot = ROTvalue[selected_sound][0];
  const int tmpSlot = tempSlot();
  assignSampleToSlot(catIndex, tmpSlot);
  ROTvalue[selected_sound][0] = tmpSlot;
  setSound(selected_sound);
  synthESP32_TRIGGER(selected_sound);
  // restore mapping d’origine
  ROTvalue[selected_sound][0] = oldSlot;
  setSound(selected_sound);
}

// Assignation réelle : copie le sample choisi dans un slot stable (= celui du pad)
static void assignIndexToPad(int catIndex) {
  if (catIndex < 0 || catIndex >= (int)CATALOG.size()) return;
  int slot = (int)ROTvalue[selected_sound][0];
  if (slot < 0 || slot >= BANK_SIZE) slot = 0;
  assignSampleToSlot(catIndex, slot);
  setSound(selected_sound);
}

void handleTouchPicker(int x, int y) {
  // click sur une ligne -> preview + surlignage
  if (y >= listTopY && y < listTopY + rowsVis*rowH) {
    int row = (y - listTopY) / rowH;
    int ix = scrollIx + row;
    if (ix >= 0 && ix < (int)CATALOG.size()) {
      selIx = ix;
      previewIndex(ix);
      redrawPickerList();
      return;
    }
  }
  // boutons bas : ASSIGN / BACK
  int by = listTopY + rowsVis*rowH + 10;
  // ASSIGN (x:20..140, y:by..by+34)
  if (x >= 20 && x <= 140 && y >= by && y <= by+34) {
    if (selIx >= 0) {
      assignIndexToPad(selIx);
    }
    redrawPickerList();
    return;
  }
  // BACK (x:160..280)
  if (x >= 160 && x <= 280 && y >= by && y <= by+34) {
    currentView = VIEW_MAIN;
    return;
  }
  // scroll simple : zone droite (barre imaginaire)
  // haut: scrollIx--
  if (x >= 440 && x <= 470 && y >= listTopY && y <= listTopY+40) {
    if (scrollIx > 0) { scrollIx--; redrawPickerList(); }
    return;
  }
  // bas: scrollIx++
  if (x >= 440 && x <= 470 && y >= listTopY+rowsVis*rowH-40 && y <= listTopY+rowsVis*rowH) {
    if (scrollIx + rowsVis < (int)CATALOG.size()) { scrollIx++; redrawPickerList(); }
    return;
  }
}