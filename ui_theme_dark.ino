#include <lvgl.h>

/* ========= Palette sombre (modifiable) ========= */
static const lv_color_t C_BG      = lv_color_hex(0x121212);
static const lv_color_t C_CARD    = lv_color_hex(0x181818);
static const lv_color_t C_FG      = lv_color_hex(0xE6E6E6);
static const lv_color_t C_MUTED   = lv_color_hex(0xA0A0A0);
static const lv_color_t C_ACCENT  = lv_color_hex(0x4FC3F7);
static const lv_color_t C_BORDER  = lv_color_hex(0x2A2A2A);
static const lv_color_t C_FOCUS   = lv_color_hex(0x76E4F7);

/* ========= Styles globaux ========= */
static bool s_theme_inited = false;

static lv_style_t st_scr;            // fond d’écran
static lv_style_t st_cont;           // conteneurs/flex/grid
static lv_style_t st_card;           // panneaux
static lv_style_t st_label;          // texte
static lv_style_t st_btn_main;       // bouton idle
static lv_style_t st_btn_pr;         // bouton pressed/checked
static lv_style_t st_knob;           // knob sliders
static lv_style_t st_slider_main;    // rail slider
static lv_style_t st_slider_indic;   // indicateur slider
static lv_style_t st_list;           // list container
static lv_style_t st_list_btn;       // list button
static lv_style_t st_scrollbar;      // scrollbars

/* ————————— helpers ————————— */
static inline void no_radius(lv_style_t* s){ lv_style_set_radius(s, 0); }
static inline void flat_border(lv_style_t* s){
  lv_style_set_border_width(s, 1);
  lv_style_set_border_color(s, C_BORDER);
}

/* ========= Init thème (à appeler 1 seule fois) ========= */
extern void ui_theme_dark_init(void)
{
  if (s_theme_inited) return;
  s_theme_inited = true;

  lv_style_init(&st_scr);
  lv_style_set_bg_color(&st_scr, C_BG);
  lv_style_set_text_color(&st_scr, C_FG);
  no_radius(&st_scr);

  lv_style_init(&st_cont);
  lv_style_set_bg_color(&st_cont, C_BG);
  lv_style_set_bg_opa(&st_cont, LV_OPA_COVER);
  flat_border(&st_cont);
  no_radius(&st_cont);
  lv_style_set_pad_all(&st_cont, 6);
  lv_style_set_pad_row(&st_cont, 6);
  lv_style_set_pad_column(&st_cont, 6);

  lv_style_init(&st_card);
  lv_style_set_bg_color(&st_card, C_CARD);
  lv_style_set_bg_opa(&st_card, LV_OPA_COVER);
  flat_border(&st_card);
  no_radius(&st_card);
  lv_style_set_pad_all(&st_card, 8);

  lv_style_init(&st_label);
  lv_style_set_text_color(&st_label, C_FG);

  lv_style_init(&st_btn_main);
  lv_style_set_bg_color(&st_btn_main, C_CARD);
  lv_style_set_bg_opa(&st_btn_main, LV_OPA_COVER);
  flat_border(&st_btn_main);
  no_radius(&st_btn_main);
  lv_style_set_pad_all(&st_btn_main, 8);
  lv_style_set_text_color(&st_btn_main, C_FG);

  lv_style_init(&st_btn_pr);
  lv_style_set_bg_color(&st_btn_pr, C_ACCENT);
  lv_style_set_text_color(&st_btn_pr, lv_color_black());
  no_radius(&st_btn_pr);

  lv_style_init(&st_slider_main);
  lv_style_set_bg_color(&st_slider_main, lv_color_hex(0x303030));
  no_radius(&st_slider_main);

  lv_style_init(&st_slider_indic);
  lv_style_set_bg_color(&st_slider_indic, C_ACCENT);
  no_radius(&st_slider_indic);

  lv_style_init(&st_knob);
  lv_style_set_bg_color(&st_knob, C_FG);
  no_radius(&st_knob);
  lv_style_set_border_width(&st_knob, 0);

  lv_style_init(&st_list);
  lv_style_set_bg_color(&st_list, C_BG);
  lv_style_set_text_color(&st_list, C_FG);
  flat_border(&st_list);
  no_radius(&st_list);

  lv_style_init(&st_list_btn);
  lv_style_set_bg_color(&st_list_btn, C_CARD);
  lv_style_set_text_color(&st_list_btn, C_FG);
  flat_border(&st_list_btn);
  no_radius(&st_list_btn);
  lv_style_set_pad_all(&st_list_btn, 8);

  lv_style_init(&st_scrollbar);
  // apparence de la barre (la “thumb”)
  lv_style_set_bg_color(&st_scrollbar, lv_color_hex(0x2F3646));
  lv_style_set_bg_opa(&st_scrollbar, LV_OPA_80);
  lv_style_set_radius(&st_scrollbar, 0);
  // épaisseur (largeur du scrollbar) – en v9, width sur le PART_SCROLLBAR
  lv_style_set_width(&st_scrollbar, 6);
}

