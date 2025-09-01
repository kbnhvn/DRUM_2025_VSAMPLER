#include "Arduino.h"
#include "synth_api.h"
#include <uClock.h>

// Variables externes avec validation
extern const int max_values[16];
extern const int min_values[16];
extern int bpm;
extern int master_vol;
extern int master_filter;
extern int octave;
extern int ztranspose;
extern int zmpitch;
extern int counter1;
extern int old_counter1;
extern byte sync_state;
extern uint8_t selected_scale;
extern byte selected_rot;
extern byte selected_sound;
extern int32_t ROTvalue[16][8];
extern bool playing;

// NOUVEAU: Validation des valeurs rotary
static int validateRotaryValue(byte rot, int value) {
  if (rot >= 16) return value; // Pas de validation pour rotary inexistant
  
  int minVal = min_values[rot];
  int maxVal = max_values[rot];
  
  if (value < minVal) {
    Serial.printf("[ROT] Value %d below min %d for rotary %d, clamping\n", value, minVal, rot);
    return minVal;
  }
  if (value > maxVal) {
    Serial.printf("[ROT] Value %d above max %d for rotary %d, clamping\n", value, maxVal, rot);
    return maxVal;
  }
  
  return value;
}

// Sélection d'un rotary : charge counter1 avec validation
void select_rot() {
  if (selected_rot >= 16) {
    Serial.printf("[ROT] Invalid selected_rot: %d\n", selected_rot);
    selected_rot = 0;
  }

  switch (selected_rot) {
    case 0:  counter1 = ROTvalue[selected_sound][0]; break; // SAM
    case 1:  counter1 = ROTvalue[selected_sound][1]; break; // INI
    case 2:  counter1 = ROTvalue[selected_sound][2]; break; // END
    case 3:  counter1 = ROTvalue[selected_sound][3]; break; // PIT
    case 4:  counter1 = ROTvalue[selected_sound][4]; break; // RVS
    case 5:  counter1 = ROTvalue[selected_sound][5]; break; // VOL
    case 6:  counter1 = ROTvalue[selected_sound][6]; break; // PAN
    case 7:  counter1 = ROTvalue[selected_sound][7]; break; // FIL
    case 8:  counter1 = bpm; break;                         // BPM
    case 9:  counter1 = master_vol; break;                  // MVOL
    case 10: counter1 = ztranspose; break;                  // TRP
    case 11: counter1 = master_filter; break;               // MFI
    case 12: counter1 = octave; break;                      // OCT
    case 13: counter1 = zmpitch; break;                     // MPI
    case 14: counter1 = sync_state; break;                  // SYN
    case 15: counter1 = selected_scale; break;              // SCA
    default: 
      Serial.printf("[ROT] Unknown rotary %d\n", selected_rot);
      counter1 = 0; 
      break;
  }
  
  Serial.printf("[ROT] Selected rotary %d (%s), value=%d\n", 
                selected_rot, 
                selected_rot < 16 ? (selected_rot < 8 ? "VOICE" : "GLOBAL") : "UNKNOWN",
                counter1);
}

// NOUVEAU: Application du changement avec validation et feedback
void do_rot() {
  if (counter1 == old_counter1) return;
  
  // Validation de la nouvelle valeur
  int newValue = validateRotaryValue(selected_rot, counter1);
  if (newValue != counter1) {
    counter1 = newValue; // Appliquer la valeur clampée
  }

  old_counter1 = counter1;

  switch (selected_rot) {
    case 0:  // SAM - Sample selection
      ROTvalue[selected_sound][0] = counter1;
      setSound(selected_sound);
      drawBar(0);
      Serial.printf("[ROT] Voice %d sample -> %d (%s)\n", 
                    selected_sound, counter1, sound_names[counter1].c_str());
      break;
      
    case 1:  // INI - Sample start
      ROTvalue[selected_sound][1] = counter1;
      setSound(selected_sound);
      drawBar(1);
      break;
      
    case 2:  // END - Sample end
      ROTvalue[selected_sound][2] = counter1;
      setSound(selected_sound);
      drawBar(2);
      break;
      
    case 3:  // PIT - Pitch
      ROTvalue[selected_sound][3] = counter1;
      setSound(selected_sound);
      drawBar(3);
      break;
      
    case 4:  // RVS - Reverse
      ROTvalue[selected_sound][4] = counter1;
      setSound(selected_sound);
      drawBar(4);
      break;
      
    case 5:  // VOL - Volume
      ROTvalue[selected_sound][5] = counter1;
      setSound(selected_sound);
      drawBar(5);
      break;
      
    case 6:  // PAN - Panoramique
      ROTvalue[selected_sound][6] = counter1;
      setSound(selected_sound);
      drawBar(6);
      break;
      
    case 7:  // FIL - Filter
      ROTvalue[selected_sound][7] = counter1;
      setSound(selected_sound);
      drawBar(7);
      break;
      
    case 8:  // BPM - Tempo global
      bpm = counter1; 
      // NOUVEAU: Appliquer BPM au séquenceur en temps réel
      if (playing) {
        uClock.setTempo(bpm);
        Serial.printf("[ROT] BPM changed to %d (live)\n", bpm);
      } else {
        Serial.printf("[ROT] BPM set to %d\n", bpm);
      }
      drawBar(8); 
      break;
      
    case 9:  // MVOL - Master volume
      master_vol = counter1;
      synthESP32_setMVol((uint8_t)master_vol);
      drawBar(9);
      Serial.printf("[ROT] Master volume -> %d\n", master_vol);
             break;
             
    case 10: // TRP - Transpose
      ztranspose = counter1; 
      drawBar(10);
      Serial.printf("[ROT] Transpose -> %d\n", ztranspose);
      break;
      
    case 11: // MFI - Master filter
      master_filter = counter1;
      synthESP32_setMFilter((uint8_t)master_filter);
      drawBar(11);
      Serial.printf("[ROT] Master filter -> %d\n", master_filter);
             break;
             
    case 12: // OCT - Octave
      octave = counter1; 
      drawBar(12);
      Serial.printf("[ROT] Octave -> %d\n", octave);
      break;
      
    case 13: // MPI - Master pitch
      zmpitch = counter1; 
      drawBar(13);
      break;
      
    case 14: // SYN - Sync mode
      sync_state = counter1; 
      drawBar(14);
      Serial.printf("[ROT] Sync mode -> %d (%s)\n", sync_state,
                    sync_state == 0 ? "INTERNAL" : 
                    sync_state == 1 ? "MASTER" : "SLAVE");
      break;
      
    case 15: // SCA - Scale
      selected_scale = counter1; 
      drawBar(15);
      Serial.printf("[ROT] Scale -> %d\n", selected_scale);
      break;
      
    default: 
      Serial.printf("[ROT] Unhandled rotary %d\n", selected_rot);
      break;
  }
}
