// ui_vsampler_lvgl.ino
#include <lvgl.h>
#include "ui_lvgl_compat.ino"

// étiquettes (alignées sur tes 16 paramètres)
static const char* kBars[16] = {
  "SAM","INI","END","PIT","RVS","VOL","PAN","FIL",
  "BPM","MVO","TRP","MFI","OCT","MPI","SYN","SCA"
};
static const char* kRow1[8] = { "PAD","RND P","LOAD P","SAVE PS","MUTE","PIANO","PLAY","SONG" };
static const char* kRow2[8] = { "SHIFT","-1","-10","+10","+1","","","SHIFT" };

// externs d’état pour affichage
extern byte selected_sound, selected_rot;
extern int bpm, octave;
extern byte sstep;
extern uint16_t pattern[16];
extern int32_t ROTvalue[16][8];

static lv_obj_t* scr_vs = nullptr;
static lv_obj_t* bars[16];
static lv_obj_t* pads[16];
static lv_obj_t* lbl_info;
static lv_timer_t* tmr = nullptr;

static lv_obj_t* make_btn(lv_obj_t* p, const char* t, lv_event_cb_t cb, void* ud, int w, int h, int x, int y){
  lv_obj_t* b = lv_button_create(p);
  lv_obj_set_size(b, w, h);
  lv_obj_align(b, LV_ALIGN_TOP_LEFT, x, y);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  lv_obj_t* l = lv_label_create(b); lv_label_set_text(l, t); lv_obj_center(l);
  return b;
}

static void cb_back(lv_event_t*){
  extern void build_main_menu();
  build_main_menu();
}

static void cb_pad (lv_event_t* e){ UI_Compat_Pad ((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }
static void cb_row1(lv_event_t* e){ UI_Compat_Row1((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }
static void cb_row2(lv_event_t* e){ UI_Compat_Row2((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }
static void cb_bar (lv_event_t* e){ UI_Compat_Bar ((uint8_t)(uintptr_t)lv_event_get_user_data(e)); }

static void refresh_ui(lv_timer_t*){
  char info[64]; snprintf(info, sizeof(info), "SND:%u  BAR:%s  BPM:%d  OCT:%d",
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

void build_vsampler_view(){
  if (tmr) { lv_timer_del(tmr); tmr = nullptr; }

  scr_vs = lv_obj_create(NULL);
  lv_scr_load(scr_vs);

  lv_obj_t* ttl = lv_label_create(scr_vs);
  lv_label_set_text(ttl, "VSampler");
  lv_obj_align(ttl, LV_ALIGN_TOP_LEFT, 6, 6);

  // BACK
  lv_obj_t* back = lv_button_create(scr_vs);
  lv_obj_set_size(back, 70, 32);
  lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -6, 6);
  lv_obj_add_event_cb(back, cb_back, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(back); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  lbl_info = lv_label_create(scr_vs);
  lv_obj_align(lbl_info, LV_ALIGN_TOP_RIGHT, -6, 28);

  // 16 pads (4x4)
  int px=6, py=40, pw=72, ph=48, gx=6, gy=6;
  for (int i=0;i<16;i++){
    int cx = px + (i%4)*(pw+gx);
    int cy = py + (i/4)*(ph+gy);
    pads[i] = make_btn(scr_vs, "", cb_pad, (void*)(uintptr_t)i, pw, ph, cx, cy);
    lv_obj_set_style_bg_opa(pads[i], LV_OPA_30, 0);
    lv_obj_t* num = lv_label_create(pads[i]);
    char t[6]; snprintf(t,sizeof(t), "%d", i);
    lv_label_set_text(num, t); lv_obj_align(num, LV_ALIGN_BOTTOM_RIGHT, -4, -2);
  }

  // rangées de 8 boutons (row1/row2)
  int bx=300, by=40, bw=80, bh=28, gy2=4;
  for (int i=0;i<8;i++)  make_btn(scr_vs, kRow1[i], cb_row1, (void*)(uintptr_t)i, bw, bh, bx,  by + i*(bh+gy2));
  for (int i=0;i<8;i++)  make_btn(scr_vs, kRow2[i], cb_row2, (void*)(uintptr_t)i, bw, bh, bx+86, by + i*(bh+gy2));

  // 16 "bars" (sélection param)
  int rx=6, ry=236, rw=72, rh=30, rgx=6;
  for (int i=0;i<16;i++){
    int cx = rx + (i%8)*(rw+rgx);
    int cy = ry - ((i/8) * (rh + 4));
    bars[i] = make_btn(scr_vs, kBars[i], cb_bar, (void*)(uintptr_t)i, rw, rh, cx, cy);
  }

  tmr = lv_timer_create(refresh_ui, 60, NULL);
}
