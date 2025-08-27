// ui_main_menu_lvgl.ino
#include <lvgl.h>

extern void build_vsampler_view();
extern void build_recorder_view();
extern void build_files_view();
extern void build_wifi_view();
extern void build_settings_view();

static lv_obj_t* make_tile(lv_obj_t* parent, const char* txt, lv_event_cb_t cb, int x, int y){
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_set_size(btn, 100, 60);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* lbl = lv_label_create(btn);
  lv_label_set_text(lbl, txt);
  lv_obj_center(lbl);
  return btn;
}

static void cb_to_vsampler(lv_event_t*){ build_vsampler_view(); }
static void cb_to_rec(lv_event_t*)     { build_recorder_view(); }
static void cb_to_files(lv_event_t*)   { build_files_view(); }
static void cb_to_wifi(lv_event_t*)    { build_wifi_view(); }
static void cb_to_settings(lv_event_t*){ build_settings_view(); }

void build_main_menu(){
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_scr_load(scr);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Main Menu");
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 6, 6);

  make_tile(scr, "VSampler", cb_to_vsampler,   10, 30);
  make_tile(scr, "Recorder", cb_to_rec,       120, 30);
  make_tile(scr, "Files",    cb_to_files,      10, 100);
  make_tile(scr, "Wi-Fi",    cb_to_wifi,      120, 100);
  make_tile(scr, "Settings", cb_to_settings,   10, 170);
}
