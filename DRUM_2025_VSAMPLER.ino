////////////////////////////
// DRUM SAMPLER 2025      //
////////////////////////////
// ZCARLOS 2025
// V002 beta

// includes
#include <Arduino.h>
#include "synth_api.h"
#include "sd_catalog.h"
#include "esp_task_wdt.h"

// CORRECTION: Ajouter les variables manquantes au début
volatile uint8_t sstep = 0;            // CORRECTION: uint8_t au lieu de byte
volatile bool refreshPATTERN = true; 
volatile bool refreshMODES = true;
volatile bool refreshPADSTEP = false;
volatile bool clearPADSTEP = false;

// CORRECTION: Variables pour sample picker - supprimer les doublons
int previewIndex = -1;
unsigned long lastPreviewTime = 0;

// --- Prototypes nécessaires (déclarés ailleurs) ---
void resetGT911();
void read_touch();
void setSound(byte voice);
void select_rot();
void onSync24Callback(uint32_t tick); // callback uClock

// CORRECTION: Ajouter les prototypes manquants
void drawBar(byte bar);
void openSamplePicker();

// --- Runtime samples from SD ---
#ifndef USE_SD_SAMPLES
  #define USE_SD_SAMPLES 1
#endif

// Forward decls for new views/server/backlight
void openMenuView();
void openPatternView();
void openSongView();
void openBrowserView();
void loopWeb();
void backlight_init();
void stopWiFiAutoReconnect(); // CORRECTION: Ajouter prototype manquant

//#define TESTING 1

//#define mylcd_type1
#define mylcd_type2
// if you don't have a ADS1115 and rotary comment next line
//#define ads_ok

int32_t muestra;
#define SAMPLE_RATE 44100         // Frecuencia de muestreo (44.1 kHz)

////////////////////////////// MIDI BLE
//#include <BLEMidi.h>

////////////////////////////// WIRE I2C
#include <Wire.h>

////////////////////////////// SD SPI
// SD
#include <SPI.h>
#include <SD.h>
// Pines SPI para la tarjeta SD
#define SD_CS   10 // Chip Select (CS)
#define SD_MOSI 11 // Master Out Slave In (MOSI)
#define SD_MISO 13 // Master In Slave Out (MISO)
#define SD_SCLK 12 // Serial Clock (SCLK)
#define MAX_SOUNDS 128    // Número máximo de archivos WAV a listar
SPIClass sdSPI(HSPI); // SPI BUS for SD
// Array para almacenar los nombres de los archivos WAV
//String soundFiles[MAX_SOUNDS];

////////////////////////////// SPIFFS
#include <FS.h>

////////////////////////////// SYNTH
//#define SAMPLE_RATE 44100
#include "driver/i2s.h"
#include "synthESP32LowPassFilter_E.h" // filter
#define MAX_16 32767
#define MIN_16 -32767

////////////////////////////// LCD 
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE ESP32_4827A043_QSPI
  #ifdef GFX_BL
    backlight_init(); // PWM brightness
  #endif
// Arduino_DataBus *bus = new Arduino_ESP32QSPI(45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
//Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);
// 320x240
//Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 1 /* rotation */);
// Arduino_GFX *gfx = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
#ifdef ESP32
#undef F
#define F(s) (s)
#endif

///////////////////////////////// TOUCH
#define GT911_ADDR 0x5D
#define TOUCH_SDA 8
#define TOUCH_SCL 4
#define TOUCH_INT 3 // I'm not using interrupt
#define TOUCH_RST 38

//............................................................................
// Buttons size properties
#define Bleft 0 // starting point
#define Btop 0  // starting point

// 16 Button pad
#define BwidthPad 60
#define BheightPad 50

// 16 Bars 
#define BwidthBar 24
#define BheightBar 80

// 8 Func Button
#define BwidthFunc 60
#define BheightFunc 50

// 8 norotary buttons
#define BwidthNorot 60
#define BheightNorot 42

// wave location
#define rot0_width 144
#define rot0_height BheightBar

#define rot1_width BwidthBar
#define rot1_height BheightBar/2
#define rot2_width BwidthBar
#define rot2_height BheightBar/2

#define WAVE_WIDTH 128
#define WAVE_HEIGHT 64
#define WAVE_ORIGIN_X 6
#define WAVE_ORIGIN_Y (BheightPad*2)+2

// 16+16+8+8
int BPOS[48][4]; // filled in setup
int BCOLOR[48];  // to store current button color

const int debounce_time=230; // time to wait, miliseconds
long start_debounce;
bool touchActivo=false;
int last_touched=-1;
bool show_last_touched=false;
long start_showlt;

