// ui_recorder_lvgl.ino
#include <lvgl.h>
#include <SD.h>

// ==== API service recorder (exposée par recorder.ino) ====
extern "C" {
  void     rec_start();
  void     rec_stop();
  bool     rec_is_active();
  uint32_t rec_duration_ms();
  void     rec_get_vu(uint16_t* L, uint16_t* R);
  const char* rec_tmp_path();

  bool     rec_build_waveform(const char* path, uint16_t target_points);
  uint16_t rec_wave_copy(uint8_t* out, uint16_t max);
  uint32_t rec_total_ms();

  void     rec_edit_set_range_ms(uint32_t startMs, uint32_t stopMs);
  void     rec_edit_get_range_ms(uint32_t* startMs, uint32_t* stopMs);
  void     rec_edit_set_speed(float speed1x);
  void     rec_edit_set_pitch_semitones(float semi);
  bool     rec_render_to(const char* dstPath, bool applyTrim, bool applySpeedPitch);
}

extern "C" bool loadWavToSlot(const char* path, uint8_t slot);
extern uint8_t selected_sound;

extern void kb_prompt_text(const char* title, bool passwordMode, const char* initial,
                           void (*on_ok)(const char*), void (*on_cancel)());

// ==== UI state ====
static lv_obj_t* scr_rec = nullptr;
static lv_obj_t* barL = nullptr;
static lv_obj_t* barR = nullptr;
static lv_obj_t* lbl_dur = nullptr;
static lv_obj_t* lbl_rng = nullptr;
static lv_obj_t* lbl_speed = nullptr;
static lv_obj_t* lbl_pitch = nullptr;
static lv_obj_t* sld_start = nullptr;
static lv_obj_t* sld_end   = nullptr;

static lv_obj_t* canvas = nullptr;
static uint16_t* canvas_buf = nullptr;
static lv_timer_t* tmr = nullptr;

static const int WF_W = 460;
static const int WF_H = 96;

static void draw_waveform_into_canvas(){
  if (!canvas_buf) return;
  // fond
  uint16_t bg  = 0x0000;     // noir
  uint16_t mid = 0x52AA;     // gris
  uint16_t col = 0xFFFF;     // blanc
  uint16_t sel = 0x07E0;     // vert

  for (int i=0;i<WF_W*WF_H;i++) canvas_buf[i]=bg;

  // grille ligne médiane
  int ymid = WF_H/2;
  for (int x=0;x<WF_W;x++) canvas_buf[ymid*WF_W + x] = mid;

  // copie peaks
  static uint8_t peaks[480];
  uint16_t n = rec_wave_copy(peaks, (uint16_t)min(WF_W,480));

  // sélection (en pixels via sliders)
  uint32_t totalMs = rec_total_ms();
  uint32_t startMs=0, endMs=totalMs;
  rec_edit_get_range_ms(&startMs,&endMs);
  int xStart = (totalMs>0) ? (int)((startMs * (uint64_t)WF_W)/totalMs) : 0;
  int xEnd   = (totalMs>0) ? (int)((endMs   * (uint64_t)WF_W)/totalMs) : WF_W;

  // dessine sélection en fond
  for (int x=xStart; x<xEnd && x<WF_W; x++){
    for (int y=0;y<WF_H;y++){
      // teinte légère (mélange)
      uint16_t c = canvas_buf[y*WF_W + x];
      // simple mix : si c==bg on passe vert, sinon on laisse
      if (c==bg) canvas_buf[y*WF_W + x] = 0x07E0;
    }
  }
  // waveform en surimpression (blanc)
  for (int x=0;x<n && x<WF_W;x++){
    int amp = (int)peaks[x] * (WF_H-4) / 255;
    int y0 = ymid - amp/2;
    int y1 = ymid + amp/2;
    if (y0<0) y0=0; if (y1>=WF_H) y1=WF_H-1;
    for (int y=y0; y<=y1; y++){
      canvas_buf[y*WF_W + x] = col;
    }
  }

  // rafraîchit
  lv_obj_invalidate(canvas);
}

static void update_range_labels(){
  uint32_t s=0,e=0; rec_edit_get_range_ms(&s,&e);
  char buf[64];
  snprintf(buf,sizeof(buf),"RANGE: %u ms → %u ms", (unsigned)s, (unsigned)e);
  lv_label_set_text(lbl_rng, buf);
}

static void cb_back(lv_event_t*){ extern void build_main_menu(); build_main_menu(); }

static lv_obj_t* mk_btn(lv_obj_t* p, const char* t, lv_event_cb_t cb, int x, int y, int w=90, int h=36){
  lv_obj_t* b = lv_button_create(p);
  lv_obj_set_size(b, w, h);
  lv_obj_align(b, LV_ALIGN_TOP_LEFT, x, y);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* l = lv_label_create(b); lv_label_set_text(l, t); lv_obj_center(l);
  return b;
}

