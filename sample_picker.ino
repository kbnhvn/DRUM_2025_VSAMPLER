#include <vector>  // <-- nécessaire pour std::vector
extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);
extern Arduino_GFX *gfx;

extern byte    selected_sound;
extern int32_t ROTvalue[16][8];
void synthESP32_TRIGGER(unsigned char voice);
void assignSampleToSlot(int catIndex, int slot);

// struct SampleMeta déjà défini dans sd_catalog.ino
struct SampleMeta;
extern std::vector<SampleMeta> CATALOG;  // <-- OK si sd_catalog.ino l’expose vraiment
// Si au lieu de ça tu exposes un tableau C-style, utilise ceci :
// extern SampleMeta catalog[];
// extern int catalogCount;

static inline int tempSlot(){ return 255; } // slot tampon

// Preview : charge catIndex → slot 255, switch temporaire le param SAM du pad, trigger, puis restaure
void previewCatalogItem(int catIndex){
  int oldSlot = ROTvalue[selected_sound][0];
  int slot = tempSlot();
  assignSampleToSlot(catIndex, slot);
  ROTvalue[selected_sound][0] = slot;
  synthESP32_TRIGGER(selected_sound);
  ROTvalue[selected_sound][0] = oldSlot;
}

void openSamplePicker(){
  gfx->fillScreen(BLACK);
  drawTopBar("SAMPLES (SD)", true);
  gfx->setCursor(10,30); gfx->print("Use picker via dedicated keys; preview is non-destructive.");
}
