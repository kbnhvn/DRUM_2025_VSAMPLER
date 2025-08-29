#include "Arduino.h"

// Variables externes (déjà dans ton code principal)
extern int bpm;
extern int master_vol;
extern int master_filter;
extern int octave;
extern int sync_state;
extern int selected_scale;
extern int ztranspose;
extern int zmpitch;
extern int selected_rot;
extern int counter1;
extern int old_counter1;

// Sélection d’un rotary : charge counter1
void select_rot() {
  switch (selected_rot) {
    case 8:  counter1 = bpm; break;
    case 9:  counter1 = master_vol; break;
    case 11: counter1 = master_filter; break;
    case 12: counter1 = octave; break;
    case 10: counter1 = ztranspose; break;
    case 13: counter1 = zmpitch; break;
    case 14: counter1 = sync_state; break;
    case 15: counter1 = selected_scale; break;
    default: counter1 = 0; break;
  }
}

// Application du changement de valeur
void do_rot() {
  if (counter1 == old_counter1) return;
  old_counter1 = counter1;

  switch (selected_rot) {
    case 8:  bpm = counter1; drawBar(8); break;
    case 9:  master_vol = counter1;
             synthESP32_setMVol(master_vol);
             drawBar(9);
             break;
    case 11: master_filter = counter1;
             synthESP32_setMFilter(master_filter);
             drawBar(11);
             break;
    case 12: octave = counter1; drawBar(12); break;
    case 10: ztranspose = counter1; drawBar(10); break;
    case 13: zmpitch = counter1; drawBar(13); break;
    case 14: sync_state = counter1; drawBar(14); break;
    case 15: selected_scale = counter1; drawBar(15); break;
    default: break;
  }
}
