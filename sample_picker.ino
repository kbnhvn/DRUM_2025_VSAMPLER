extern void drawTopBar(const char* title, bool showBack);
extern void drawButtonBox(int x,int y,int w,int h,int color,const char* txt);
extern Arduino_GFX *gfx;

extern int selected_sound;
extern uint8_t ROTvalue[16][16];
void synthESP32_TRIGGER(unsigned char voice);
void assignSampleToSlot(int catIndex, int slot);

struct SampleMeta { String path; String name; uint32_t len; int16_t* buf; };
extern std::vector<SampleMeta> CATALOG;

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
