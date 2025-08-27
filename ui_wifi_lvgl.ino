// ui_wifi_lvgl.ino
#include <lvgl.h>
#include <WiFi.h>

static lv_obj_t* scr_wifi;
static lv_obj_t* list_ssid;
static lv_obj_t* lbl_status;
static lv_timer_t* tmr_conn = nullptr;

static lv_obj_t* modal = nullptr;
static lv_obj_t* kb = nullptr;
static lv_obj_t* ta = nullptr;
static char g_selected_ssid[40] = {0};

extern void kb_prompt_text(const char* title, bool passwordMode, const char* initial,
                           void (*on_ok)(const char*), void (*on_cancel)());

// Connect to Wi-Fi
static void start_connect_with_pass(const char* pass){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(50);
  WiFi.begin(g_selected_ssid, pass);
  lv_label_set_text(lbl_status, "Status: CONNECTING...");
  if (tmr_conn) lv_timer_del(tmr_conn);
  tmr_conn = lv_timer_create([](){
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
      IPAddress a = WiFi.localIP();
      char ip[32]; snprintf(ip, sizeof(ip), "IP: %u.%u.%u.%u", a[0], a[1], a[2], a[3]);
      lv_label_set_text(lbl_status, ip);
      lv_timer_del(tmr_conn); tmr_conn = nullptr;
    } else if (st == WL_CONNECT_FAILED) {
      lv_label_set_text(lbl_status, "Status: FAILED");
      lv_timer_del(tmr_conn); tmr_conn = nullptr;
    } else {
      lv_label_set_text(lbl_status, "Status: CONNECTING...");
    }
  }, 500, NULL);
}

// Keyboard event callbacks
static void kb_ok_handler(const char* txt){
  if (txt && *txt) start_connect_with_pass(txt);
  kb_close();
}

static void kb_cancel_handler(){ kb_close(); }

static void open_keyboard_for_pass(const char* ssid){
  strncpy(g_selected_ssid, ssid, sizeof(g_selected_ssid)-1);
  char title[64]; snprintf(title, sizeof(title), "Pass for: %s", ssid);
  kb_prompt_text(title, /*passwordMode=*/true, /*initial=*/"", kb_ok_handler, kb_cancel_handler);
}

static void refresh_ssid_list(){
  lv_obj_clean(list_ssid);
  lv_label_set_text(lbl_status, "Scanning...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);
  int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    lv_label_set_text(lbl_status, "No networks");
    return;
  }
  for (int i = 0; i < n; i++){
    String s = WiFi.SSID(i);
    lv_obj_t* btn = lv_list_add_button(list_ssid, LV_SYMBOL_WIFI, s.c_str());
    lv_obj_add_event_cb(btn, [](lv_event_t* ev){
      const char* ssid = lv_list_get_button_text(list_ssid, lv_event_get_target(ev));
      if (ssid) open_keyboard_for_pass(ssid);
    }, LV_EVENT_CLICKED, NULL);
  }
  lv_label_set_text(lbl_status, "Select SSID");
}

static void cb_back(lv_event_t*){
  extern void build_main_menu();
  if (tmr_conn) { lv_timer_del(tmr_conn); tmr_conn = nullptr; }
  build_main_menu();
}

void build_wifi_view(){
  if (tmr_conn) { lv_timer_del(tmr_conn); tmr_conn = nullptr; }

  scr_wifi = lv_obj_create(NULL);
  lv_scr_load(scr_wifi);

  lv_obj_t* ttl = lv_label_create(scr_wifi);
  lv_label_set_text(ttl, "Wi-Fi");
  lv_obj_align(ttl, LV_ALIGN_TOP_LEFT, 6, 6);

  // BACK
  lv_obj_t* back = lv_button_create(scr_wifi);
  lv_obj_set_size(back, 70, 32);
  lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -6, 6);
  lv_obj_add_event_cb(back, cb_back, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(back); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  lbl_status = lv_label_create(scr_wifi);
  lv_label_set_text(lbl_status, "Status: IDLE");
  lv_obj_align(lbl_status, LV_ALIGN_TOP_RIGHT, -86, 6);

  list_ssid = lv_list_create(scr_wifi);
  lv_obj_set_size(list_ssid, 460, 200);
  lv_obj_align(list_ssid, LV_ALIGN_TOP_MID, 0, 36);

  auto mk = [&](const char* t, lv_event_cb_t cb, int x){
    lv_obj_t* b = lv_button_create(scr_wifi);
    lv_obj_set_size(b, 110, 36);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, x, -8);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l=lv_label_create(b); lv_label_set_text(l, t); lv_obj_center(l);
    return b;
  };
  mk("SCAN", [](lv_event_t*){ refresh_ssid_list(); }, 10);

  refresh_ssid_list();
}
