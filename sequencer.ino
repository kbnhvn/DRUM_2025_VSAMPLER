#include "synth_api.h"

extern bool playing;
extern byte fx1;
extern byte selected_sound;
extern bool clearPADSTEP;
extern volatile byte sstep;
extern byte firstStep;
extern byte lastStep;
extern byte newLastStep;
extern uint16_t mutes;
extern uint16_t solos;
extern uint16_t pattern[16];
extern byte latch[16];
extern uint16_t isMelodic;
extern uint8_t melodic[16][16];
extern bool songing;
extern volatile bool load_flag;
extern bool sync_flag;
extern volatile bool refreshPADSTEP;


void IRAM_ATTR tic(){  
  if (sstep==firstStep){
    sync_flag=true;
  }  
  for (int f = 0; f < 16; f++) { 
    if (!bitRead(mutes, f)) {
      if (solos == 0 || (solos > 0 && bitRead(solos, f))) {
        if (bitRead(pattern[f], sstep)) { // note on
          latch[f]=0;        
          if (bitRead(isMelodic,f)){
            synthESP32_TRIGGER_P(f,melodic[f][sstep]);
          } else {
            // Trigger con el pitch del canal
            synthESP32_TRIGGER(f);
          }
        } 
      }
    }
  }

  sstep++;
  // Comprobar step final
  if (sstep==(lastStep+1) || sstep==(newLastStep+1) || sstep==16) {
    lastStep=newLastStep;
    sstep=firstStep;
    if (songing){
      load_flag=true; // inside loop I will load next pattern
    }
  }
  refreshPADSTEP=true;
}


void onSync24Callback(uint32_t tick){

  // FX1
  if (playing){
    if (!(tick % (12)) && fx1==1) {
      synthESP32_TRIGGER(selected_sound);
    }
    if (!(tick % (6)) && fx1==2) {
      synthESP32_TRIGGER(selected_sound);
    }
    if (!(tick % (3)) && fx1==3) {
      synthESP32_TRIGGER(selected_sound);
    }
  }
  
  // Lanzar cambio de step
  if (!(tick % (6))) tic();

  // Limpiar marcas de sound y step
  if ((tick % (6))==4) clearPADSTEP=true;

}



