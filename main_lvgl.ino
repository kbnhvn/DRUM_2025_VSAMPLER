// main_lvgl.ino
#include <Arduino.h>

void lv_port_init();       // from lv_port_jc4827.ino
void build_main_menu();    // from ui_main_menu_lvgl.ino

// ==== externs du moteur existant (ajuste les noms si besoin) ====
extern void audio_output_init();
extern void AudioOut_setVolume(uint8_t vol);
extern void midiUSB_begin();
extern void midiUSB_poll();
extern void synthESP32_begin();
extern void setSound(byte f);

void setup() {
  Serial.begin(115200);
  Serial.println("VSAMPLER LVGL boot");

  // Audio / I2S / Codec
  audio_output_init();
  AudioOut_setVolume(80);

  // MIDI / Synth
  midiUSB_begin();
  synthESP32_begin();
  for (byte f = 0; f < 16; ++f) setSound(f);

  // LVGL + Display/Touch (aligné sur l’exemple radio)
  lv_port_init();

  // UI: Main Menu
  build_main_menu();
}

void loop() {
  // Aligné sur l’exemple radio
  lv_task_handler();
  midiUSB_poll();
  vTaskDelay(1);
}
