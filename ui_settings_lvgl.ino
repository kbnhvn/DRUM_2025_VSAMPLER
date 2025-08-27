// ui_settings_lvgl.ino
#include <lvgl.h>

extern void AudioOut_setVolume(uint8_t);
static uint8_t g_vol = 80;
extern void AudioOut_setRoute(int route); // 0=jack 1=speaker
static int g_route = 0;

static lv_obj_t* scr_set;

static void cb_back_settings(lv_event_t*){
   extern void build_main_menu();
   build_main_menu();
}

void build_settings_view(){
  scr_set = lv_obj_create(NULL);
  lv_scr_load(scr_set);

  lv_obj_t* ttl = lv_label_create(scr_set);
  lv_label_set_text(ttl, "Settings");
  lv_obj_align(ttl, LV_ALIGN_TOP_LEFT, 6, 6);

  // BACK
  lv_obj_t* back = lv_button_create(scr_set);
  lv_obj_set_size(back, 70, 32);
  lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -6, 6);
  lv_obj_add_event_cb(back, cb_back_settings, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(back); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  auto mk = [&](const char* t, lv_event_cb_t cb, int x, int y){
    lv_obj_t* b = lv_button_create(scr_set);
    lv_obj_set_size(b, 120, 36);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l=lv_label_create(b); lv_label_set_text(l, t); lv_obj_center(l);
    return b;
  };

  mk("VOL-", [](lv_event_t*){ if(g_vol>0) g_vol--; AudioOut_setVolume(g_vol); }, 10, 40);
  mk("VOL+", [](lv_event_t*){ if(g_vol<100) g_vol++; AudioOut_setVolume(g_vol); }, 140, 40);
  mk("OUT:Jack", [](lv_event_t*){ g_route=0; AudioOut_setRoute(g_route); }, 10, 86);
  mk("OUT:Spk",  [](lv_event_t*){ g_route=1; AudioOut_setRoute(g_route); }, 140, 86);
}