// ---- Callbacks boutons principaux ----
static void cb_rec(lv_event_t*){ if (!rec_is_active()) rec_start(); }
static void cb_stop(lv_event_t*){
  if (rec_is_active()) rec_stop();
  // (re)construit la waveform sur le fichier courant
  rec_build_waveform(rec_tmp_path(), (uint16_t)WF_W);
  // maj sliders plage complète
  rec_edit_set_range_ms(0, rec_total_ms());
  update_range_labels();
  draw_waveform_into_canvas();
}

static void cb_saveas(lv_event_t*){
  // Boite pour nom de fichier
  kb_prompt_text("Save as", false, "record_edit.wav",
    [](const char* txt){
      if (!txt || !*txt) return;
      String dest = String("/samples/") + txt;
      rec_render_to(dest.c_str(), /*trim*/true, /*speedPitch*/true);
      // option: recharger dans slot courant
      // loadWavToSlot(dest.c_str(), selected_sound);
    },
    nullptr);
}

static void cb_export_slot(lv_event_t*){
  // rend dans un tmp puis charge dans le slot sélectionné
  const char* tmpOut = "/samples/__render.wav";
  rec_render_to(tmpOut, true, true);
  loadWovToSlot: // (typo fix si signature : loadWavToSlot)
  loadWavToSlot(tmpOut, selected_sound);
}

static void cb_rebuild(lv_event_t*){
  rec_build_waveform(rec_tmp_path(), (uint16_t)WF_W);
  draw_waveform_into_canvas();
}

// ---- Sliders sélection (start/end en ‰ de la durée) ----
static void on_sld_change(lv_event_t* e){
  if (lv_event_get_code(e)!=LV_EVENT_VALUE_CHANGED) return;
  int vStart = lv_slider_get_value(sld_start);
  int vEnd   = lv_slider_get_value(sld_end);
  if (vEnd <= vStart+1){
    if (lv_event_get_target(e)==sld_start) vStart = vEnd-1;
    else                                    vEnd   = vStart+1;
    lv_slider_set_value(sld_start, vStart, LV_ANIM_OFF);
    lv_slider_set_value(sld_end,   vEnd,   LV_ANIM_OFF);
  }
  uint32_t T = rec_total_ms();
  uint32_t sMs = (uint32_t)((uint64_t)vStart * T / 1000ULL);
  uint32_t eMs = (uint32_t)((uint64_t)vEnd   * T / 1000ULL);
  rec_edit_set_range_ms(sMs,eMs);
  update_range_labels();
  draw_waveform_into_canvas();
}

// ---- Speed/Pitch sliders ----
static void on_speed_change(lv_event_t* e){
  int v = lv_slider_get_value(lv_event_get_target(e)); // 50..200 %
  float sp = (float)v / 100.0f;
  rec_edit_set_speed(sp);
  char buf[32]; snprintf(buf,sizeof(buf),"Speed: %d%%", v);
  lv_label_set_text(lbl_speed, buf);
}
static void on_pitch_change(lv_event_t* e){
  int semis = lv_slider_get_value(lv_event_get_target(e)); // -12..+12
  rec_edit_set_pitch_semitones((float)semis);
  char buf[32]; snprintf(buf,sizeof(buf),"Pitch: %+d st", semis);
  lv_label_set_text(lbl_pitch, buf);
}

// ---- Poll timer ----
static void poll(lv_timer_t*){
  uint16_t L=0,R=0; rec_get_vu(&L,&R);
  lv_bar_set_value(barL, L, LV_ANIM_OFF);
  lv_bar_set_value(barR, R, LV_ANIM_OFF);

  uint32_t ms = rec_duration_ms();
  char t[32]; snprintf(t,sizeof(t),"dur: %02u:%02u.%03u",(ms/60000)%60,(ms/1000)%60,ms%1000);
  lv_label_set_text(lbl_dur, t);
}