/* ========= Application par type =========
   On détecte la classe et on pousse les styles + états.
   NB: LVGL 9 → lv_obj_check_type(obj, &lv_button_class) etc.
*/
static void apply_for_type(lv_obj_t* obj)
{
  /* base style to everything */
  lv_obj_add_style(obj, &st_cont, LV_PART_MAIN);
  lv_obj_add_style(obj, &st_scrollbar, LV_PART_SCROLLBAR);

  /* labels */
  if (lv_obj_check_type(obj, &lv_label_class)) {
    lv_obj_add_style(obj, &st_label, LV_PART_MAIN);
    return;
  }

  /* buttons */
  if (lv_obj_check_type(obj, &lv_button_class)) {
    lv_obj_add_style(obj, &st_btn_main, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &st_btn_pr,   LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(obj, &st_btn_pr,   LV_PART_MAIN | LV_STATE_CHECKED);
    return;
  }

  /* slider */
  if (lv_obj_check_type(obj, &lv_slider_class)) {
    lv_obj_add_style(obj, &st_slider_main,  LV_PART_MAIN);
    lv_obj_add_style(obj, &st_slider_indic, LV_PART_INDICATOR);
    lv_obj_add_style(obj, &st_knob,         LV_PART_KNOB);
    return;
  }

  /* list container */
  if (lv_obj_check_type(obj, &lv_list_class)) {
    lv_obj_add_style(obj, &st_list, LV_PART_MAIN);
    return;
  }

  /* list button item */
  if (lv_obj_check_type(obj, &lv_list_button_class)) {
    lv_obj_add_style(obj, &st_list_btn, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &st_btn_pr,   LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(obj, &st_btn_pr,   LV_PART_MAIN | LV_STATE_CHECKED);
    return;
  }

  /* generic containers/cards */
  if (lv_obj_check_type(obj, &lv_obj_class) || lv_obj_check_type(obj, &lv_canvas_class)) {
    // Si tu veux des “cards” pour certains conteneurs, applique st_card ici
    // lv_obj_add_style(obj, &st_card, LV_PART_MAIN);
    return;
  }
}

/* ========= Application récursive sur un arbre ========= */
static void apply_recursive(lv_obj_t* root)
{
  apply_for_type(root);
  uint32_t n = lv_obj_get_child_cnt(root);
  for (uint32_t i = 0; i < n; ++i) {
    apply_recursive(lv_obj_get_child(root, i));
  }
}

/* ========= Auto-style sur création d’enfants ========= */
static void EV_child_created(lv_event_t* e)
{
  lv_obj_t* child = (lv_obj_t*)lv_event_get_target(e);
  apply_for_type(child);
}

/* ========= API publique : appliquer le thème à un écran ========= */
extern void ui_theme_dark_apply(lv_obj_t* screen_root)
{
  if (!s_theme_inited) ui_theme_dark_init();

  /* Style d’écran */
  lv_obj_add_style(screen_root, &st_scr, LV_PART_MAIN);
  lv_obj_set_style_bg_color(screen_root, C_BG, 0);
  lv_obj_set_style_text_color(screen_root, C_FG, 0);
  lv_obj_set_style_radius(screen_root, 0, 0);

  // ➜ mode d’affichage de la scrollbar
  lv_obj_set_scrollbar_mode(screen_root, LV_SCROLLBAR_MODE_AUTO);

  // ➜ style visuel de la scrollbar
  lv_obj_add_style(screen_root, &st_scrollbar, LV_PART_SCROLLBAR);

  // puis applique récursivement aux enfants + hook (comme tu fais déjà)
  apply_recursive(screen_root);
  lv_obj_add_event_cb(screen_root, EV_child_created, LV_EVENT_CHILD_CREATED, NULL);
}

/* ========= Optionnel : set une font globale =========
   Appelle ça APRÈS lv_init() et l’init de la font (extern LV_FONT_DECLARE...).
*/
// extern lv_font_t my_font_16;
// extern lv_font_t my_font_20;
extern void ui_theme_dark_set_global_font(const lv_font_t* font)
{
  if (!font) return;
  lv_style_set_text_font(&st_scr, font);
  lv_style_set_text_font(&st_label, font);
  lv_style_set_text_font(&st_btn_main, font);
  lv_style_set_text_font(&st_list_btn, font);
  // etc.
}
