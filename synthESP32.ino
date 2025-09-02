#include "synth_api.h"
#include <Arduino.h>
#include "driver/i2s.h"
#include "config_pins.h"

// CORRECTION: Déclarer les pins comme variables au lieu d'utiliser les defines
// pour éviter les conflits avec les #define dans le main
int I2S_BCK_PIN = SYNTH_I2S_BCK_PIN;
int I2S_WS_PIN = SYNTH_I2S_WS_PIN;  
int I2S_DATA_OUT_PIN = SYNTH_I2S_DATA_OUT_PIN;

// Tabla de frecuencias MIDI (0-127)
const float midiFrequencies[128] = {
    8.18, 8.66, 9.18, 9.72, 10.3, 10.91, 11.56, 12.25, 12.98, 13.75, 14.57, 15.43,
    16.35, 17.32, 18.35, 19.45, 20.6, 21.83, 23.12, 24.5, 25.96, 27.5, 29.14, 30.87,
    32.7, 34.65, 36.71, 38.89, 41.2, 43.65, 46.25, 49.0, 51.91, 55.0, 58.27, 61.74,
    65.41, 69.3, 73.42, 77.78, 82.41, 87.31, 92.5, 98.0, 103.83, 110.0, 116.54, 123.47,
    130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.0, 196.0, 207.65, 220.0, 233.08, 246.94,
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.0, 415.3, 440.0, 466.16, 493.88,
    523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880.0, 932.33, 987.77,
    1046.5, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22, 1760.0, 1864.66, 1975.53,
    2093.0, 2217.46, 2349.32, 2489.02, 2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520.0, 3729.31, 3951.07
};

void setRandomPitch(byte v);

void synthESP32_begin(){

  // 16 filters + master L & R (18)
  for (int fi=0; fi < 18; fi++) {
    FILTROS[fi].setResonance(reso);
    FILTROS[fi].setCutoffFreq(cutoff);
  }

  // I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX ),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    //.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
    .dma_buf_count = DMA_NUM_BUF,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = true,
  };

  i2s_pin_config_t i2s_pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num =  I2S_WS_PIN,
    .data_out_num = I2S_DATA_OUT_PIN
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_set_pin(I2S_NUM_0, &i2s_pin_config);

  // Highest possible priority for realtime audio task
  xTaskCreatePinnedToCore(
                  audio_task,       //Function to implement the task 
                  "audio", //Name of the task
                  8000,       //Stack size in words 
                  NULL,       //Task input parameter 
                  configMAX_PRIORITIES - 1,          //Priority of the task 
                  NULL,       //Task handle.
                  0);         //Core where the task should run 

  // Iniciar NEWENDS
  int msize = sizeof(ENDS);
  memcpy(NEWENDS, ENDS, msize);

}

