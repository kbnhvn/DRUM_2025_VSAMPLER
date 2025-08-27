#include <lvgl.h>

// ==== pont vers ta logique existante ====
extern void DO_KEYPAD();
extern byte trigger_on[48];   // 0..15 pads, 16..23 row1, 24..31 row2, 32..47 bars

inline void UI_Compat_TouchIndex(uint8_t idx){
  if (idx >= 48) return;
  trigger_on[idx] = 1;
  DO_KEYPAD();                // ton moteur traite immédiatement l'action
}
inline void UI_Compat_Pad(uint8_t i){ UI_Compat_TouchIndex(i); }          // 0..15
inline void UI_Compat_Row1(uint8_t i){ UI_Compat_TouchIndex(16 + i); }    // 0..7
inline void UI_Compat_Row2(uint8_t i){ UI_Compat_TouchIndex(24 + i); }    // 0..7
inline void UI_Compat_Bar(uint8_t i){ UI_Compat_TouchIndex(32 + i); }     // 0..15
