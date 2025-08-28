// ui_files_lvgl.ino
#include <lvgl.h>
#include <SD.h>

extern void build_main_menu();
extern bool loadWavToSlot(const char* path, uint8_t slot);
extern uint8_t selected_sound;

static lv_obj_t* scr_files = nullptr;
static lv_obj_t* list_files = nullptr;
static lv_obj_t* lbl_path = nullptr;
static lv_timer_t* tmr_files = nullptr;

static String g_dir = "/samples";
static String sel_name;

static void cb_back_files(lv_event_t*) { build_main_menu(); }

static void list_refresh() {
  lv_list_clear(list_files);
  if (!SD.exists(g_dir)) SD.mkdir(g_dir.c_str());

  File dir = SD.open(g_dir.c_str());
  if (!dir) return;

  while (1) {
    File e = dir.openNextFile();
    if (!e) break;
    if (e.isDirectory()) {
      lv_obj_t* btn = lv_list_add_button(list_files, LV_SYMBOL_DIRECTORY, e.name());
      lv_obj_add_event_cb(btn, [](lv_event_t* ev){
        const char* name = lv_list_get_button_text(list_files, (lv_obj_t*)lv_event_get_target(ev));
        if (!name) return;
        sel_name = name;
      }, LV_EVENT_CLICKED, NULL);
    } else {
      // Ne montrer que .wav
      String n = e.name();
      n.toLowerCase();
      if (n.endsWith(".wav")) {
        lv_obj_t* btn = lv_list_add_button(list_files, LV_SYMBOL_AUDIO, e.name());
        lv_obj_add_event_cb(btn, [](lv_event_t* ev){
          const char* name = lv_list_get_button_text(list_files, (lv_obj_t*)lv_event_get_target(ev));
          if (!name) return;
          sel_name = name;
        }, LV_EVENT_CLICKED, NULL);
      }
    }
    e.close();
  }
  dir.close();

  char p[96]; snprintf(p, sizeof(p), "Path: %s", g_dir.c_str());
  lv_label_set_text(lbl_path, p);
}

static void cb_open_dir(lv_event_t*) {
  if (sel_name.length()==0) return;
  String np = g_dir + "/" + sel_name;
  File f = SD.open(np.c_str());
  if (f && f.isDirectory()) { g_dir = np; list_refresh(); }
  if (f) f.close();
}

static void cb_up_dir(lv_event_t*) {
  if (g_dir == "/") return;
  int slash = g_dir.lastIndexOf('/');
  if (slash <= 0) g_dir = "/";
  else g_dir = g_dir.substring(0, slash);
  list_refresh();
}

static void cb_load(lv_event_t*) {
  if (sel_name.length()==0) return;
  String path = g_dir + "/" + sel_name;
  loadWavToSlot(path.c_str(), selected_sound);
}

static void cb_rename(lv_event_t*) {
  if (sel_name.length()==0) return;
  String oldp = g_dir + "/" + sel_name;
  extern void kb_prompt_text(const char*, bool, const char*, void (*)(const char*), void(*)());
  kb_prompt_text("Rename", false, sel_name.c_str(), [](const char* txt){
    if (!txt || !*txt) return;
    String newp = String("/samples/") + txt;
    if (SD.exists(newp)) SD.remove(newp);
    String oldp = String("/samples/") + sel_name; // sel_name global au moment du click
    SD.rename(oldp, newp);
  }, nullptr);
  // Après renommage manuel, rafraîchir
  list_refresh();
}

static void cb_delete(lv_event_t*) {
  if (sel_name.length()==0) return;
  String p = g_dir + "/" + sel_name;
  SD.remove(p.c_str());
  sel_name = "";
  list_refresh();
}

void build_files_view() {
  if (tmr_files) { lv_timer_del(tmr_files); tmr_files = nullptr; }

  scr_files = lv_obj_create(NULL);
  lv_obj_set_size(scr_files, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_files, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr_files, 8, 0);
  lv_scr_load(scr_files);

  // Header
  lv_obj_t* header = lv_obj_create(scr_files);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(header, 4, 0);
  lv_obj_set_style_pad_column(header, 6, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* lbl = lv_label_create(header); lv_label_set_text(lbl, "Files");
  lv_obj_t* spacer = lv_obj_create(header); lv_obj_set_flex_grow(spacer, 1);
  lv_obj_set_style_bg_opa(spacer, LV_OPA_0, 0);
  lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* btnBack = lv_button_create(header);
  lv_obj_add_event_cb(btnBack, cb_back_files, LV_EVENT_CLICKED, NULL);
  lv_obj_t* bl = lv_label_create(btnBack); lv_label_set_text(bl, "BACK"); lv_obj_center(bl);

  // Toolbar
  lv_obj_t* tb = lv_obj_create(scr_files);
  lv_obj_set_width(tb, LV_PCT(100));
  lv_obj_set_flex_flow(tb, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(tb, 6, 0);
  lv_obj_set_style_pad_column(tb, 6, 0);
  lv_obj_set_style_pad_row(tb, 6, 0);
  lv_obj_clear_flag(tb, LV_OBJ_FLAG_SCROLLABLE);

  auto mk = [&](const char* t, lv_event_cb_t cb){
    lv_obj_t* b = lv_button_create(tb);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l=lv_label_create(b); lv_label_set_text(l,t); lv_obj_center(l);
    return b;
  };
  mk("OPEN", cb_open_dir);
  mk("UP",   cb_up_dir);
  mk("LOAD", cb_load);
  mk("REN",  cb_rename);
  mk("DEL",  cb_delete);

  // Path label
  lbl_path = lv_label_create(scr_files);
  lv_label_set_text(lbl_path, "Path: /samples");

  // List
  list_files = lv_list_create(scr_files);
  lv_obj_set_size(list_files, LV_PCT(100), LV_PCT(100));
  list_refresh();
}
