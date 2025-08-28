// ui_settings_lvgl.ino
#include <lvgl.h>
#include <Arduino.h>

extern void build_main_menu();

// Backends existants
extern void AudioOut_setRoute(int r);        // 0=JACK, 1=SPK (routage)
extern void AudioOut_setVolume(uint8_t pct); // 0..100
extern uint8_t g_midiChannel;                // 1..16 (selon ton implementation)

// Petites helpers UI
static lv_obj_t* scr_set   = nullptr;
static lv_obj_t* lbl_info  = nullptr;
static lv_obj_t* sw_wifi   = nullptr; // bool UI only (si tu veux relier à ton save JSON)
static lv_obj_t* dd_route  = nullptr;
static lv_obj_t* sld_vol   = nullptr;
static lv_obj_t* dd_midi   = nullptr;

static uint8_t local_midi = 1;
static int     local_route = 0;
static int     local_vol   = 80;
static bool    local_wifi  = false;

static void apply_and_refresh(){
  // Applique
  AudioOut_setRoute(local_route);
  AudioOut_setVolume((uint8_t)local_vol);
  g_midiChannel = (uint8_t)local_midi;

  // Affiche
  char info[64];
  snprintf(info, sizeof(info), "MIDI CH:%u  VOL:%u  OUT:%s",
           (unsigned)g_midiChannel, (unsigned)local_vol, local_route? "SPK":"JACK");
  lv_label_set_text(lbl_info, info);
}

static void cb_back_settings(lv_event_t*) { build_main_menu(); }

static void on_wifi_toggle(lv_event_t* e){
  lv_obj_t* t=(lv_obj_t*)lv_event_get_target(e);
  local_wifi = lv_obj_has_state(t, LV_STATE_CHECKED);
  // Tu peux persister en JSON ici si tu veux
}

static void on_route_change(lv_event_t* e){
  lv_obj_t* t=(lv_obj_t*)lv_event_get_target(e);
  local_route = lv_dropdown_get_selected(t);   // 0..1
  apply_and_refresh();
}

static void on_vol_change(lv_event_t* e){
  lv_obj_t* t=(lv_obj_t*)lv_event_get_target(e);
  local_vol = lv_slider_get_value(t);
  apply_and_refresh();
}

static void on_midi_change(lv_event_t* e){
  lv_obj_t* t=(lv_obj_t*)lv_event_get_target(e);
  local_midi = (uint8_t)(lv_dropdown_get_selected(t)+1); // 1..16
  apply_and_refresh();
}

static lv_obj_t* make_row(lv_obj_t* parent, const char* label){
  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_set_width(row, LV_PCT(100));
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(row, 6, 0);
  lv_obj_set_style_pad_column(row, 10, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* l = lv_label_create(row); lv_label_set_text(l, label);
  lv_obj_t* spacer = lv_obj_create(row); lv_obj_set_flex_grow(spacer, 1);
  lv_obj_set_style_bg_opa(spacer, LV_OPA_0, 0); lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
  return row;
}

void build_settings_view(){
  if (scr_set) { lv_obj_del(scr_set); scr_set=nullptr; }

  // init valeurs locales (si tu as de la persistance, lis-la ici)
  local_midi = g_midiChannel ? g_midiChannel : 1;
  local_vol  = 80;
  local_route= 0;

  scr_set = lv_obj_create(NULL);
  lv_obj_set_size(scr_set, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_set, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr_set, 10, 0);
  lv_scr_load(scr_set);

  // Header
  lv_obj_t* header = lv_obj_create(scr_set);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(header, 6, 0);
  lv_obj_set_style_pad_column(header, 8, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* t = lv_label_create(header); lv_label_set_text(t, "Settings");

  lv_obj_t* grow = lv_obj_create(header); lv_obj_set_flex_grow(grow, 1);
  lv_obj_set_style_bg_opa(grow, LV_OPA_0, 0);
  lv_obj_clear_flag(grow, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* btnBack = lv_button_create(header);
  lv_obj_add_event_cb(btnBack, cb_back_settings, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(btnBack); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  // Corps (colonnes flex)
  lv_obj_t* body = lv_obj_create(scr_set);
  lv_obj_set_width(body, LV_PCT(100));
  lv_obj_set_height(body, LV_PCT(100));
  lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(body, 8, 0);
  lv_obj_set_style_pad_row(body, 8, 0);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

  // WiFi toggle
  {
    lv_obj_t* row = make_row(body, "Wi-Fi");
    sw_wifi = lv_switch_create(row);
    if (local_wifi) lv_obj_add_state(sw_wifi, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_wifi, on_wifi_toggle, LV_EVENT_VALUE_CHANGED, NULL);
  }

  // Audio route (JACK / SPK)
  {
    lv_obj_t* row = make_row(body, "Audio Out");
    dd_route = lv_dropdown_create(row);
    lv_dropdown_set_options_static(dd_route, "JACK\nSPK");
    lv_dropdown_set_selected(dd_route, local_route);
    lv_obj_add_event_cb(dd_route, on_route_change, LV_EVENT_VALUE_CHANGED, NULL);
  }

  // Volume (0..100)
  {
    lv_obj_t* row = make_row(body, "Volume");
    sld_vol = lv_slider_create(row);
    lv_obj_set_width(sld_vol, LV_PCT(60));
    lv_slider_set_range(sld_vol, 0, 100);
    lv_slider_set_value(sld_vol, local_vol, LV_ANIM_OFF);
    lv_obj_add_event_cb(sld_vol, on_vol_change, LV_EVENT_VALUE_CHANGED, NULL);
  }

  // MIDI channel (1..16)
  {
    lv_obj_t* row = make_row(body, "MIDI Channel");
    dd_midi = lv_dropdown_create(row);

    // Crée "1\n2\n...\n16" statique
    static char opts[64];
    char* p = opts; size_t left = sizeof(opts);
    for (int i=1;i<=16;i++){
      int n = snprintf(p, left, (i<16)?"%d\n":"%d", i);
      p += n; left -= n;
    }
    lv_dropdown_set_options(dd_midi, opts);
    lv_dropdown_set_selected(dd_midi, local_midi-1);
    lv_obj_add_event_cb(dd_midi, on_midi_change, LV_EVENT_VALUE_CHANGED, NULL);
  }

  // Info & apply
  lbl_info = lv_label_create(body);
  apply_and_refresh();
}
