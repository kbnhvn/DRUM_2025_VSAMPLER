// ui_theme_dark.h/.ino (LVGL v9)
#include <lvgl.h>

// -------- Palette (modifiable) --------
static const lv_color_t C_BG       = lv_color_hex(0x0F1115);
static const lv_color_t C_SURFACE  = lv_color_hex(0x151922);
static const lv_color_t C_BORDER   = lv_color_hex(0x262A35);
static const lv_color_t C_TEXT     = lv_color_hex(0xE5E7EB);
static const lv_color_t C_TEXT_MUT = lv_color_hex(0x94A3B8);
static const lv_color_t C_ACCENT   = lv_color_hex(0x6CA0FF);
static const lv_color_t C_ACCENT2  = lv_color_hex(0x22C55E);
static const lv_color_t C_WARN     = lv_color_hex(0xF59E0B);
static const lv_color_t C_ERR      = lv_color_hex(0xEF4444);

// -------- Styles globaux réutilisables --------
static bool        g_theme_inited = false;
static lv_style_t  st_screen, st_noradius_any, st_text, st_card;
static lv_style_t  st_btn, st_btn_pr, st_btn_chk, st_btn_dis;
static lv_style_t  st_slider_main, st_slider_ind, st_slider_knob;
static lv_style_t  st_list_btn, st_list_btn_pr, st_dd, st_sw_main, st_sw_ind;

static void ui_theme_dark_init_once() {
  if (g_theme_inited) return;
  g_theme_inited = true;

  // Screen / root
  lv_style_init(&st_screen);
  lv_style_set_bg_color(&st_screen, C_BG);
  lv_style_set_bg_opa(&st_screen, LV_OPA_COVER);

  // Radius = 0 partout (toutes parts / états)
  lv_style_init(&st_noradius_any);
  lv_style_set_radius(&st_noradius_any, 0); // appliqué avec LV_PART_ANY | LV_STATE_ANY

  // Texte global
  lv_style_init(&st_text);
  lv_style_set_text_color(&st_text, C_TEXT);
  // lv_style_set_text_font(&st_text, &lv_font_montserrat_16); // décommente si tu veux forcer une taille

  // “Card” / conteneurs
  lv_style_init(&st_card);
  lv_style_set_bg_color(&st_card, C_SURFACE);
  lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
  lv_style_set_border_color(&st_card, C_BORDER);
  lv_style_set_border_width(&st_card, 1);
  lv_style_set_pad_all(&st_card, 8);

  // Boutons
  lv_style_init(&st_btn);
  lv_style_set_bg_color(&st_btn, lv_color_mix(C_SURFACE, C_BG, LV_OPA_60));
  lv_style_set_bg_opa(&st_btn, LV_OPA_COVER);
  lv_style_set_border_color(&st_btn, C_BORDER);
  lv_style_set_border_width(&st_btn, 1);
  lv_style_set_text_color(&st_btn, C_TEXT);
  lv_style_set_pad_hor(&st_btn, 10);
  lv_style_set_pad_ver(&st_btn, 8);

  lv_style_init(&st_btn_pr);
  lv_style_set_bg_color(&st_btn_pr, lv_color_mix(C_SURFACE, C_ACCENT, LV_OPA_20));
  lv_style_set_translate_y(&st_btn_pr, 1);

  lv_style_init(&st_btn_chk);
  lv_style_set_bg_color(&st_btn_chk, lv_color_mix(C_SURFACE, C_ACCENT, LV_OPA_40));
  lv_style_set_border_color(&st_btn_chk, C_ACCENT);

  lv_style_init(&st_btn_dis);
  lv_style_set_bg_color(&st_btn_dis, lv_color_mix(C_SURFACE, C_BG, LV_OPA_40));
  lv_style_set_text_color(&st_btn_dis, C_TEXT_MUT);

  // Slider
  lv_style_init(&st_slider_main);
  lv_style_set_bg_color(&st_slider_main, lv_color_hex(0x202534));
  lv_style_set_bg_opa(&st_slider_main, LV_OPA_COVER);
  lv_style_set_border_color(&st_slider_main, C_BORDER);
  lv_style_set_border_width(&st_slider_main, 1);

  lv_style_init(&st_slider_ind);
  lv_style_set_bg_color(&st_slider_ind, C_ACCENT);
  lv_style_set_bg_opa(&st_slider_ind, LV_OPA_COVER);

  lv_style_init(&st_slider_knob);
  lv_style_set_bg_color(&st_slider_knob, C_TEXT);
  lv_style_set_pad_all(&st_slider_knob, 6);

  // List button (pour lv_list)
  lv_style_init(&st_list_btn);
  lv_style_set_bg_color(&st_list_btn, lv_color_mix(C_SURFACE, C_BG, LV_OPA_40));
  lv_style_set_border_color(&st_list_btn, C_BORDER);
  lv_style_set_border_width(&st_list_btn, 1);
  lv_style_set_text_color(&st_list_btn, C_TEXT);
  lv_style_set_pad_all(&st_list_btn, 8);

  lv_style_init(&st_list_btn_pr);
  lv_style_set_bg_color(&st_list_btn_pr, lv_color_mix(C_SURFACE, C_ACCENT, LV_OPA_20));

  // Dropdown
  lv_style_init(&st_dd);
  lv_style_set_bg_color(&st_dd, C_SURFACE);
  lv_style_set_border_color(&st_dd, C_BORDER);
  lv_style_set_border_width(&st_dd, 1);
  lv_style_set_text_color(&st_dd, C_TEXT);

  // Switch
  lv_style_init(&st_sw_main);
  lv_style_set_bg_color(&st_sw_main, lv_color_hex(0x202534));
  lv_style_set_border_color(&st_sw_main, C_BORDER);
  lv_style_set_border_width(&st_sw_main, 1);

  lv_style_init(&st_sw_ind);
  lv_style_set_bg_color(&st_sw_ind, C_ACCENT2);
}

