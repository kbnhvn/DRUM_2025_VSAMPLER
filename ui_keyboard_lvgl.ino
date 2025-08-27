// ui_keyboard_lvgl.ino
#include <lvgl.h>

// Petit modal clavier + textarea. Appelle vos callbacks et se détruit proprement.
static lv_obj_t* kb_modal = nullptr;
static lv_obj_t* kb_kbd   = nullptr;
static lv_obj_t* kb_ta    = nullptr;
static void (*cb_ok_)(const char*) = nullptr;
static void (*cb_cancel_)() = nullptr;

static void kb_close_modal(){
  if (kb_modal) { lv_obj_del(kb_modal); kb_modal=nullptr; kb_kbd=nullptr; kb_ta=nullptr; }
  cb_ok_ = nullptr; cb_cancel_ = nullptr;
}

static void kb_event_handler(lv_event_t* e){
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target_obj(e);
  if (code == LV_EVENT_READY){           // touche "OK"
    if (cb_ok_ && kb_ta) cb_ok_(lv_textarea_get_text(kb_ta));
    kb_close_modal();
  } else if (code == LV_EVENT_CANCEL){   // touche "Close"
    if (cb_cancel_) cb_cancel_();
    kb_close_modal();
  }
}

void kb_prompt_text(const char* title, bool passwordMode, const char* initial,
                    void (*on_ok)(const char*), void (*on_cancel)())
{
  // garde-fou: fermer l’existant si présent
  kb_close_modal();
  cb_ok_     = on_ok;
  cb_cancel_ = on_cancel;

  kb_modal = lv_obj_create(lv_scr_act());
  lv_obj_set_size(kb_modal, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(kb_modal, LV_OPA_80, 0);
  lv_obj_center(kb_modal);

  lv_obj_t* cont = lv_obj_create(kb_modal);
  lv_obj_set_size(cont, LV_PCT(90), LV_PCT(80));
  lv_obj_center(cont);

  lv_obj_t* lbl = lv_label_create(cont);
  lv_label_set_text(lbl, title ? title : "Enter text");
  lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 6);

  kb_ta = lv_textarea_create(cont);
  lv_obj_set_width(kb_ta, LV_PCT(90));
  lv_obj_align(kb_ta, LV_ALIGN_TOP_MID, 0, 28);
  if (initial) lv_textarea_set_text(kb_ta, initial);
  if (passwordMode){ lv_textarea_set_password_mode(kb_ta, true); lv_textarea_set_password_show_time(kb_ta, 0); }

  kb_kbd = lv_keyboard_create(cont);
  lv_keyboard_set_mode(kb_kbd, LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(kb_kbd, kb_ta);
  lv_obj_set_size(kb_kbd, LV_PCT(100), LV_PCT(60));
  lv_obj_align(kb_kbd, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_event_cb(kb_kbd, kb_event_handler, LV_EVENT_ALL, NULL);
}