bool flag_ss=false;
int count_loop=0;

// Touchscreen coordinates: (x, y) and pressure (z)
int cox, coy, coz;
////////////////////////////// TIMER SEQ 
#include <uClock.h>

////////////////////////////// MIDI USB
//#include <Adafruit_TinyUSB.h>
//#include <MIDI.h>

////////////////////////////// SERIAL 2 (SYNC)
// Hardware Serial 2 pins
// #define RXD2 34
// #define TXD2 21


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

// modeZ types

//  0 - play pad sound
//  1 - select sound
//  2 - edit track, write
//  3 - mute
//  4 - solo
//  5 - clear

//  6 - load pattern
//  7 - load pattern
//  8 - load pattern and sound

//  9 - save pattern
// 10 - save sound
// 11 - save sound and pattern
 
// 12 - random sound
// 13 - random pattern
// 14 - first step
// 15 - last step
// 16 - set melodic (read pitch from pattern melodic instead of rotary value)
// 17 - random note
// 18 - piano
// 19 - song or pattern
// 20 - select memory
// 21 - first memory
// 22 - last memory
// 23 - random sound 2

#define tPad    0
#define tSel    1
#define tWrite  2
#define tMute   3
#define tSolo   4
#define tClear  5
#define tLoaPS  6
#define tLoadP  7
#define tLoadS  8
#define tSavPS  9
#define tSaveP 10
#define tSaveS 11
#define tRndS  12
#define tRndP  13
#define tFirst 14
#define tLast  15
#define tMelod 16
#define tRndNo 17
#define tPiano 18
#define tSong  19
#define tMemo  20
#define tMfirs 21
#define tMlast 22
#define tRndS2 23

const String trot[16] = { "SAM","INI","END","PIT","RVS","VOL","PAN","FIL","BPM","MVO","TRP","MFI","OCT","MPI","SYN","SCA"};
const String tbuttons1[8] = {"   PAD   ", "  RND P ", "  LOAD PS ", " SAVE PS ", "  MUTE  ", "  PIANO ", "  PLAY  ", "  SONG  "};
const String tbuttons2[8] = {"  SHIFT  ", "  - 1   ", "  - 10   ", "  + 10   ", "  + 1   ", "  PTRN  ", "  MENU  ", "  SHIFT "};

#if (USE_SD_SAMPLES)
  #define BANK_SIZE 256
  extern int16_t* SAMPLES[BANK_SIZE];
  extern uint64_t ENDS[BANK_SIZE];
  extern String   sound_names[BANK_SIZE];
  extern void initSD();
  extern void buildCatalog();
#endif


uint64_t NEWENDS[256];
uint64_t NEWINIS[256];

byte latch[16];

uint64_t samplePos[16];
uint64_t stepSize[16];

// I2s - CORRECTION: Définir les pins correctement
#ifdef mylcd_type1
  #define SYNTH_I2S_BCK_PIN 41
  #define SYNTH_I2S_WS_PIN 2
  #define SYNTH_I2S_DATA_OUT_PIN 42
#endif
#ifdef mylcd_type2
  #define SYNTH_I2S_BCK_PIN 9
  #define SYNTH_I2S_WS_PIN 5
  #define SYNTH_I2S_DATA_OUT_PIN 14
#endif
#define DMA_BUF_LEN     64          
#define DMA_NUM_BUF     8

static uint16_t out_buf[DMA_BUF_LEN * 2];

// 16 filters + master L & R (Total = 18)
 LowPassFilter FILTROS[18]={
   LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
   LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
   LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
   LowPassFilter(),LowPassFilter(),LowPassFilter(),LowPassFilter(),
   LowPassFilter(),LowPassFilter()
 };

const int cutoff=255;
const int reso=511;

static int VOL_R[16] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10}; 
static int VOL_L[16] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};   
static int PAN[16]   = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10}; 

static int mvol=10;
int master_vol=80;
int master_filter=0;
int octave=5;

////////////////////////////// MIDI USB
//Adafruit_USBD_MIDI usb_midi;
//MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

////////////////////////////// SEQ 
int bpm=120;
byte selected_pattern=0;
byte s_old_selected_pattern=255;
int s_old_sstep=-1;
volatile bool load_flag=false;    // volatile car modifié dans ISR

uint16_t mutes=0;
uint16_t solos=0;
uint16_t pattern[16];

byte fx1=0; 

byte selected_memory=0;
byte s_old_selected_memory=255;
int pattern_song_counter=-1;
byte last_pattern_song=255;
uint8_t  melodic[16][16];
//uint16_t isMelodic=B11111111<<8 | B00000000; // last 8 sounds are set to melodic, set into setup
uint16_t isMelodic=B00000000<<8 | B00000000; // none set to melodic

