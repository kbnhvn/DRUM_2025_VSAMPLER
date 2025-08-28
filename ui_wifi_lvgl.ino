// ui_wifi_lvgl.ino — LVGL v9 compliant
#include <lvgl.h>
#include <WiFi.h>

extern void build_main_menu();
extern void kb_prompt_text(const char* title, bool passwordMode, const char* initial,
                           void (*on_ok)(const char*), void (*on_cancel)());

static lv_obj_t* scr_wifi   = nullptr;
static lv_obj_t* list_ssid  = nullptr;
static lv_obj_t* lbl_status = nullptr;
static lv_timer_t* tmr_conn = nullptr;

static String s_sel_ssid;

/* ---------- Back ---------- */
static void cb_back_wifi(lv_event_t*) {
  if (tmr_conn) { lv_timer_del(tmr_conn); tmr_conn = nullptr; }
  build_main_menu();
}

/* ---------- Connect polling timer ---------- */
static void tmr_conn_cb(lv_timer_t*) {
  wl_status_t st = WiFi.status();
  if (st == WL_CONNECTED) {
    IPAddress a = WiFi.localIP();
    char ip[32];
    snprintf(ip, sizeof(ip), "IP: %u.%u.%u.%u", a[0], a[1], a[2], a[3]);
    lv_label_set_text(lbl_status, ip);
    lv_timer_del(tmr_conn); tmr_conn = nullptr;
  } else if (st == WL_CONNECT_FAILED) {
    lv_label_set_text(lbl_status, "Status: FAILED");
    lv_timer_del(tmr_conn); tmr_conn = nullptr;
  } else {
    lv_label_set_text(lbl_status, "Status: CONNECTING...");
  }
}

static void start_connect_with_pass(const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(50);
  WiFi.begin(s_sel_ssid.c_str(), pass);
  if (tmr_conn) lv_timer_del(tmr_conn);
  tmr_conn = lv_timer_create(tmr_conn_cb, 500, NULL);
}

/* ---------- KB handlers ---------- */
static void kb_ok_handler(const char* txt) {
  if (!txt) return;
  start_connect_with_pass(txt);
}
static void kb_cancel_handler() { /* no-op */ }

/* ---------- Item click: read SSID from list btn ---------- */
static void on_ssid_clicked(lv_event_t* e) {
  lv_obj_t* btn = lv_event_get_target(e);                 // v9: returns lv_obj_t*
  const char* ssid = lv_list_get_btn_text(list_ssid, btn); // v9 API
  if (!ssid || !*ssid) return;
  s_sel_ssid = ssid;
  kb_prompt_text("WiFi password", true, "", kb_ok_handler, kb_cancel_handler);
}

/* ---------- Scan + populate ---------- */
static void refresh_ssid_list() {
  lv_obj_clean(list_ssid); // v9: clean children
  lv_label_set_text(lbl_status, "Scanning...");

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    const char* name = WiFi.SSID(i).c_str();
    lv_obj_t* btn = lv_list_add_btn(list_ssid, LV_SYMBOL_WIFI, name); // v9
    lv_obj_add_event_cb(btn, on_ssid_clicked, LV_EVENT_CLICKED, NULL);
  }
  char t[48]; snprintf(t, sizeof(t), "Found: %d", n);
  lv_label_set_text(lbl_status, t);
}

/* ---------- View ---------- */
void build_wifi_view() {
  if (tmr_conn) { lv_timer_del(tmr_conn); tmr_conn = nullptr; }

  scr_wifi = lv_obj_create(NULL);
  lv_obj_set_size(scr_wifi, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_wifi, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr_wifi, 8, 0);
  lv_scr_load(scr_wifi);

  // Header
  lv_obj_t* header = lv_obj_create(scr_wifi);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(header, 4, 0);
  lv_obj_set_style_pad_column(header, 6, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* lbl = lv_label_create(header); lv_label_set_text(lbl, "Wi-Fi");
  lv_obj_t* spacer = lv_obj_create(header);
  lv_obj_set_flex_grow(spacer, 1);
  lv_obj_set_style_bg_opa(spacer, LV_OPA_0, 0);
  lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* btnBack = lv_button_create(header);
  lv_obj_add_event_cb(btnBack, cb_back_wifi, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(btnBack); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  // Toolbar
  lv_obj_t* tb = lv_obj_create(scr_wifi);
  lv_obj_set_width(tb, LV_PCT(100));
  lv_obj_set_flex_flow(tb, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(tb, 6, 0);
  lv_obj_set_style_pad_column(tb, 6, 0);
  lv_obj_set_style_pad_row(tb, 6, 0);
  lv_obj_clear_flag(tb, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* bScan = lv_button_create(tb);
  lv_obj_add_event_cb(bScan, [](lv_event_t*){ refresh_ssid_list(); }, LV_EVENT_CLICKED, NULL);
  lv_obj_t* lS = lv_label_create(bScan); lv_label_set_text(lS, "SCAN"); lv_obj_center(lS);

  // Status
  lbl_status = lv_label_create(scr_wifi);
  lv_label_set_text(lbl_status, "Idle");

  // List
  list_ssid = lv_list_create(scr_wifi);
  lv_obj_set_size(list_ssid, LV_PCT(100), LV_PCT(100));

  // Initial scan
  refresh_ssid_list();
}
