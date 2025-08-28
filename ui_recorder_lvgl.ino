// ui_recorder_lvgl.ino
#include <lvgl.h>
#include <SD.h>

// ==== Backend recorder (protos) ====
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

  // Wrappers “compat”
  void     rec_set_speed_percent(int pct);
  void     rec_set_pitch_semitones(int semis);
  void     rec_set_region(uint32_t startMs, uint32_t stopMs);

  // Export rendu
  bool     rec_render_to(const char* dstPath, bool applyTrim, bool applySpeedPitch);
}

// Optionnel: auto-load vers slot si besoin
extern bool loadWavToSlot(const char* path, uint8_t slot);
extern uint8_t selected_sound;

// Clavier (SAVEAS / WiFi etc.)
extern void kb_prompt_text(const char* title, bool passwordMode, const char* initial,
                           void (*on_ok)(const char*), void (*on_cancel)());

// ==== UI nodes ====
static lv_obj_t* scr_rec   = nullptr;
static lv_obj_t* lbl_dur   = nullptr;
static lv_obj_t* barL      = nullptr;
static lv_obj_t* barR      = nullptr;
static lv_obj_t* chart     = nullptr;
static lv_chart_series_t* s_wave = nullptr;
static lv_obj_t* slider_speed = nullptr;
static lv_obj_t* slider_pitch = nullptr;
static lv_obj_t* lbl_speed = nullptr;
static lv_obj_t* lbl_pitch = nullptr;

static lv_obj_t* slider_start = nullptr;
static lv_obj_t* slider_end   = nullptr;
static lv_obj_t* lbl_range    = nullptr;

static lv_timer_t* tmr_rec = nullptr;

// État UI
static bool s_prev_rec_active = false;
static uint32_t s_len_ms = 0;
static uint32_t s_start_ms = 0;
static uint32_t s_end_ms   = 0;

