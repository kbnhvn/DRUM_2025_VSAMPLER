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

// Tuile = simple bouton avec un seul label (1 ligne, pas de sous-titre)
static lv_obj_t* make_tile(lv_obj_t* parent, const char* title, lv_event_cb_t cb)
{
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_set_size(btn, 150, 72);                 // taille “tuile”
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* l = lv_label_create(btn);
  lv_label_set_text(l, title ? title : "");
  lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP); // 1 ligne, évite l’effet “tronqué dessous”
  lv_obj_center(l);
  return btn;
}

void build_main_menu()
{
  if (scr_menu) { lv_obj_del(scr_menu); scr_menu = nullptr; }
  scr_menu = lv_obj_create(NULL);
  lv_scr_load(scr_menu);

  // Écran plein en grille : pas de header, pas de scroll
  lv_obj_set_size(scr_menu, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(scr_menu, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(scr_menu, 8, 0);
  lv_obj_set_style_pad_row(scr_menu, 8, 0);
  lv_obj_set_style_pad_column(scr_menu, 8, 0);
  lv_obj_set_scrollbar_mode(scr_menu, LV_SCROLLBAR_MODE_OFF);

  make_tile(scr_menu, "VSampler", [](lv_event_t*){ build_vsampler_view(); });
  make_tile(scr_menu, "Files",    [](lv_event_t*){ build_files_view();    });
  make_tile(scr_menu, "Recorder", [](lv_event_t*){ build_recorder_view(); });
  make_tile(scr_menu, "Wi-Fi",    [](lv_event_t*){ build_wifi_view();     });
  make_tile(scr_menu, "Settings", [](lv_event_t*){ build_settings_view(); });
  make_tile(scr_menu, "About",    [](lv_event_t*){
    lv_obj_t* m = lv_msgbox_create(NULL);
    lv_msgbox_add_title(m, "About");
    lv_msgbox_add_text(m, "DRUM 2025 VSAMPLER\nLVGL v9 UI\nESP32-S3");
    lv_obj_t* b_ok = lv_msgbox_add_footer_button(m, "OK");
    lv_obj_add_event_cb(b_ok, [](lv_event_t* e){
      lv_obj_del(lv_obj_get_parent(lv_event_get_target_obj(e)));
    }, LV_EVENT_CLICKED, NULL);
    lv_msgbox_add_close_button(m);
    lv_obj_center(m);
    ui_theme_dark_apply(m);
  });
  ui_theme_dark_apply(scr_menu);
}