// Función de soft clipping en enteros
inline int16_t soft_clip(int32_t sample) {
    if (sample > 32767) return 32767 - ((sample - 32767) >> 3);
    if (sample < -32768) return -32768 + ((-32768 - sample) >> 3);
    return sample;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
static void write_buffer() {

size_t bytes_written;

memset(out_buf, 0, sizeof(out_buf));

for (int i = 0; i < DMA_BUF_LEN; i++) {

    int32_t DRUMTOTAL_R = 0;
    int32_t DRUMTOTAL_L = 0;

    // sum all samples sounding
    for (byte f = 0; f < 16; f++) {
        if (latch[f]) {
          int16_t sample;
          uint64_t index = samplePos[f] >> 16; // convertir a entero
          if (ROTvalue[f][4]==1){ // reverse
            if (index <= NEWINIS[ROTvalue[f][0]] || index <= 0) {
                latch[f]     = 0;
                samplePos[f] = NEWINIS[ROTvalue[f][0]];
                stepSize[f]  = 0;
                continue;
            } else {
                // Extraigo la muestra
                sample = SAMPLES[ROTvalue[f][0]][index];
                samplePos[f] -= stepSize[f];
            }
          } else {
            if (index >= NEWENDS[ROTvalue[f][0]] || index >= ENDS[ROTvalue[f][0]]) {
                latch[f]     = 0;
                samplePos[f] = 0;
                stepSize[f]  = 0;
                continue;
            } else {
                // Extraigo la muestra
                sample = SAMPLES[ROTvalue[f][0]][index];
                samplePos[f] += stepSize[f];
            }
          }

          int32_t sss = FILTROS[f].next(sample); 
          int32_t sampleL = ((sss * VOL_L[f])) >> 8;
          int32_t sampleR = ((sss * VOL_R[f])) >> 8;

          DRUMTOTAL_L += sampleL;
          DRUMTOTAL_R += sampleR;
        } // endif latch
    } // endfor

    // Aplicar soft clipping
    DRUMTOTAL_L = soft_clip(DRUMTOTAL_L);
    DRUMTOTAL_R = soft_clip(DRUMTOTAL_R);

    // Filtro master
    int16_t DRUMTOTAL_L_OUT = FILTROS[16].next(DRUMTOTAL_L);
    int16_t DRUMTOTAL_R_OUT = FILTROS[17].next(DRUMTOTAL_R);

     // Ajuste de volumen master
    DRUMTOTAL_L_OUT = (DRUMTOTAL_L_OUT * mvol) >> 8;
    DRUMTOTAL_R_OUT = (DRUMTOTAL_R_OUT * mvol) >> 8;

    DRUMTOTAL_L_OUT = soft_clip(DRUMTOTAL_L_OUT);
    DRUMTOTAL_R_OUT = soft_clip(DRUMTOTAL_R_OUT);

    // Almacenar en el buffer de salida
    out_buf[i * 2]     = (uint16_t)DRUMTOTAL_L_OUT;  
    out_buf[i * 2 + 1] = (uint16_t)DRUMTOTAL_R_OUT;
  } // fin bucle del buffer de audio

  // Escribir buffer de audio  
  i2s_write(I2S_NUM_0, out_buf, sizeof(out_buf), &bytes_written, portMAX_DELAY);

}
////////////////////////////////////////////////////////////////////////////////

void audio_task(void *userData){
  while(1) {
      write_buffer();
      vTaskDelay(1);      
  }
} 

//*********************************************************************
//  Setup initial 
//*********************************************************************

void synthESP32_setIni(unsigned char voice, int ini) {
  NEWINIS[ROTvalue[voice][0]]=(ini * ENDS[ROTvalue[voice][0]]) >>11; // map de 0-2048 a los valores del sample
}

//*********************************************************************
//  Setup end 
//*******************************************************************

void synthESP32_setEnd(unsigned char voice, int end) {
  NEWENDS[ROTvalue[voice][0]]=(end * ENDS[ROTvalue[voice][0]]) >>11; // map de 0-2048 a los valores del sample
}

//*********************************************************************
//  Setup master vol [0-127]
//*********************************************************************

void synthESP32_setMVol(unsigned char vol) {
  mvol=vol;	
}

//*********************************************************************
//  Setup master filter [0-255] parameter 0-127
//*********************************************************************

void synthESP32_setMFilter(unsigned char freq)  {
  // ya que 0 es no filter hago un map y cambio el rango
  freq=map(freq,0,127,255,0);
  FILTROS[16].setCutoffFreq(freq);
  FILTROS[17].setCutoffFreq(freq);
}  

//*********************************************************************
//  Setup voice filter [0-255] parameter 0-127
//*********************************************************************

void synthESP32_setFilter(unsigned char voice, unsigned char freq)  {
  // ya que 0 es no filter hago un map y cambio el rango
  freq=map(freq,0,127,255,0);
  FILTROS[voice].setCutoffFreq(freq);
} 

//*********************************************************************
//  Setup vol [0-127]
//*********************************************************************

void synthESP32_setVol(unsigned char voice,unsigned char vol) {
  synthESP32_updateVolPan(voice);
}

//*********************************************************************
//  Setup pan [-127 - 127%]
//*********************************************************************

void synthESP32_setPan(unsigned char voice,signed char pan) {
  synthESP32_updateVolPan(voice);
}

//*********************************************************************
//  Update vol and pan
//*********************************************************************

void synthESP32_updateVolPan(unsigned char voice) {
  int vol = ROTvalue[voice][5];
  int pan = ROTvalue[voice][6];

  // PAN va de -127 (izquierda) a +127 (derecha)
  int panL = 128 - pan; // más PAN a la derecha, menos volumen a la izquierda
  int panR = 128 + pan; // más PAN a la izquierda, menos volumen a la derecha

  // Normalizamos el volumen de cada canal
  VOL_L[voice] = (vol * constrain(panL, 0, 128)) >> 7;
  VOL_R[voice] = (vol * constrain(panR, 0, 128)) >> 7;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

// CORRECTION: Harmoniser le type avec synth_api.h (uint8_t au lieu d'int)
void synthESP32_TRIGGER(uint8_t nkey){

  if (nkey >= 16) return;

  if (ROTvalue[nkey][4]==1) {
    samplePos[nkey] = ((uint64_t)NEWENDS[ROTvalue[nkey][0]]) << 16; // reinicia desde newinis
  } else {
    samplePos[nkey] = ((uint64_t)NEWINIS[ROTvalue[nkey][0]]) << 16; // reinicia desde newinis
  }

  float targetFreq = midiFrequencies[ROTvalue[nkey][3]];
  stepSize[nkey] = (uint64_t)((targetFreq / 261.63) * (1 << 16)); // * 65536
  latch[nkey] = 1;
}

// CORRECTION: Harmoniser le type avec synth_api.h (uint8_t au lieu d'int)
void synthESP32_TRIGGER_P(uint8_t nkey, int ppitch){

  if (nkey >= 16) return; 
  
  if (ROTvalue[nkey][4]==1) {
    samplePos[nkey] = ((uint64_t)NEWENDS[ROTvalue[nkey][0]]) << 16; // reinicia desde newinis
  } else {
    samplePos[nkey] = ((uint64_t)NEWINIS[ROTvalue[nkey][0]]) << 16; // reinicia desde newinis
  }  

  float targetFreq = midiFrequencies[ppitch];
  stepSize[nkey] = (uint64_t)((targetFreq / 261.63) * (1 << 16)); // * 65536
  latch[nkey] = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////

void setSound(byte f){
  synthESP32_setIni(f,ROTvalue[f][1]);  
  synthESP32_setEnd(f,ROTvalue[f][2]);
  synthESP32_updateVolPan(f);  
  synthESP32_setFilter(f,ROTvalue[f][7]);
}

void setRandomVoice2(byte f){
  ROTvalue[f][0]=random(min_values[0], max_values[0]);
  ROTvalue[f][3]=random(min_values[3], max_values[3]);
  ROTvalue[f][6]=random(min_values[6], max_values[6]);
}

void setRandomVoice(byte f){
  ROTvalue[f][0]=random(min_values[0], max_values[0]);
  ROTvalue[f][6]=random(min_values[6], max_values[6]);
}

void setRandomPattern(byte f){
  byte veces=random(0,2);
  for (byte b = 0; b < 16; b++) {
    byte mybit=random(0,2);
    if (veces) {
      if (mybit) mybit=random(0,2); // Si es 1 hago otro random para que haya menos unos
    }
    bitWrite(pattern[f],b,mybit);
  } 
  setRandomPitch(f);
}

bool find_scale(uint8_t note){

  uint8_t candidate=note % 12;
  // buscar en array de escalas
  bool in_scale=false;
  for (byte s = 0; s < 8; s++) {
    if (note==escalas[selected_scale][s]){
      in_scale=true;
      break;
    }
  }
  if (in_scale){
    return true;
  }
  return false;
}

void setRandomPitch(byte f){
  // Tomo como referencia para el rango el valor del pot pitch actual
  uint8_t actual=ROTvalue[selected_sound][3];
  uint8_t limite;
  uint8_t prelimite=24;
  for (byte b = 0; b < 16; b++) {
    limite=actual+random(0,prelimite);
    uint8_t candidate1=random(actual-(prelimite>>1),limite);
    if (selected_scale==0) {
      melodic[f][b]=candidate1;
    } else { // scale
      if (find_scale(candidate1)){
        melodic[f][b]=candidate1;
      } else {
        limite=actual+random(0,prelimite);
        uint8_t candidate2=random(actual-(prelimite>>1),limite); 
        if (find_scale(candidate1)){
          melodic[f][b]=candidate2;
        } else {
          limite=actual+random(0,prelimite);
          uint8_t candidate3=random(actual-(prelimite>>1),limite); 
          if (find_scale(candidate1)){
            melodic[f][b]=candidate3;
          }          
        }             
      }
    }
    
  }  
}

void setRandomNotes(byte f){
  setRandomPitch(f);
}    

// ==== externs déjà présents dans ton projet ====
extern int master_vol;               // défini dans DRUM_2025_VSAMPLER.ino
extern int master_filter;            // idem
extern int32_t ROTvalue[16][8];      // [voice]: SAM,INI,END,PIT,RVS,VOL,PAN,FIL
extern int16_t* SAMPLES[];           // banques de samples (ROM ou SD) – runtime (SD)
extern uint64_t ENDS[];
extern uint64_t NEWINIS[];           // bornes start (pour afficher/découper)
extern uint64_t NEWENDS[];           // bornes end
extern uint64_t samplePos[16];       // position de lecture par voice
extern uint64_t stepSize[16];        // incrément de lecture par voice
extern byte latch[16];               // 0 => (re)-déclencher à la prochaine frame
extern int mvol;                     // (si utilisé par le mixeur interne)
extern LowPassFilter FILTROS[18];    // si tes filtres master L/R = index 16/17
extern const int cutoff;             // bornes pour setCutoff éventuel
extern const int reso;               // etc.

// Variables supplémentaires nécessaires
extern uint16_t pattern[16];
extern uint8_t melodic[16][16]; 
extern const uint8_t escalas[13][8];
extern uint8_t selected_scale;
extern byte selected_sound;
extern const int min_values[16];
extern const int max_values[16];
extern String sound_names[BANK_SIZE];

// Switch audio HP/DAC
static bool currentUseDAC = false;

void switchAudioOutput(bool useDAC) {
  if (currentUseDAC == useDAC) return; // Pas de changement
  
  Serial.printf("[AUDIO] Switching from %s to %s\n", 
                currentUseDAC ? "DAC" : "HP", 
                useDAC ? "DAC" : "HP");
  
  // Arrêter I2S proprement
  i2s_driver_uninstall(I2S_NUM_0);
  delay(100);
  
  // Nouvelle config pins
  i2s_pin_config_t pin_config;
  if (useDAC) {
    // PCM5102A DAC externe
    pin_config.bck_io_num = I2S_PCM_BCLK;
    pin_config.ws_io_num = I2S_PCM_LRCK;
    pin_config.data_out_num = I2S_PCM_DOUT;
  } else {
    // HP interne
    pin_config.bck_io_num = I2S_SPK_BCLK;
    pin_config.ws_io_num = I2S_SPK_LRCK;  
    pin_config.data_out_num = I2S_SPK_DOUT;
  }
  pin_config.data_in_num = I2S_PIN_NO_CHANGE;
  
  // Réinstaller I2S avec même config audio
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
    .dma_buf_count = DMA_NUM_BUF,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = true,
  };
  
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("[AUDIO] I2S install failed: %d\n", err);
    return;
  }
  
  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("[AUDIO] I2S pin config failed: %d\n", err);
    return;
  }
  
  currentUseDAC = useDAC;
  Serial.println("[AUDIO] Audio output switched successfully");
}

