// ui_keyboard_shim.ino
#include <lvgl.h>

// API attendue par les vues
void kb_prompt_text(const char* title, bool passwordMode, const char* initial,
                    void (*on_ok)(const char*), void (*on_cancel)()) {
  // TODO: brancher ton vrai clavier LVGL.
  // Pour debogage: renvoyer immédiatement la valeur initiale
  if (on_ok) on_ok(initial ? initial : "");
}

void kb_close() {
  // no-op pour l’instant
}