// --- helpers ---
static void chart_config(lv_obj_t* c, uint16_t points) {
  lv_chart_set_type(c, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(c, points);
  lv_chart_set_range(c, LV_CHART_AXIS_PRIMARY_Y, 0, 255);
  lv_chart_set_div_line_count(c, 4, 6);
  lv_obj_set_style_pad_all(c, 6, 0);
}
static void chart_set_wave(lv_obj_t* c, lv_chart_series_t* ser, const uint8_t* y, uint16_t n) {
  if (!c || !ser || !y || n == 0) return;
  lv_chart_set_point_count(c, n);
  for (uint16_t i = 0; i < n; ++i) lv_chart_set_value_by_id(c, ser, i, y[i]);
  lv_chart_refresh(c);
}
static void refresh_waveform_from_backend() {
  if (!chart || !s_wave) return;
  static uint8_t peaks[480];
  uint16_t n = rec_wave_copy(peaks, (uint16_t)(sizeof(peaks)));
  if (n == 0) {
    lv_chart_set_point_count(chart, 8);
    for (uint16_t i = 0; i < 8; ++i) lv_chart_set_value_by_id(chart, s_wave, i, 0);
    lv_chart_refresh(chart);
    return;
  }
  chart_set_wave(chart, s_wave, peaks, n);
}
static void update_speed_label(int pct) {
  char t[32]; snprintf(t, sizeof(t), "Speed: %d%%", pct);
  lv_label_set_text(lbl_speed, t);
}
static void update_pitch_label(int semis) {
  char t[32]; snprintf(t, sizeof(t), "Pitch: %+d st", semis);
  lv_label_set_text(lbl_pitch, t);
}
static void update_range_label() {
  char t[48];
  snprintf(t, sizeof(t), "Range: %u–%u ms", (unsigned)s_start_ms, (unsigned)s_end_ms);
  lv_label_set_text(lbl_range, t);
}

// ==== Callbacks ====
static void cb_back_rec(lv_event_t*) {
  extern void build_main_menu();
  build_main_menu();
}
static void cb_rec(lv_event_t*) {
  if (!rec_is_active()) {
    rec_start();
    s_prev_rec_active = true;
  }
}
static void cb_stop(lv_event_t*) {
  if (rec_is_active()) {
    rec_stop();
    rec_build_waveform(rec_tmp_path(), 480);
    refresh_waveform_from_backend();
    s_prev_rec_active = false;
    // longueur totale
    s_len_ms = rec_total_ms();
    if (s_len_ms == 0) s_len_ms = rec_duration_ms();
    // reset range par défaut
    s_start_ms = 0; s_end_ms = s_len_ms ? s_len_ms : 1;
    rec_set_region(s_start_ms, s_end_ms);
    update_range_label();
    if (slider_start) lv_slider_set_value(slider_start, 0, LV_ANIM_OFF);
    if (slider_end)   lv_slider_set_value(slider_end, 100, LV_ANIM_OFF);
  }
}
static void cb_saveas(lv_event_t*) {
  kb_prompt_text("Save as", false, "record.wav",
    [](const char* txt) {
      if (!txt || !*txt) return;
      String dest = String("/samples/") + txt;
      if (SD.exists(dest)) SD.remove(dest);
      SD.rename(rec_tmp_path(), dest.c_str());
      rec_build_waveform(dest.c_str(), 480);
      refresh_waveform_from_backend();
    }, nullptr);
}
static void cb_export(lv_event_t*) {
  // Rend vers /samples/edited.wav avec TRIM + Speed/Pitch
  const char* out = "/samples/edited.wav";
  if (rec_render_to(out, true, true)) {
    // Optionnel: auto-load dans le slot courant
    // loadWavToSlot(out, selected_sound);
  }
}

// sliders valeur
static void on_speed_change(lv_event_t* e) {
  lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
  int v = lv_slider_get_value(t);
  update_speed_label(v);
  rec_set_speed_percent(v);
}
static void on_pitch_change(lv_event_t* e) {
  lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
  int semis = lv_slider_get_value(t);
  update_pitch_label(semis);
  rec_set_pitch_semitones(semis);
}

// sliders range en % 0..100 → ms
static void on_range_change(lv_event_t* e) {
  if (s_len_ms == 0) s_len_ms = rec_total_ms();
  if (s_len_ms == 0) return;
  int p_start = lv_slider_get_value(slider_start);
  int p_end   = lv_slider_get_value(slider_end);
  if (p_end < p_start) { // force end >= start
    p_end = p_start;
    lv_slider_set_value(slider_end, p_end, LV_ANIM_OFF);
  }
  s_start_ms = (uint32_t)((uint64_t)p_start * s_len_ms / 100ULL);
  s_end_ms   = (uint32_t)((uint64_t)p_end   * s_len_ms / 100ULL);
  if (s_end_ms <= s_start_ms) s_end_ms = s_start_ms + 1;
  rec_set_region(s_start_ms, s_end_ms);
  update_range_label();
}

// Timer de poll UI
static void poll(lv_timer_t*) {
  uint16_t L = 0, R = 0; rec_get_vu(&L, &R);
  lv_bar_set_value(barL, L, LV_ANIM_OFF);
  lv_bar_set_value(barR, R, LV_ANIM_OFF);

  uint32_t ms = rec_duration_ms();
  char t[32]; snprintf(t, sizeof(t), "dur: %02u:%02u.%03u", (ms / 60000) % 60, (ms / 1000) % 60, ms % 1000);
  lv_label_set_text(lbl_dur, t);

  bool ra = rec_is_active();
  if (s_prev_rec_active && !ra) {
    rec_build_waveform(rec_tmp_path(), 480);
    refresh_waveform_from_backend();
    s_len_ms = rec_total_ms();
    if (s_len_ms == 0) s_len_ms = ms;
  }
  s_prev_rec_active = ra;
}

// ==== Fabrication UI ====
static lv_obj_t* make_btn(lv_obj_t* parent, const char* txt, lv_event_cb_t cb) {
  lv_obj_t* b = lv_button_create(parent);
  lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(b, 8, 0);
  lv_obj_set_style_radius(b, 10, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* l = lv_label_create(b); lv_label_set_text(l, txt);
  return b;
}

void build_recorder_view() {
  if (tmr_rec) { lv_timer_del(tmr_rec); tmr_rec = nullptr; }

  scr_rec = lv_obj_create(NULL);
  lv_obj_set_size(scr_rec, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_rec, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr_rec, 8, 0);
  lv_scr_load(scr_rec);

  // Header
  lv_obj_t* header = lv_obj_create(scr_rec);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(header, 4, 0);
  lv_obj_set_style_pad_column(header, 6, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* lbl = lv_label_create(header); lv_label_set_text(lbl, "Recorder");
  lv_obj_t* spacer = lv_obj_create(header); lv_obj_set_flex_grow(spacer, 1);
  lv_obj_set_style_bg_opa(spacer, LV_OPA_0, 0); lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
  make_btn(header, "BACK", cb_back_rec);

  // Toolbar
  lv_obj_t* toolbar = lv_obj_create(scr_rec);
  lv_obj_set_width(toolbar, LV_PCT(100));
  lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(toolbar, 6, 0);
  lv_obj_set_style_pad_column(toolbar, 6, 0);
  lv_obj_set_style_pad_row(toolbar, 6, 0);
  lv_obj_clear_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);
  make_btn(toolbar, "REC", cb_rec);
  make_btn(toolbar, "STOP", cb_stop);
  make_btn(toolbar, "SAVEAS", cb_saveas);
  make_btn(toolbar, "EXPORT", cb_export);

  // Waveform
  chart = lv_chart_create(scr_rec);
  lv_obj_set_width(chart, LV_PCT(100));
  lv_obj_set_height(chart, LV_PCT(38));
  chart_config(chart, 120);
  s_wave = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

  // Range sliders (start/end %)
  lv_obj_t* range = lv_obj_create(scr_rec);
  lv_obj_set_width(range, LV_PCT(100));
  lv_obj_set_flex_flow(range, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(range, 6, 0);
  lv_obj_clear_flag(range, LV_OBJ_FLAG_SCROLLABLE);

  lbl_range = lv_label_create(range); lv_label_set_text(lbl_range, "Range: 0–0 ms");

  slider_start = lv_slider_create(range);
  lv_slider_set_range(slider_start, 0, 100);
  lv_slider_set_value(slider_start, 0, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider_start, on_range_change, LV_EVENT_VALUE_CHANGED, NULL);

  slider_end = lv_slider_create(range);
  lv_slider_set_range(slider_end, 0, 100);
  lv_slider_set_value(slider_end, 100, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider_end, on_range_change, LV_EVENT_VALUE_CHANGED, NULL);

  // Speed / Pitch
  lv_obj_t* sliders = lv_obj_create(scr_rec);
  lv_obj_set_width(sliders, LV_PCT(100));
  lv_obj_set_flex_flow(sliders, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(sliders, 6, 0);
  lv_obj_set_style_pad_column(sliders, 10, 0);
  lv_obj_set_style_pad_row(sliders, 6, 0);
  lv_obj_clear_flag(sliders, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* col_speed = lv_obj_create(sliders);
  lv_obj_set_width(col_speed, LV_PCT(48));
  lv_obj_set_flex_flow(col_speed, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(col_speed, 6, 0);
  lv_obj_clear_flag(col_speed, LV_OBJ_FLAG_SCROLLABLE);
  lbl_speed = lv_label_create(col_speed); update_speed_label(100);
  slider_speed = lv_slider_create(col_speed);
  lv_slider_set_range(slider_speed, 50, 200);
  lv_slider_set_value(slider_speed, 100, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider_speed, on_speed_change, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t* col_pitch = lv_obj_create(sliders);
  lv_obj_set_width(col_pitch, LV_PCT(48));
  lv_obj_set_flex_flow(col_pitch, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(col_pitch, 6, 0);
  lv_obj_clear_flag(col_pitch, LV_OBJ_FLAG_SCROLLABLE);
  lbl_pitch = lv_label_create(col_pitch); update_pitch_label(0);
  slider_pitch = lv_slider_create(col_pitch);
  lv_slider_set_range(slider_pitch, -12, 12);
  lv_slider_set_value(slider_pitch, 0, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider_pitch, on_pitch_change, LV_EVENT_VALUE_CHANGED, NULL);

  // VU + durée
  lv_obj_t* meter = lv_obj_create(scr_rec);
  lv_obj_set_width(meter, LV_PCT(100));
  lv_obj_set_flex_flow(meter, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(meter, 6, 0);
  lv_obj_set_style_pad_column(meter, 8, 0);
  lv_obj_clear_flag(meter, LV_OBJ_FLAG_SCROLLABLE);

  barL = lv_bar_create(meter); lv_obj_set_width(barL, LV_PCT(40)); lv_bar_set_range(barL, 0, 255);
  barR = lv_bar_create(meter); lv_obj_set_width(barR, LV_PCT(40)); lv_bar_set_range(barR, 0, 255);
  lbl_dur = lv_label_create(meter); lv_label_set_text(lbl_dur, "dur: 00:00.000");

  // First waveform if exists
  if (SD.exists(rec_tmp_path())) { rec_build_waveform(rec_tmp_path(), 480); }
  refresh_waveform_from_backend();

  // init range
  s_len_ms = rec_total_ms();
  s_start_ms = 0; s_end_ms = s_len_ms ? s_len_ms : 1;
  update_range_label();
  rec_set_region(s_start_ms, s_end_ms);

  // Timer
  tmr_rec = lv_timer_create(poll, 80, NULL);
}
