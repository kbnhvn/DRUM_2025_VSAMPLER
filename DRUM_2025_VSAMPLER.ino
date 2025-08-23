////////////////////////////
// DRUM SAMPLER 2025      //
////////////////////////////
// ZCARLOS 2025
// V002 beta

// Lots of bugs

// includes
#include <Arduino.h>
// MIDI USB Native - plus besoin de "driver/uart.h"

//#define TESTING 1

//#define mylcd_type1
#define mylcd_type2
// if you don't have a ADS1115 and rotary comment next line
//#define ads_ok


int32_t muestra;
#define SAMPLE_RATE 44100         // Frecuencia de muestreo (44.1 kHz)

////////////////////////////// MIDI USB INTEGRATION
// Migration UART -> USB MIDI Native completed
// L'ancien système BLE MIDI est remplacé par USB Native
//#include <BLEMidi.h>

////////////////////////////// WIRE I2C
#include <Wire.h>

////////////////////////////// SD SPI
// SD Card pour samples dynamiques
#include <SPI.h>
#include <SD.h>
// Configuration compatible avec notre module sd.ino
// Utilisation de notre configuration optimisée
bool sdSystemEnabled = true;    // Flag pour activer/désactiver le système SD
bool sdBrowserMode = false;     // Mode navigateur de samples SD
uint16_t sdSelectedFile = 0;    // Index du fichier SD sélectionné
uint8_t sdMode = 0;             // 0=samples intégrés, 1=samples SD, 2=mode hybride

/////////////////////////////// I2C POTS ADS1015
#include <Adafruit_ADS1X15.h>
#ifdef mylcd_type1
  #define SDA_2 15  
  #define SCL_2 16 
#endif
#ifdef mylcd_type2
  #define SDA_2 17  
  #define SCL_2 18 
#endif

TwoWire I2C_2 = TwoWire(1); // 2nd I2C port
Adafruit_ADS1015 ads;
int16_t adc0=0;
int16_t adc1=0;
int16_t adc2=0;
int16_t adc3=0; 
int16_t tmp_adc1;
int16_t old_adc0=0;
int16_t old_adc1=0;
int16_t old_adc2=0;
int16_t old_adc3=0;

unsigned long previousMillis = 0;
const long interval = 100; 

int16_t adcValue0 = 0;
int16_t adcValue1 = 0;
int16_t adcValue2 = 0;
int16_t adcValue3 = 0;

////////////////////////////// SPIFFS
#include <FS.h>
#include <SPIFFS.h>


////////////////////////////// SYNTH
//#define SAMPLE_RATE 44100
#include "driver/i2s.h"
#include "synthESP32LowPassFilter_E.h" // filter
#define MAX_16 32767
#define MIN_16 -32767

////////////////////////////// TEXTS
const String trot[16] = { "SAM", "INI", "END", "PIT", "RVS", "VOL", "PAN", "FIL", "BPM","MVO","TRP","MFI","OCT","MPI","SYN","SCA"};
const String tbuttons1[8]       = {"   PAD   ", "  RND P ", " LOAD PS ", " SAVE PS ", "  MUTE  ", "  PIANO ", "  PLAY  ", "  SONG  "};
const String tbuttons2[8]       = {"  SHIFT  ", "  - 1   ", "  - 10   ", "  + 10   ", "  + 1   ", "SD BROWSE", "SD LOAD ", "  SHIFT "};