byte firstStep=0;
byte lastStep=15;
byte newLastStep=15; 

byte firstMemory=0;
byte lastMemory=15;
byte newLastMemory=15; 

byte selected_sound=0;
byte oldselected_sound=0;
byte s_old_selected_sound=255;
//byte s_old_drawWaveform=99;
byte selected_sndSet=0;
byte s_old_selected_sndSet=255;
int ztranspose=0;
int zmpitch=0;

byte sync_state=0;              // 0 no sync, 1 master, 2 slave
volatile bool sync_flag=false;  // volatile car modifié dans ISR

// ESCALAS (255 = void)
uint8_t selected_scale=0;
const uint8_t escalas[13][8] = {  
  {0, 0, 0, 0, 0, 0, 0, 0}, // free
  // Escalas diatónicas
  {0, 2, 4, 5, 7, 9, 11, 255},   // Mayor (Jónica)
  {0, 2, 3, 5, 7, 8, 10, 255},   // Menor natural (Eólica)
  {0, 2, 3, 5, 7, 8, 11, 255},   // Menor armónica
  {0, 2, 3, 5, 7, 9, 11, 255},   // Menor melódica

  // Escalas pentatónicas y blues
  {0, 2, 4, 7, 9, 255, 255, 255},  // Pentatónica mayor
  {0, 3, 5, 7, 10, 255, 255, 255}, // Pentatónica menor
  {0, 3, 5, 6, 7, 10, 255, 255},   // Blues menor

  // Modos
  {0, 2, 3, 5, 7, 9, 10, 255},   // Dórico
  {0, 1, 3, 5, 7, 8, 10, 255},   // Frigio
  {0, 2, 4, 6, 7, 9, 11, 255},   // Lidio
  {0, 2, 4, 5, 7, 9, 10, 255},   // Mixolidio
  {0, 1, 3, 5, 6, 8, 10, 255}    // Locrio
};
const char* nombresEscalas[] = {
  "Free"
  "Mayor",
  "Menor Natural",
  "Menor Armónica",
  "Menor Melódica",
  "Pentatónica Mayor",
  "Pentatónica Menor",
  "Blues Menor",
  "Dórico",
  "Frigio",
  "Lidio",
  "Mixolidio",
  "Locrio"
};
////////////////////////////// ROTARY

byte ENC_PORT1=0;
int counter1=0;
int old_counter1=0;
byte shiftR1=false;
byte old_shiftR1=true;

// 8 sound parameters + bpm + master vol + transpose + master filter + octave + Pattern song selector + Sync mode
const int max_values[16]={255,2047,2047,127,1,127, 127,127,400,127, 1,127,20, 1,2,12}; 
const int min_values[16]={  0,   0,   0,  0,0,  0,-127,  0,  0,  0,-1,  0, 0,-1,0, 0};

int32_t ROTvalue[16][8]={ // init sound values
  { 16,0, 2047, 60,0,80,-10,0}, // THIS IS VOICE #0
  { 17,0, 2047, 60,0,80,-31,0}, // THIS IS VOICE #1
  { 18,0, 2047, 60,0,80, 12,0}, // ...
  { 19,0, 2047, 60,0,80,  0,0},
  { 20,0, 2047, 60,0,80, 20,0},
  { 21,0, 2047, 60,0,80,  0,0},
  { 22,0, 2047, 60,0,80, 30,0},
  { 23,0, 2047, 60,0,80,  0,0},
  
  { 24,0, 2047, 60,0,80, 66,0},
  { 25,0, 2047, 60,0,80, 66,0},
  { 26,0, 2047, 60,0,80,  0,0},
  { 27,0, 2047, 60,0,80,-20,0},
  { 28,0, 2047, 60,0,80,  0,0},
  { 29,0, 2047, 60,0,80,-10,0},
  { 30,0, 2047, 60,0,80, 30,0},
  { 31,0, 2047, 60,0,80,-30,0}
};

byte selected_rot=0;
byte s_old_selected_rot=255;

byte trigger_on[48]; // 16 pads + 8 funcs + 8 norot buttons + 16 bars
byte nkey;

////////////////////////////// MISC
byte modeZ=0;
int s_old_modeZ=-1;

volatile bool playing = false;      // volatile car lu dans ISR
bool pre_playing = false;
bool songing     = false; // switch to make load auto patterns++
bool recording   = false;
bool shifting    = false;

bool clearPATTERNPADS=false;

#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP  0xFC

