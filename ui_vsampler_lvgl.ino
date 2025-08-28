// ui_vsampler_lvgl.ino
#include <lvgl.h>

// Hooks compat (déjà fournis dans ui_lvgl_compat.ino)
void UI_Compat_Pad(uint8_t);
void UI_Compat_Row1(uint8_t);
void UI_Compat_Row2(uint8_t);
void UI_Compat_Bar(uint8_t);

// État exposé
extern byte selected_sound, selected_rot;
extern int bpm, octave;
extern byte sstep;
extern uint16_t pattern[16];
extern int32_t ROTvalue[16][8];

static const char* kBars[16] = {
  "SAM","INI","END","PIT","RVS","VOL","PAN","FIL",
  "BPM","MVO","TRP","MFI","OCT","MPI","SYN","SCA"
};
static const char* kRow1[8] = { "PAD","RND P","LOAD P","SAVE PS","MUTE","PIANO","PLAY","SONG" };
static const char* kRow2[8] = { "SHIFT","-1","-10","+10","+1","","","SHIFT" };

static lv_obj_t* scr_vs = nullptr;
static lv_obj_t* pads[16];
static lv_obj_t* bars[16];
static lv_obj_t* lbl_info = nullptr;
static lv_timer_t* tmr_vs = nullptr;

static void cb_back_vs(lv_event_t*){
  extern void build_main_menu();
  build_main_menu();
}

static void cb_pad (lv_event_t* e){ UI_Compat_Pad ((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }
static void cb_row1(lv_event_t* e){ UI_Compat_Row1((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }
static void cb_row2(lv_event_t* e){ UI_Compat_Row2((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }
static void cb_bar (lv_event_t* e){ UI_Compat_Bar ((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }

static void refresh_ui(lv_timer_t*){
  char info[64];
  snprintf(info, sizeof(info), "SND:%u  BAR:%s  BPM:%d  OCT:%d",
           (unsigned)selected_sound, kBars[selected_rot % 16], bpm, octave);
  lv_label_set_text(lbl_info, info);

  for (int i=0;i<16;i++){
    bool on = (pattern[i] >> sstep) & 1;
    lv_obj_set_style_bg_opa(pads[i], on ? LV_OPA_100 : LV_OPA_30, 0);
    if (i < 8){
      char t[20]; snprintf(t,sizeof(t), "%s:%ld", kBars[i], (long)ROTvalue[selected_sound][i]);
      lv_label_set_text(lv_obj_get_child(bars[i], 0), t);
    } else if (i==8 || i==12) {
      char t[16]; snprintf(t,sizeof(t), "%s:%d", kBars[i], (i==8)?bpm:octave);
      lv_label_set_text(lv_obj_get_child(bars[i], 0), t);
    } else {
      lv_label_set_text(lv_obj_get_child(bars[i], 0), kBars[i]);
    }
  }
}

static lv_obj_t* make_btn(lv_obj_t* p, const char* t, lv_event_cb_t cb, void* ud){
  lv_obj_t* b = lv_button_create(p);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  lv_obj_set_style_radius(b, 10, 0);
  lv_obj_set_style_pad_all(b, 6, 0);
  lv_obj_t* l = lv_label_create(b); lv_label_set_text(l, t); lv_obj_center(l);
  return b;
}

void build_vsampler_view(){
  if (tmr_vs) { lv_timer_del(tmr_vs); tmr_vs = nullptr; }

  scr_vs = lv_obj_create(NULL);
  lv_obj_set_size(scr_vs, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_vs, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr_vs, 8, 0);
  lv_scr_load(scr_vs);

  // Header
  lv_obj_t* header = lv_obj_create(scr_vs);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(header, 6, 0);
  lv_obj_set_style_pad_column(header, 8, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* ttl = lv_label_create(header); lv_label_set_text(ttl, "VSampler");

  lv_obj_t* spacer = lv_obj_create(header);
  lv_obj_set_flex_grow(spacer, 1);
  lv_obj_set_style_bg_opa(spacer, LV_OPA_0, 0);
  lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* btnBack = lv_button_create(header);
  lv_obj_add_event_cb(btnBack, cb_back_vs, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(btnBack); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  // Info
  lbl_info = lv_label_create(scr_vs);
  lv_label_set_text(lbl_info, "SND:0  BAR:SAM  BPM:120  OCT:5");

  // Grille pads 4x4
  lv_obj_t* pad_grid = lv_obj_create(scr_vs);
  lv_obj_set_width(pad_grid, LV_PCT(100));
  lv_obj_set_flex_flow(pad_grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(pad_grid, 6, 0);
  lv_obj_set_style_pad_row(pad_grid, 6, 0);
  lv_obj_set_style_pad_column(pad_grid, 6, 0);
  lv_obj_clear_flag(pad_grid, LV_OBJ_FLAG_SCROLLABLE);

  for (int i=0;i<16;i++){
    pads[i] = make_btn(pad_grid, "", cb_pad, (void*)(uintptr_t)i);
    lv_obj_set_size(pads[i], LV_PCT(23), 52);  // 4 par ligne
    lv_obj_set_style_bg_opa(pads[i], LV_OPA_30, 0);
    lv_obj_t* num = lv_label_create(pads[i]);
    char t[6]; snprintf(t,sizeof(t), "%d", i);
    lv_label_set_text(num, t); lv_obj_align(num, LV_ALIGN_BOTTOM_RIGHT, -4, -2);
  }

  // Deux colonnes (row1 / row2)
  lv_obj_t* rows = lv_obj_create(scr_vs);
  lv_obj_set_width(rows, LV_PCT(100));
  lv_obj_set_flex_flow(rows, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(rows, 6, 0);
  lv_obj_set_style_pad_column(rows, 8, 0);
  lv_obj_clear_flag(rows, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* col1 = lv_obj_create(rows);
  lv_obj_set_width(col1, LV_PCT(48));
  lv_obj_set_flex_flow(col1, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(col1, 0, 0);
  lv_obj_set_style_pad_row(col1, 4, 0);
  lv_obj_clear_flag(col1, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* col2 = lv_obj_create(rows);
  lv_obj_set_width(col2, LV_PCT(48));
  lv_obj_set_flex_flow(col2, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(col2, 0, 0);
  lv_obj_set_style_pad_row(col2, 4, 0);
  lv_obj_clear_flag(col2, LV_OBJ_FLAG_SCROLLABLE);

  for (int i=0;i<8;i++){
    make_btn(col1, kRow1[i], cb_row1, (void*)(uintptr_t)i);
    make_btn(col2, kRow2[i], cb_row2, (void*)(uintptr_t)i);
  }

  // 16 “bars” (sélection de param)
  lv_obj_t* bars_row = lv_obj_create(scr_vs);
  lv_obj_set_width(bars_row, LV_PCT(100));
  lv_obj_set_flex_flow(bars_row, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(bars_row, 6, 0);
  lv_obj_set_style_pad_row(bars_row, 4, 0);
  lv_obj_set_style_pad_column(bars_row, 4, 0);
  lv_obj_clear_flag(bars_row, LV_OBJ_FLAG_SCROLLABLE);

  for (int i=0;i<16;i++){
    bars[i] = make_btn(bars_row, kBars[i], cb_bar, (void*)(uintptr_t)i);
    lv_obj_set_size(bars[i], LV_PCT(12), 30); // 8 par ligne (wrap)
  }

  // Timer de refresh
  tmr_vs = lv_timer_create(refresh_ui, 60, NULL);
}