void build_recorder_view(){
  if (tmr) { lv_timer_del(tmr); tmr = nullptr; }
  scr_rec = lv_obj_create(NULL);
  lv_scr_load(scr_rec);

  // Titre + Back
  lv_obj_t* ttl = lv_label_create(scr_rec);
  lv_label_set_text(ttl, "Recorder / Editor");
  lv_obj_align(ttl, LV_ALIGN_TOP_LEFT, 6, 6);

  lv_obj_t* back = lv_button_create(scr_rec);
  lv_obj_set_size(back, 70, 32);
  lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -6, 6);
  lv_obj_add_event_cb(back, cb_back, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(back); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  // Boutons principaux
  mk_btn(scr_rec, "REC",     cb_rec,       10,  32);
  mk_btn(scr_rec, "STOP",    cb_stop,     110,  32);
  mk_btn(scr_rec, "REBUILD", cb_rebuild,  210,  32);
  mk_btn(scr_rec, "EXPORT",  cb_saveas,   310,  32);

  // VU + durée
  barL = lv_bar_create(scr_rec); lv_obj_set_size(barL, 220, 10); lv_obj_align(barL, LV_ALIGN_TOP_LEFT, 10, 74); lv_bar_set_range(barL, 0, 255);
  barR = lv_bar_create(scr_rec); lv_obj_set_size(barR, 220, 10); lv_obj_align(barR, LV_ALIGN_TOP_LEFT, 10, 90); lv_bar_set_range(barR, 0, 255);
  lbl_dur = lv_label_create(scr_rec); lv_obj_align(lbl_dur, LV_ALIGN_TOP_RIGHT, -10, 86); lv_label_set_text(lbl_dur, "dur: 00:00.000");

  // Waveform canvas
  canvas = lv_canvas_create(scr_rec);
  canvas_buf = (uint16_t*)malloc(WF_W*WF_H*2);
  lv_canvas_set_buffer(canvas, canvas_buf, WF_W, WF_H, LV_COLOR_FORMAT_RGB565);
  lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 10, 112);

  // Sliders de sélection (start/end en ‰)
  sld_start = lv_slider_create(scr_rec);
  lv_obj_set_size(sld_start, 220, 14);
  lv_obj_align(sld_start, LV_ALIGN_TOP_LEFT, 10, 112 + WF_H + 8);
  lv_slider_set_range(sld_start, 0, 1000);
  lv_obj_add_event_cb(sld_start, on_sld_change, LV_EVENT_VALUE_CHANGED, NULL);

  sld_end = lv_slider_create(scr_rec);
  lv_obj_set_size(sld_end, 220, 14);
  lv_obj_align(sld_end, LV_ALIGN_TOP_LEFT, 250, 112 + WF_H + 8);
  lv_slider_set_range(sld_end, 0, 1000);
  lv_obj_add_event_cb(sld_end, on_sld_change, LV_EVENT_VALUE_CHANGED, NULL);

  lbl_rng = lv_label_create(scr_rec);
  lv_obj_align(lbl_rng, LV_ALIGN_TOP_LEFT, 10, 112 + WF_H + 28);
  lv_label_set_text(lbl_rng, "RANGE: 0 ms → 0 ms");

  // Speed / Pitch
  lv_obj_t* sld_speed = lv_slider_create(scr_rec);
  lv_obj_set_size(sld_speed, 220, 14);
  lv_obj_align(sld_speed, LV_ALIGN_TOP_LEFT, 10, 112 + WF_H + 48);
  lv_slider_set_range(sld_speed, 50, 200); // 50..200 %
  lv_slider_set_value(sld_speed, 100, LV_ANIM_OFF);
  lv_obj_add_event_cb(sld_speed, on_speed_change, LV_EVENT_VALUE_CHANGED, NULL);

  lbl_speed = lv_label_create(scr_rec);
  lv_obj_align(lbl_speed, LV_ALIGN_TOP_LEFT, 240, 112 + WF_H + 46);
  lv_label_set_text(lbl_speed, "Speed: 100%");

  lv_obj_t* sld_pitch = lv_slider_create(scr_rec);
  lv_obj_set_size(sld_pitch, 220, 14);
  lv_obj_align(sld_pitch, LV_ALIGN_TOP_LEFT, 10, 112 + WF_H + 68);
  lv_slider_set_range(sld_pitch, -12, 12);
  lv_slider_set_value(sld_pitch, 0, LV_ANIM_OFF);
  lv_obj_add_event_cb(sld_pitch, on_pitch_change, LV_EVENT_VALUE_CHANGED, NULL);

  lbl_pitch = lv_label_create(scr_rec);
  lv_obj_align(lbl_pitch, LV_ALIGN_TOP_LEFT, 240, 112 + WF_H + 66);
  lv_label_set_text(lbl_pitch, "Pitch: +0 st");

  // Init waveform (si un fichier existe déjà)
  rec_build_waveform(rec_tmp_path(), (uint16_t)WF_W);
  rec_edit_set_range_ms(0, rec_total_ms());
  lv_slider_set_value(sld_start, 0, LV_ANIM_OFF);
  lv_slider_set_value(sld_end, 1000, LV_ANIM_OFF);
  update_range_labels();
  draw_waveform_into_canvas();

  // Timer
  tmr = lv_timer_create(poll, 50, NULL);
}