// -------- Application au root / screen --------
static inline void ui_theme_dark_apply_root(lv_obj_t* root) {
  ui_theme_dark_init_once();
  lv_obj_add_style(root, &st_screen, 0);

  // radius 0 pour tout ce qui vit sous root
  lv_obj_add_style(root, &st_noradius_any, LV_PART_ANY | LV_STATE_ANY);

  // texte par défaut pour les descendants (héritage)
  lv_obj_add_style(root, &st_text, 0);

  // scrollbar (optionnel)
  lv_obj_set_style_scrollbar_mode(root, LV_SCROLLBAR_MODE_AUTO, 0);
  lv_obj_set_style_scrollbar_color(root, lv_color_hex(0x2F3646), 0);
  lv_obj_set_style_scrollbar_opa(root, LV_OPA_80, 0);
}

// -------- Helpers par widget --------
static inline void ui_style_card(lv_obj_t* obj) {
  ui_theme_dark_init_once();
  lv_obj_add_style(obj, &st_card, 0);
}
static inline void ui_style_button(lv_obj_t* btn) {
  ui_theme_dark_init_once();
  lv_obj_add_style(btn, &st_btn, 0);
  lv_obj_add_style(btn, &st_btn_pr, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &st_btn_chk, LV_STATE_CHECKED);
  lv_obj_add_style(btn, &st_btn_dis, LV_STATE_DISABLED);
}
static inline void ui_style_slider(lv_obj_t* s) {
  ui_theme_dark_init_once();
  lv_obj_add_style(s, &st_slider_main, LV_PART_MAIN);
  lv_obj_add_style(s, &st_slider_ind,  LV_PART_INDICATOR);
  lv_obj_add_style(s, &st_slider_knob, LV_PART_KNOB);
}
static inline void ui_style_list(lv_obj_t* list) {
  ui_theme_dark_init_once();
  // list container
  lv_obj_add_style(list, &st_card, 0);
  // à appeler aussi sur chaque item créé :
  //   lv_obj_t* b = lv_list_add_button(list, LV_SYMBOL_WIFI, "SSID");
  //   ui_style_list_btn(b);
}
static inline void ui_style_list_btn(lv_obj_t* list_btn) {
  ui_theme_dark_init_once();
  lv_obj_add_style(list_btn, &st_list_btn, 0);
  lv_obj_add_style(list_btn, &st_list_btn_pr, LV_STATE_PRESSED);
}
static inline void ui_style_dropdown(lv_obj_t* dd) {
  ui_theme_dark_init_once();
  lv_obj_add_style(dd, &st_dd, 0);
}
static inline void ui_style_switch(lv_obj_t* sw) {
  ui_theme_dark_init_once();
  lv_obj_add_style(sw, &st_sw_main, LV_PART_MAIN);
  lv_obj_add_style(sw, &st_sw_ind,  LV_PART_INDICATOR | LV_STATE_CHECKED);
}