//////////////////////////////  S E T U P  //////////////////////////////
void setup() {

  // Serial
  Serial.begin(115200);
  delay(200);
  Serial.println("=== DRUM SAMPLER 2025 Starting ===");
  
  // Watchdog des tâches : inutile ici et source de boot-loop si une tâche ne yield pas
  esp_task_wdt_deinit();
  
  // Seed via RNG matériel (évite analogRead(0) non ADC sur S3)
  randomSeed(esp_random());
  
  Serial.println("[SETUP] Basic init OK");

  // Iniciar el primer puerto I2C
  Wire.begin(TOUCH_SDA, TOUCH_SCL,400000);

//........................................................................................................................
// LCD and TOUCH
  Serial.println("[SETUP] Starting display...");
  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  backlight_init();

  gfx->setFont(u8g2_font_5x8_mr);
  gfx->setUTF8Print(true);
  
  fillBPOS();
  drawScreen1_ONLY1();

  // Touch
  resetGT911();
  delay(100);
  
  Serial.println("[SETUP] Display and touch OK");

  // Synth
  Serial.println("[SETUP] Starting synth...");
  synthESP32_begin();

  // Set 16 voices
  for (byte f=0;f<16;f++){
    setSound(f);
  }
  // Set master vol
  synthESP32_setMVol(master_vol);
  // Set master filter
  synthESP32_setMFilter(master_filter);
  
  Serial.println("[SETUP] Synth OK");
  
  ///////////////////////////////////////////////////////
  // SD Card avec gestion d'erreur robuste
  Serial.println("[SETUP] Starting SD...");
  
  // Inicializar otro bus SPI para la tarjeta SD
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  // Inicializar la tarjeta SD avec retry
  bool sdOk = false;
  for (int retry = 0; retry < 3; retry++) {
    if (SD.begin(SD_CS, sdSPI)) {
      sdOk = true;
      break;
    }
    Serial.printf("[SETUP] SD init attempt %d failed, retrying...\n", retry + 1);
    delay(500);
  }
  
  if (sdOk) {
    Serial.println("[SETUP] SD init OK");
    
    // Build catalog avec gestion d'erreur
    Serial.println("[SETUP] Building sample catalog...");
    try {
      buildCatalog();
      Serial.println("[SETUP] Sample catalog built successfully");
    } catch (...) {
      Serial.println("[SETUP] ERROR: Catalog build failed, continuing without samples");
    }
  } else {
    Serial.println("[SETUP] WARNING: SD card failed, running without samples");
    // Continuer sans SD - pas de while(1) qui bloque
  }

  stopWiFiAutoReconnect(); 

  ///////////////////////////////////////////////////////
  // Sequencer
  Serial.println("[SETUP] Starting sequencer...");
  
  // fill melodic with 60
  for (byte a=0;a<16;a++){
    for (byte b=0;b<16;b++){
     melodic[a][b]=60;
    }    
  }
  // set 8-15 as melodic;
  isMelodic=B11111111<<8 | B00000000;

  // Setup our clock system
  uClock.init();
  uClock.setOnSync24(onSync24Callback);
  uClock.setTempo(bpm);

  Serial.println("[SETUP] Sequencer OK");

//........................................................................................................................
  // UI final
  select_rot();
  draw8aBar();
  draw8bBar();
  
  Serial.println("=== SETUP COMPLETE ===");
}

//////////////////////////////  L O O P  //////////////////////////////

void loop() {

  esp_task_wdt_reset();
  loopWeb();
  
  // flag to do things outside sequencer timer isr
  if (load_flag){
    load_flag=false;
    pattern_song_counter++;
    // Comprobar step final
    if ( (pattern_song_counter>lastMemory) || (pattern_song_counter>newLastMemory) || (pattern_song_counter==16) ){
      lastMemory=newLastMemory;
      pattern_song_counter=firstMemory;
    }
    load_pattern(pattern_song_counter);
    selected_pattern=pattern_song_counter;
    load_sound(pattern_song_counter);
    selected_sndSet=pattern_song_counter;
    select_rot();
    draw8aBar();
    refreshPADSTEP=true;
  }

  read_touch();

  // CORRECTION: Vérifier si uClock tourne correctement  
  if (!playing && uClock.state != 0) {  // CORRECTION: utiliser .state au lieu de .getMode()
    uClock.stop();
  }
  
  DO_KEYPAD();
  REFRESH_KEYS();
  REFRESH_STATUS();

  showLastTouched();
  clearLastTouched();

  // CORRECTION: == au lieu de =
  if (flag_ss == true){
    count_loop++;
    if (count_loop==2){
      count_loop=0;
      flag_ss=false;
      for (byte f=0;f<16;f++){
        setSound(f);
      } 
    } 
  }
}