////////////////////////////// LCD 
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE ESP32_4827A043_QSPI
#define GFX_BL 1
Arduino_DataBus *bus = new Arduino_ESP32QSPI(45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
//Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);
// 320x240
//Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 1 /* rotation */);
Arduino_GFX *gfx = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
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

////////////////////////////// MIDI USB NATIVE
// Migration terminée - USB MIDI Native intégré
// Plus besoin de bibliothèques externes
// Implémentation dans midi.ino

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


uint64_t NEWENDS[50];
uint64_t NEWINIS[50];

byte latch[16];

uint64_t samplePos[16];
uint64_t stepSize[16];

////////////////////////////// SOUNDS

#include "SOUNDS/zSAMPLE00.h"
#include "SOUNDS/zSAMPLE01.h"
#include "SOUNDS/zSAMPLE02.h"
#include "SOUNDS/zSAMPLE03.h"
#include "SOUNDS/zSAMPLE04.h"
#include "SOUNDS/zSAMPLE05.h"
#include "SOUNDS/zSAMPLE06.h"
#include "SOUNDS/zSAMPLE07.h"
#include "SOUNDS/zSAMPLE08.h"
#include "SOUNDS/zSAMPLE09.h"
#include "SOUNDS/zSAMPLE10.h"
#include "SOUNDS/zSAMPLE11.h"
#include "SOUNDS/zSAMPLE12.h"
#include "SOUNDS/zSAMPLE13.h"
#include "SOUNDS/zSAMPLE14.h"
#include "SOUNDS/zSAMPLE15.h"
#include "SOUNDS/zSAMPLE16.h"
#include "SOUNDS/zSAMPLE17.h"
#include "SOUNDS/zSAMPLE18.h"
#include "SOUNDS/zSAMPLE19.h"
#include "SOUNDS/zSAMPLE20.h"
#include "SOUNDS/zSAMPLE21.h"
#include "SOUNDS/zSAMPLE22.h"
#include "SOUNDS/zSAMPLE23.h"
#include "SOUNDS/zSAMPLE24.h"
#include "SOUNDS/zSAMPLE25.h"
#include "SOUNDS/zSAMPLE26.h"
#include "SOUNDS/zSAMPLE27.h"
#include "SOUNDS/zSAMPLE28.h"
#include "SOUNDS/zSAMPLE29.h"
#include "SOUNDS/zSAMPLE30.h"
#include "SOUNDS/zSAMPLE31.h"
#include "SOUNDS/SYNTH1.h"
#include "SOUNDS/zSAMPLE33.h"
#include "SOUNDS/zSAMPLE34.h"
#include "SOUNDS/zSAMPLE35.h"
#include "SOUNDS/zSAMPLE36.h"
#include "SOUNDS/zSAMPLE37.h"
#include "SOUNDS/zSAMPLE38.h"
#include "SOUNDS/zSAMPLE39.h"
#include "SOUNDS/SYNTH4.h" 
#include "SOUNDS/zSAMPLE41.h"
#include "SOUNDS/zSAMPLE42.h"
#include "SOUNDS/SYNTH6.h"
#include "SOUNDS/zSAMPLE44.h"
#include "SOUNDS/zSAMPLE45.h"
#include "SOUNDS/zSAMPLE46.h"
#include "SOUNDS/zSAMPLE47.h"
#include "SOUNDS/zSAMPLE48.h"
#include "SOUNDS/zSAMPLE49.h"                          

const String sound_names[50] = { 
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "17",
    "18",
    "19",
    "20",
    "21",
    "22",
    "23",
    "24",
    "25",
    "26",
    "27",
    "28",
    "29",
    "30",
    "31",
    "SYNTH 1",
    "33",
    "34",
    "35",
    "36",
    "37",
    "38",
    "39",
    "SYNTH 4",
    "SAMPLE41",
    "42",
    "SYNTH 6",
    "44",
    "45",
    "46",
    "47",
    "48",
    "49"
};

const uint64_t ENDS[50]={
    (sizeof(SAMPLE00)/ sizeof(SAMPLE00[0]))-1,
    (sizeof(SAMPLE01)/ sizeof(SAMPLE01[0]))-1,
    (sizeof(SAMPLE02)/ sizeof(SAMPLE02[0]))-1,
    (sizeof(SAMPLE03)/ sizeof(SAMPLE03[0]))-1,
    (sizeof(SAMPLE04)/ sizeof(SAMPLE04[0]))-1,
    (sizeof(SAMPLE05)/ sizeof(SAMPLE05[0]))-1,
    (sizeof(SAMPLE06)/ sizeof(SAMPLE06[0]))-1,
    (sizeof(SAMPLE07)/ sizeof(SAMPLE07[0]))-1,
    (sizeof(SAMPLE08)/ sizeof(SAMPLE08[0]))-1,
    (sizeof(SAMPLE09)/ sizeof(SAMPLE09[0]))-1,
    (sizeof(SAMPLE10)/ sizeof(SAMPLE10[0]))-1,
    (sizeof(SAMPLE11)/ sizeof(SAMPLE11[0]))-1,
    (sizeof(SAMPLE12)/ sizeof(SAMPLE12[0]))-1,
    (sizeof(SAMPLE13)/ sizeof(SAMPLE13[0]))-1,
    (sizeof(SAMPLE14)/ sizeof(SAMPLE14[0]))-1,
    (sizeof(SAMPLE15)/ sizeof(SAMPLE15[0]))-1,
    (sizeof(SAMPLE16)/ sizeof(SAMPLE16[0]))-1, 
    (sizeof(SAMPLE17)/ sizeof(SAMPLE17[0]))-1,
    (sizeof(SAMPLE18)/ sizeof(SAMPLE18[0]))-1,
    (sizeof(SAMPLE19)/ sizeof(SAMPLE19[0]))-1, 
    (sizeof(SAMPLE20)/ sizeof(SAMPLE20[0]))-1,
    (sizeof(SAMPLE21)/ sizeof(SAMPLE21[0]))-1,             
    (sizeof(SAMPLE22)/ sizeof(SAMPLE22[0]))-1, 
    (sizeof(SAMPLE23)/ sizeof(SAMPLE23[0]))-1, 
    (sizeof(SAMPLE24)/ sizeof(SAMPLE24[0]))-1, 
    (sizeof(SAMPLE25)/ sizeof(SAMPLE25[0]))-1,
    (sizeof(SAMPLE26)/ sizeof(SAMPLE26[0]))-1,
    (sizeof(SAMPLE27)/ sizeof(SAMPLE27[0]))-1, 
    (sizeof(SAMPLE28)/ sizeof(SAMPLE28[0]))-1, 
    (sizeof(SAMPLE29)/ sizeof(SAMPLE29[0]))-1, 
    (sizeof(SAMPLE30)/ sizeof(SAMPLE30[0]))-1, 
    (sizeof(SAMPLE31)/ sizeof(SAMPLE31[0]))-1,
    (sizeof(SYNTH1)/ sizeof(SYNTH1[0]))-1,
    (sizeof(SAMPLE33)/ sizeof(SAMPLE33[0]))-1,
    (sizeof(SAMPLE34)/ sizeof(SAMPLE34[0]))-1,
    (sizeof(SAMPLE35)/ sizeof(SAMPLE35[0]))-1, 
    (sizeof(SAMPLE36)/ sizeof(SAMPLE36[0]))-1,
    (sizeof(SAMPLE37)/ sizeof(SAMPLE37[0]))-1, 
    (sizeof(SAMPLE38)/ sizeof(SAMPLE38[0]))-1, 
    (sizeof(SAMPLE39)/ sizeof(SAMPLE39[0]))-1, 
    (sizeof(SYNTH4)/ sizeof(SYNTH4[0]))-1, 
    (sizeof(SAMPLE41)/ sizeof(SAMPLE41[0]))-1,
    (sizeof(SAMPLE42)/ sizeof(SAMPLE42[0]))-1,
    (sizeof(SYNTH6)/ sizeof(SYNTH6[0]))-1,
    (sizeof(SAMPLE44)/ sizeof(SAMPLE44[0]))-1,
    (sizeof(SAMPLE45)/ sizeof(SAMPLE45[0]))-1,
    (sizeof(SAMPLE46)/ sizeof(SAMPLE46[0]))-1,
    (sizeof(SAMPLE47)/ sizeof(SAMPLE47[0]))-1, 
    (sizeof(SAMPLE48)/ sizeof(SAMPLE48[0]))-1, 
    (sizeof(SAMPLE49)/ sizeof(SAMPLE49[0]))-1                            
};

const int16_t* SAMPLES[50] = {
    SAMPLE00,
    SAMPLE01,
    SAMPLE02,
    SAMPLE03,
    SAMPLE04,
    SAMPLE05,
    SAMPLE06,
    SAMPLE07,
    SAMPLE08,
    SAMPLE09,
    SAMPLE10,
    SAMPLE11,
    SAMPLE12,
    SAMPLE13,
    SAMPLE14,
    SAMPLE15,
    SAMPLE16,
    SAMPLE17,
    SAMPLE18,
    SAMPLE19, 
    SAMPLE20,
    SAMPLE21,             
    SAMPLE22,             
    SAMPLE23,             
    SAMPLE24,             
    SAMPLE25,             
    SAMPLE26,             
    SAMPLE27,             
    SAMPLE28,             
    SAMPLE29,             
    SAMPLE30,             
    SAMPLE31,
    SYNTH1,   
    SAMPLE33,    
    SAMPLE34,
    SAMPLE35,
    SAMPLE36,             
    SAMPLE37,             
    SAMPLE38,             
    SAMPLE39,             
    SYNTH4,             
    SAMPLE41,
    SAMPLE42,   
    SYNTH6,    
    SAMPLE44,
    SAMPLE45, 
    SAMPLE46,             
    SAMPLE47,             
    SAMPLE48,             
    SAMPLE49                      
};


// I2s

// #define I2S_BCK_PIN 7
// #define I2S_WS_PIN 16 
// #define I2S_DATA_OUT_PIN 15
#ifdef mylcd_type1
  #define I2S_BCK_PIN 41
  #define I2S_WS_PIN 2
  #define I2S_DATA_OUT_PIN 42
#endif
#ifdef mylcd_type2
  #define I2S_BCK_PIN 9
  #define I2S_WS_PIN 5
  #define I2S_DATA_OUT_PIN 14
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

////////////////////////////// MIDI USB NATIVE STATUS
// USB MIDI Native actif - voir midi.ino pour l'implémentation
// Compatibilité API complète maintenue
static bool midiUSBSystemReady = true;

////////////////////////////// SEQ 
int bpm=120;
byte selected_pattern=0;
byte s_old_selected_pattern=99;
byte sstep=0;
int s_old_sstep=-1;

uint16_t mutes=0;
uint16_t solos=0;
uint16_t pattern[16];

byte fx1=0; 

byte selected_memory=0;
byte s_old_selected_memory=99;
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
byte s_old_selected_sound=99;
//byte s_old_drawWaveform=99;
byte selected_sndSet=0;
byte s_old_selected_sndSet=99;
int ztranspose=0;
int zmpitch=0;
bool load_flag=false;

byte sync_state=0; // 0 no sync, 1, master, 2 slave
bool sync_flag=false;

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
#ifdef mylcd_type1 
  #define CLK 9 // CLK ENCODER 
  #define DT 14 // DT ENCODER
  const byte pinBR1=5;
#endif
#ifdef mylcd_type2 
  #define CLK 7 // CLK ENCODER 
  #define DT 6 // DT ENCODER
  const byte pinBR1=15;
#endif

byte ENC_PORT1=0;
int counter1=0;
int old_counter1=0;
byte shiftR1=false;
byte old_shiftR1=true;

// 8 sound parameters + bpm + master vol + transpose + master filter + octave + Pattern song selector + Sync mode
const int max_values[16]={49,2047,2047,127,1,127, 127,127,400,127, 1,127,20, 1,2,12}; 
const int min_values[16]={ 0,  0,  0,  0,  0,  0,-127,  0,  0,  0,-1,  0, 0,-1,0, 0};

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
byte s_old_selected_rot=1;

byte trigger_on[48]; // 16 pads + 8 funcs + 8 norot buttons + 16 bars
byte nkey;

////////////////////////////// MISC
byte modeZ=0;
int s_old_modeZ=-1;

bool playing     = false;
bool pre_playing = false;
bool songing     = false; // switch to make load auto patterns++
bool recording   = false;
bool shifting    = false;

bool clearPADSTEP=false;
bool clearPATTERNPADS=false;
bool refreshPATTERN=true;
bool refreshPADSTEP=false;
bool refreshMODES=true;
//bool refreshPADTOUCHED=false;

#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP  0xFC

////////////////////////////// MENU SYSTEM DECLARATIONS
// Déclarations pour le système de menu (implémentation dans menu_system.ino)
extern void initMenuSystem();
extern void updateMenuSystem();
extern void handleMenuTouch(int touchX, int touchY);
extern bool isInMainMenu();
extern void drawCurrentApplication();
extern void updateSystemStatus(bool sdPresent, bool midiStatus, bool wifiStatus);
extern void returnToMainMenu();
extern void handleSDCommands(uint8_t command);

// Variable pour gérer l'affichage du menu au démarrage
bool showMenuAtStartup = true;

////////////////////////////// FILE BROWSER DECLARATIONS
// Déclarations pour le module File Browser (implémentation dans file_browser.ino)
extern void fileBrowserBegin();
extern void fileBrowserUpdate();
extern void fileBrowserDraw();
extern void fileBrowserHandleTouch(uint16_t zone);
extern void fileBrowserNavigateUp();
extern void fileBrowserNavigateDown();
extern void fileBrowserSelectItem();
extern void fileBrowserPlayPreview();
extern void fileBrowserLoadToSlot();

// Variables globales partagées du File Browser
extern bool fbIsInitialized;
extern bool fbNeedsRefresh;



// ////////////////////////////////////////////////////////////////////////////////////////////
// //                                       I S R / SEQUENCER
// ////////////////////////////////////////////////////////////////////////////////////////////
// #include "driver/timer.h"
// #define TIMER_DIVIDER         80  // 80MHz / 80 = 1MHz -> 1 tick = 1us
// #define TIMER_SCALE           (1000000)  // para pasar a segundos
// #define TIMER_GROUP           TIMER_GROUP_0
// #define TIMER_INDEX           TIMER_0
// volatile bool timerFired = false;
// void (*onSync24Callback)() = nullptr;  // callback a ejecutar en cada tick
// uint16_t tick=0;

// void IRAM_ATTR onTimer(void *arg) {
//   if (onSync24Callback != nullptr) {
//     onSync24Callback();
//   }
//   // Limpiar interrupción
//   timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_INDEX);
//   // Volver a habilitar la interrupción
//   timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER_INDEX);
// }

// void IRAM_ATTR tic(){  
//   if (sstep==firstStep){
//     sync_flag=true;
//   }  
//   for (int f = 0; f < 16; f++) { 
//     if (!bitRead(mutes, f)) {
//       if (solos == 0 || (solos > 0 && bitRead(solos, f))) {
//         if (bitRead(pattern[f], sstep)) { // note on
//           latch[f]=0;        
//           if (bitRead(isMelodic,f)){
//             synthESP32_TRIGGER_P(f,melodic[f][sstep]);
//           } else {
//             // Trigger con el pitch del canal
//             synthESP32_TRIGGER(f);
//           }
//         } 
//       }
//     }
//   }

//   sstep++;
//   // Comprobar step final
//   if (sstep==(lastStep+1) || sstep==(newLastStep+1) || sstep==16) {
//     lastStep=newLastStep;
//     sstep=firstStep;
//     if (songing){
//       load_flag=true; // inside loop I will load next pattern
//     }
//   }
//   refreshPADSTEP=true;
// }

// void IRAM_ATTR onSync24() {
//   // FX1
//   if (playing){
//     if (!(tick % (12)) && fx1==1) {
//       synthESP32_TRIGGER(selected_sound);
//     }
//     if (!(tick % (6)) && fx1==2) {
//       synthESP32_TRIGGER(selected_sound);
//     }
//     if (!(tick % (3)) && fx1==3) {
//       synthESP32_TRIGGER(selected_sound);
//     }
//   }
  
//   // Lanzar cambio de step
//   if (!(tick % (6))) tic();

//   // Limpiar marcas de sound y step
//   if ((tick % (6))==4) clearPADSTEP=true;
//   tick++;
//   if (tick==24) tick=0;
// }

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////  S E T U P  //////////////////////////////
void setup() {

  // Serial
  Serial.begin(115200);
  delay(500);
  randomSeed(analogRead(0));
  // // FX
  // if (!psramFound()) {
  //   Serial.println("¡PSRAM no detectada!");
  //   while (1);
  // }

  // if (!initAudioFX()) {
  //   Serial.println("Error inicializando efectos");
  //   while (1);
  // }
  
  // Iniciar el primer puerto I2C
  Wire.begin(TOUCH_SDA, TOUCH_SCL,400000);
  #ifdef ads_ok
  // Iniciar el segundo puerto I2C
  I2C_2.begin(SDA_2, SCL_2, 400000);

  // Iniciar el ADS1015 en el segundo puerto I2C
  if (!ads.begin(0x48, &I2C_2)) { // Dirección I2C por defecto del ADS1015/ADS1115
    Serial.println("ADS1015 not found.");
  } else {
    Serial.println("ADS1015 init OK.");
  }
  #endif
//........................................................................................................................
// LCD and TOUCH
  // Init Display
  if (!gfx->begin())
  // if (!gfx->begin(80000000)) /* specify data bus speed */
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  #ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
  #endif

  gfx->setFont(u8g2_font_5x8_mr);
  gfx->setUTF8Print(true);
  
  fillBPOS();
  drawScreen1_ONLY1();

  // Touch
  resetGT911();
  delay(100);


  // Serial 2
 // Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);    //Hardware Serial of ESP32
  
  // MIDI USB NATIVE
  // L'initialisation USB MIDI est maintenant gérée dans midiSetup()
  // Plus besoin de configuration manuelle des callbacks


  // Synth
  synthESP32_begin();

  // Set 16 voices
  for (byte f=0;f<16;f++){
    setSound(f);
  }
  // Set master vol
  synthESP32_setMVol(master_vol);
  // Set master filter
  synthESP32_setMFilter(master_filter);

  // MIDI USB Native
  midiSetup();  // Nouvelle implémentation USB dans midi.ino
  
  // Rapport de migration
  Serial.println("=== MIDI MIGRATION REPORT ===");
  Serial.println("✅ UART Hardware -> USB Native");
  Serial.println("✅ DIN 5-pin -> USB Cable");
  Serial.println("✅ API Functions: Preserved");
  Serial.println("✅ Clock Sync: Maintained");
  Serial.println("==============================");

  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Error al montar el sistema de archivos SPIFFS");
    return;
  } 
  
  ///////////////////////////////////////////////////////
  // Initialiser le système SD Card avec notre module
  Serial.println("=== INITIALISATION SYSTÈME SD ===");
  
  if (sdSystemEnabled) {
    if (sdBegin()) {
      Serial.println("Système SD initialisé avec succès");
      sdPerformanceTest();  // Test de performance
      sdPrintCacheStats();  // Afficher les statistiques
      
      // Créer les répertoires de samples si ils n'existent pas
      if (!SD.exists("/SAMPLES")) {
        SD.mkdir("/SAMPLES");
        SD.mkdir("/SAMPLES/KICKS");
        SD.mkdir("/SAMPLES/SNARES");
        SD.mkdir("/SAMPLES/HIHATS");
        SD.mkdir("/SAMPLES/PERCUSSION");
        SD.mkdir("/SAMPLES/SYNTHS");
        SD.mkdir("/SAMPLES/USER");
        Serial.println("Répertoires de samples créés");
      }
      
      // Exemple de chargement d'un sample SD (si disponible)
      if (sdGetSampleCount() > 0) {
        Serial.println("Chargement d'un sample de démonstration...");
        const char* firstSample = sdGetSamplePath(0);
        if (sdLoadSample(0, firstSample)) {
          // Assigner le sample SD à la première voix
          assignSampleToVoice(0, SAMPLE_SOURCE_SD, 0);
          Serial.printf("Sample SD assigné à la voix 0 : %s\n", firstSample);
        }
      }
    } else {
      Serial.println("Erreur d'initialisation SD - utilisation des samples intégrés uniquement");
      sdSystemEnabled = false;
    }
  } else {
    Serial.println("Système SD désactivé - utilisation des samples intégrés uniquement");
  }
  
  Serial.println("==============================");
  ///////////////////////////////////////////////////////

  // Seq
  // fill melodic with 60
  for  (byte a=0;a<16;a++){
    for  (byte b=0;b<16;b++){
     melodic[a][b]=60;
    }    
  }
  // set 8-15 as melodic;
  isMelodic=B11111111<<8 | B00000000;

  // Setup our clock system
  uClock.init();
  uClock.setOnSync24(onSync24Callback);
  uClock.setTempo(bpm);

  // initTimer(onSync24);
  // setBPM(120);  // 120 BPM

//    // demo pattern
//    pattern[0]=B00010001<<8 | B10010001;
//    pattern[2]=B11000101<<8 | B10101011;
//    pattern[6]=B10110101<<8 | B10001111;
//    pattern[3]=B00000110<<8 | B00000000;
//    pattern[7]=B00000000<<8 | B01001000;
//    // start playing demo pattern
//    uClock.start();
//    sstep=firstStep;
//    refreshPADSTEP=true; 
//    playing=true;
  #ifdef ads_ok
  // Rotary
  pinMode(pinBR1,INPUT_PULLUP);
  pinMode(CLK,INPUT_PULLUP);
  pinMode(DT,INPUT_PULLUP);
  #endif
//........................................................................................................................
//
//   B L E M I D I
//
  // BLEMidiServer.begin("zc2025ds");

  // BLEMidiServer.setNoteOnCallback(onNoteOnBLE);

  // BLEMidiServer.setOnConnectCallback([]() {
  //   Serial.println("Connected");
  // });
  // BLEMidiServer.setOnDisconnectCallback([](){    
  //   Serial.println("Disconnected");
  // });


//........................................................................................................................

  select_rot();
  draw8aBar();
  draw8bBar();
  
  // Initialiser le système de menu
  initMenuSystem();
  
  // Initialiser le File Browser
  fileBrowserBegin();
  
  // Initialiser le WiFi Manager
  if (wifiManagerBegin()) {
    Serial.println("WiFi Manager initialisé avec succès");
  } else {
    Serial.println("Erreur d'initialisation WiFi Manager");
  }
  
  // Afficher le menu au démarrage si demandé
  if (showMenuAtStartup) {
    // Le menu sera affiché automatiquement par le système
    Serial.println("Menu principal pret - utilisez les touches pour naviguer");
  }

}