// helpers internes (safe/robustes) - VERSIONS SIMPLIFIÉES

static inline uint64_t clampU64(uint64_t v, uint64_t lo, uint64_t hi){
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

static inline uint64_t map12b_to_index(int32_t val12, uint64_t len){
  if (val12 < 0)    val12 = 0;
  if (val12 > 2047) val12 = 2047;
  const uint64_t L = (len > 0) ? (len - 1) : 0;
  return ((uint64_t)val12 * L) / 2047u;
}

static inline uint64_t makeStepSizeFromPit(int32_t pit){
  int semitones = (int)((pit - 64) * 24 / 64);   
  float factor = 1.0f;
  if (semitones > 0) { 
    while (semitones--) factor *= 1.059463094f; 
  } else if (semitones < 0) { 
    while (semitones++) factor /= 1.059463094f; 
  }

  uint64_t stepFixed1616 = (uint64_t)(factor * 65536.0f);
  if (stepFixed1616 < 1) stepFixed1616 = 1;
  return stepFixed1616;
}

static void applyStartEndForSlot(int slot, uint64_t len, int32_t ini12, int32_t end12,
                                 uint64_t& outStart, uint64_t& outEnd){
  uint64_t s = map12b_to_index(ini12, len);
  uint64_t e = map12b_to_index(end12, len);
  if (e <= s) e = (s + 1 <= (len? len-1 : 1)) ? (s + 1) : (len? len-1 : 1);
  outStart = s; outEnd = e;
  if (slot >= 0) {
    NEWINIS[slot] = s;
    NEWENDS[slot] = e;
  }
}

// Implémentations API publiques

void synthESP32_setMVol(int v){
  if (v < 0)   v = 0;
  if (v > 127) v = 127;
  master_vol = v;
}

void synthESP32_setMFilter(int v){
  if (v < 0)   v = 0;
  if (v > 127) v = 127;
  master_filter = v;
}