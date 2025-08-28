// // Inicializa el timer y setea el callback
// void initTimer(void (*callback)()) {
//   onSync24Callback = callback;

//   timer_config_t config = {
//     .alarm_en = TIMER_ALARM_EN,
//     .counter_en = TIMER_PAUSE,
//     .intr_type = TIMER_INTR_LEVEL,
//     .counter_dir = TIMER_COUNT_UP,
//     .auto_reload = TIMER_AUTORELOAD_EN,
//     .divider = TIMER_DIVIDER
//   };

//   timer_init(TIMER_GROUP, TIMER_INDEX, &config);
//   timer_isr_register(TIMER_GROUP, TIMER_INDEX, onTimer, NULL, ESP_INTR_FLAG_IRAM, NULL);
// }

// // Configura el BPM
// void setBPM(float bpm) {
//   if (bpm <= 0) return;
//   float intervalSeconds = 60.0 / (bpm * 24.0);  // 24ppqn como en uClock
//   uint64_t intervalUs = intervalSeconds * TIMER_SCALE;
//   timer_set_alarm_value(TIMER_GROUP, TIMER_INDEX, intervalUs);
// }

// // Arranca el timer
// void startTimer() {
//   timer_set_counter_value(TIMER_GROUP, TIMER_INDEX, 0);
//   timer_enable_intr(TIMER_GROUP, TIMER_INDEX);
//   timer_start(TIMER_GROUP, TIMER_INDEX);
// }

// // Detiene el timer
// void stopTimer() {
//   timer_pause(TIMER_GROUP, TIMER_INDEX);
//   timer_disable_intr(TIMER_GROUP, TIMER_INDEX);
// }

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

// soft 24 PPQN clock pilotée dans loop()
extern int bpm;
static uint32_t s_last_us = 0;
static uint32_t s_tick = 0;

extern void onSync24Callback(uint32_t tick);

// --- API simple pour remplacer uClock ---
static bool s_running = false;
void sequencer_start()  { s_running = true;  }
void sequencer_stop()   { s_running = false; }
void sequencer_set_bpm(int nbpm) { bpm = nbpm; }

extern void sequencer_tick(void) { /* no-op */ }