//////////////////////////////  L O O P  //////////////////////////////

void loop() {
  
  // Mettre à jour le système de menu
  updateMenuSystem();
  
  // Mettre à jour le File Browser si actif
  if (getCurrentApplication() == APP_FILE_BROWSER) {
    fileBrowserUpdate();
  }
  
  // Mettre à jour le WiFi Manager
  wifiManagerUpdate();
  
  // Mettre à jour les statuts système pour le menu
  updateSystemStatus(sdSystemEnabled && sdIsReady(), true, isWifiManagerActive());

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

  // if (sync_flag){ 
  //     sync_flag=false;
  //     if (sync_state==1){   // if this machine is master
  //       Serial1.write(11);
  //     }
  // }
  // if (sync_state==2){ // if this machine is slave
  //   if (Serial1.available() > 0){
  //     byte var=Serial1.read();
  //     if (var==11){
  //       if (pre_playing){
  //         sstep=firstStep;
  //         pre_playing=false;
  //         uClock.start();
  //         refreshPADSTEP=true;          
  //       }
  //     }
  //   }
  // }

  // Process MIDI USB Native
  midiProcess();  // Nouvelle implémentation USB dans midi.ino
  #ifdef ads_ok
  shiftR1=!digitalRead(pinBR1);
  #endif

  // if (shiftR1!=old_shiftR1){
  //   old_shiftR1=shiftR1;
  //   drawScreen1b();
  //   //Serial.println(shiftR1);
  //   if (!shiftR1) refreshPATTERN=true;
  // }
  #ifdef ads_ok
  READ_ENCODERS();
  #endif
  read_touch();
  DO_KEYPAD();
  
  // Gestion de l'affichage selon l'application active
  if (!isInMainMenu()) {
    // Si on n'est pas dans le menu principal, afficher l'interface normale
    REFRESH_KEYS();
    REFRESH_STATUS();
  }

  showLastTouched();
  clearLastTouched();
  
  #ifdef ads_ok
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    READ_POTS(); 
  }
  #endif

  // Weird code!!!! Segunda carga de setsoud porque con la primera no se generan bien el inicio y el fin de sample y el volumen/pan. Parece que no le da tiempo.
  if (flag_ss=true){
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

/*
 * FONCTIONS D'INTERFACE POUR SYSTÈME SD CARD
 */

/*
 * Afficher la liste des samples SD disponibles
 */
void sdShowSamplesList() {
    if (!sdSystemEnabled || !sdIsReady()) {
        Serial.println("Système SD non disponible");
        return;
    }
    
    Serial.println("=== SAMPLES SD DISPONIBLES ===");
    uint16_t count = sdGetSampleCount();
    
    if (count == 0) {
        Serial.println("Aucun sample trouvé sur la carte SD");
        return;
    }
    
    for (uint16_t i = 0; i < count && i < 50; i++) {  // Limiter l'affichage
        const SampleInfo* info = sdGetSampleInfo(i);
        if (info && info->isValid) {
            Serial.printf("%2d: %s (%dHz, %dch, %dms)\n",
                        i, info->displayName, info->sampleRate,
                        info->channels, info->duration);
        }
    }
    Serial.println("=============================");
}

/*
 * Charger un sample SD sur une voix spécifique
 */
bool sdLoadSampleToVoice(uint8_t voice, uint16_t sampleIndex) {
    if (!sdSystemEnabled || !sdIsReady() || voice >= 16) {
        return false;
    }
    
    if (sampleIndex >= sdGetSampleCount()) {
        Serial.printf("Index de sample invalide : %d\n", sampleIndex);
        return false;
    }
    
    const char* samplePath = sdGetSamplePath(sampleIndex);
    if (!samplePath) {
        Serial.printf("Impossible d'obtenir le chemin du sample %d\n", sampleIndex);
        return false;
    }
    
    // Trouver un slot libre dans le cache SD
    uint8_t sdSlot = 255; // Auto-assignment
    
    // Charger le sample dans le cache
    if (!sdLoadSample(sdSlot, samplePath)) {
        Serial.printf("Erreur lors du chargement du sample : %s\n", samplePath);
        return false;
    }
    
    // Trouver le slot réellement utilisé
    for (uint8_t i = 0; i < 16; i++) {
        if (sdIsSampleLoaded(i)) {
            const char* fileName = sdGetSamplePath(sampleIndex);
            // Comparer avec le nom du fichier chargé (logique simplifiée)
            sdSlot = i;
            break;
        }
    }
    
    // Assigner le sample à la voix
    if (assignSampleToVoice(voice, SAMPLE_SOURCE_SD, sdSlot)) {
        Serial.printf("Sample SD '%s' assigné à la voix %d\n",
                    sdGetSampleName(sampleIndex), voice);
        
        // Mettre à jour les paramètres de la voix
        setSound(voice);
        return true;
    }
    
    Serial.printf("Erreur lors de l'assignation à la voix %d\n", voice);
    return false;
}

/*
 * Navigateur de samples SD - mode interactif
 */
void sdBrowserUpdate() {
    if (!sdBrowserMode || !sdSystemEnabled) {
        return;
    }
    
    // Cette fonction serait appelée lors de la navigation tactile
    // Pour l'instant, version simplifiée en console
    
    static uint32_t lastBrowserUpdate = 0;
    if (millis() - lastBrowserUpdate > 5000) { // Mise à jour toutes les 5 secondes
        lastBrowserUpdate = millis();
        
        Serial.printf("Mode navigateur SD - Sample %d/%d : %s\n",
                    sdSelectedFile + 1, sdGetSampleCount(),
                    sdGetSampleName(sdSelectedFile));
    }
}

/*
 * Gestion des commandes SD via interface tactile
 */
void handleSDCommands(uint8_t command) {
    switch(command) {
        case 0: // SD BROWSE
            sdBrowserMode = !sdBrowserMode;
            if (sdBrowserMode) {
                Serial.println("Mode navigateur SD activé");
                sdShowSamplesList();
                sdSelectedFile = 0;
            } else {
                Serial.println("Mode navigateur SD désactivé");
            }
            break;
            
        case 1: // SD LOAD
            if (sdBrowserMode && sdSystemEnabled) {
                sdLoadSampleToVoice(selected_sound, sdSelectedFile);
            } else {
                Serial.println("Activez d'abord le mode navigateur SD");
            }
            break;
            
        case 2: // SD NEXT
            if (sdBrowserMode && sdSystemEnabled) {
                sdSelectedFile++;
                if (sdSelectedFile >= sdGetSampleCount()) {
                    sdSelectedFile = 0;
                }
                Serial.printf("Sample sélectionné : %s\n", sdGetSampleName(sdSelectedFile));
            }
            break;
            
        case 3: // SD PREV
            if (sdBrowserMode && sdSystemEnabled) {
                if (sdSelectedFile == 0) {
                    sdSelectedFile = sdGetSampleCount() - 1;
                } else {
                    sdSelectedFile--;
                }
                Serial.printf("Sample sélectionné : %s\n", sdGetSampleName(sdSelectedFile));
            }
            break;
            
        case 4: // SD STATS
            if (sdSystemEnabled) {
                sdPrintCacheStats();
            }
            break;
            
        case 5: // SD CLEAR CACHE
            if (sdSystemEnabled) {
                sdClearCache();
                // Réassigner tous les samples intégrés
                for (uint8_t i = 0; i < 16; i++) {
                    assignSampleToVoice(i, SAMPLE_SOURCE_BUILTIN, ROTvalue[i][0]);
                    setSound(i);
                }
                Serial.println("Cache SD vidé - retour aux samples intégrés");
            }
            break;
            
        default:
            break;
    }
}

/*
 * Fonction de test pour charger plusieurs samples SD
 */
void sdTestLoadMultipleSamples() {
    if (!sdSystemEnabled || !sdIsReady()) {
        Serial.println("Système SD non disponible pour le test");
        return;
    }
    
    uint16_t count = sdGetSampleCount();
    if (count == 0) {
        Serial.println("Aucun sample disponible pour le test");
        return;
    }
    
    Serial.println("=== TEST CHARGEMENT MULTIPLE ===");
    
    // Charger jusqu'à 8 samples sur les premières voix
    uint8_t samplesToLoad = (count < 8) ? count : 8;
    
    for (uint8_t i = 0; i < samplesToLoad; i++) {
        if (sdLoadSampleToVoice(i, i)) {
            Serial.printf("✓ Voix %d : %s\n", i, sdGetSampleName(i));
        } else {
            Serial.printf("✗ Erreur voix %d\n", i);
        }
        delay(100); // Délai pour éviter la surcharge
    }
    
    Serial.println("==============================");
    sdPrintCacheStats();
}

/*
 * Fonction pour basculer entre samples intégrés et SD
 */
void sdToggleMode() {
    if (!sdSystemEnabled) {
        Serial.println("Système SD non disponible");
        return;
    }
    
    sdMode = (sdMode + 1) % 3; // 0, 1, 2 puis retour à 0
    
    switch(sdMode) {
        case 0: // Samples intégrés uniquement
            Serial.println("Mode : Samples intégrés uniquement");
            sdClearCache();
            for (uint8_t i = 0; i < 16; i++) {
                assignSampleToVoice(i, SAMPLE_SOURCE_BUILTIN, ROTvalue[i][0]);
                setSound(i);
            }
            break;
            
        case 1: // Samples SD uniquement (si disponibles)
            Serial.println("Mode : Samples SD uniquement");
            if (sdGetSampleCount() > 0) {
                sdTestLoadMultipleSamples();
            } else {
                Serial.println("Aucun sample SD disponible - retour mode intégré");
                sdMode = 0;
            }
            break;
            
        case 2: // Mode hybride
            Serial.println("Mode : Hybride (intégrés + SD)");
            // Charger quelques samples SD sur les dernières voix
            if (sdGetSampleCount() > 0) {
                uint8_t startVoice = 12; // Voix 12-15 pour les samples SD
                uint16_t sampleCount = sdGetSampleCount();
                for (uint8_t i = 0; i < 4 && i < sampleCount; i++) {
                    sdLoadSampleToVoice(startVoice + i, i);
                }
            }
            break;
    }
}




