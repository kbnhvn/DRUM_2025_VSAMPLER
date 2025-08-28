// ui_main_menu_lvgl.ino
#include <lvgl.h>

// Ces vues doivent exister (tu les as ou on les a fournis)
extern void build_vsampler_view();
extern void build_files_view();
extern void build_recorder_view();
extern void build_wifi_view();
extern void build_settings_view();
extern void ui_theme_dark_apply(lv_obj_t* root);

static lv_obj_t* scr_menu = nullptr;

static lv_obj_t* make_tile(lv_obj_t* parent, const char* title, const char* subtitle, lv_event_cb_t cb){
  lv_obj_t* b = lv_button_create(parent);
  lv_obj_set_size(b, LV_PCT(48), 76);
  lv_obj_set_style_radius(b, 12, 0);
  lv_obj_set_style_pad_all(b, 10, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t* col = lv_obj_create(b);
  lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_bg_opa(col, LV_OPA_0, 0);
  lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* l1 = lv_label_create(col);
  lv_label_set_text(l1, title);
  lv_obj_set_style_text_font(l1, lv_theme_get_font_large(NULL), 0);

  lv_obj_t* l2 = lv_label_create(col);
  lv_label_set_text(l2, subtitle);

  return b;
}

void build_main_menu(){
  if (scr_menu) { lv_obj_del(scr_menu); scr_menu = nullptr; }

  scr_menu = lv_obj_create(NULL);
  lv_obj_set_size(scr_menu, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_menu, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr_menu, 10, 0);
  lv_scr_load(scr_menu);
  ui_theme_dark_apply(scr_menu);

  // Header
  lv_obj_t* header = lv_obj_create(scr_menu);
  lv_obj_set_width(header, LV_PCT(100));
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(header, 6, 0);
  lv_obj_set_style_pad_column(header, 8, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(header);
  lv_label_set_text(title, "DRUM 2025 — Main");

  lv_obj_t* grow = lv_obj_create(header);
  lv_obj_set_flex_grow(grow, 1);
  lv_obj_set_style_bg_opa(grow, LV_OPA_0, 0);
  lv_obj_clear_flag(grow, LV_OBJ_FLAG_SCROLLABLE);

  // Grid de tuiles 2 colonnes (flex wrap)
  lv_obj_t* grid = lv_obj_create(scr_menu);
  lv_obj_set_width(grid, LV_PCT(100));
  lv_obj_set_height(grid, LV_PCT(100));
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(grid, 8, 0);
  lv_obj_set_style_pad_row(grid, 10, 0);
  lv_obj_set_style_pad_column(grid, 10, 0);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

  make_tile(grid, "VSampler", "Pads + Params",
    [](lv_event_t*){ build_vsampler_view(); });

  make_tile(grid, "Recorder", "Record • Trim • Export",
    [](lv_event_t*){ build_recorder_view(); });

  make_tile(grid, "Files", "Browser WAV",
    [](lv_event_t*){ build_files_view(); });

  make_tile(grid, "Wi-Fi", "Scan • Connect",
    [](lv_event_t*){ build_wifi_view(); });

  make_tile(grid, "Settings", "Audio • MIDI",
    [](lv_event_t*){ build_settings_view(); });

  // Optionnel: About
  make_tile(grid, "About", "Firmware / Build",
    [](lv_event_t*){
      lv_obj_t* m = lv_msgbox_create(NULL);
      lv_msgbox_add_title(m, "About");
      lv_msgbox_add_text(m, "DRUM 2025 VSAMPLER\nLVGL v9 UI\nESP32-S3");
      lv_msgbox_add_footer_button(m, "OK");
      lv_msgbox_open(m);
      ui_theme_dark_apply(m)
      lv_obj_center(m);
    });
}
