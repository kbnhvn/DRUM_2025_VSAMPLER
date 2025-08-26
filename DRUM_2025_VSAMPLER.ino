////////////////////////////
// DRUM SAMPLER 2025      //
////////////////////////////
// ZCARLOS 2025
// V002 beta

// Lots of bugs

// includes
#include <Arduino.h>
#include <FS.h>
#include <Arduino_GFX_Library.h>

// ===== Types & Protos partagés (visibles pour tous les .ino agrégés) =====
// Routage audio
enum AudioOut : uint8_t { AUDIO_JACK = 0, AUDIO_SPK = 1 };
void AudioOut_setRoute(enum AudioOut r);   // impl. dans audio_output.ino
enum AudioOut AudioOut_getRoute(void);     // impl. dans audio_output.ino

// WAV header (+ écriture)
struct WavHeader {
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint16_t numChannels;
  uint32_t dataBytes;
};
extern void writeWavHeader(fs::File& f, const WavHeader& h); // impl. dans recorder.ino

// Hooks synth utilisés par keys.ino / midi.ino
void synthESP32_TRIGGER(int nkey);
void synthESP32_TRIGGER_P(uint8_t sound, int key);

// === Afficheur : la lib Dev_Device_Pins fournit bus/gfx -> ne PAS redéfinir ici
extern Arduino_DataBus *bus;
extern Arduino_GFX     *gfx;

// === Encoders retirés : on laisse des symboles neutres pour l'ancien code clavier
volatile bool    shiftR1   = false, shifting = false;
volatile int16_t counter1  = 0,     old_counter1 = 0;

extern void select_rot();

//#define TESTING 1

//#define mylcd_type1
#define mylcd_type2

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
const String tbuttons2[8]       = {"  SHIFT  ", "  - 1   ", "  - 10   ", "  + 10   ", "  + 1   ", "        ", "        ", "  SHIFT "};

////////////////////////////// LCD 
#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE ESP32_4827A043_QSPI
#define GFX_BL 1

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

////////////////////////////// MIDI USB (géré dans midi.ino)
void midiUSB_begin();
void midiUSB_poll();
extern uint8_t g_midiChannel;

// === Navigation par pages (utilisées par ui_main_menu.ino & co) ===
enum PageId { PAGE_MAIN=0, PAGE_SAMPLER, PAGE_RECORDER, PAGE_FILES, PAGE_SETTINGS, PAGE_WIFI, PAGE_SETTINGS_MIDI };
volatile PageId currentPage = PAGE_MAIN;
int LCD_W = 480;   // mis à jour après init écran
int LCD_H = 272;   // idem
void MainMenu_enter();  // défini dans ui_main_menu.ino
void MainMenu_loop();

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

bool clearPADSTEP=false;
bool clearPATTERNPADS=false;
bool refreshPATTERN=true;
bool refreshPADSTEP=false;
bool refreshMODES=true;
//bool refreshPADTOUCHED=false;

#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP  0xFC



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
  
  // Iniciar el primer puerto I2C
  Wire.begin(TOUCH_SDA, TOUCH_SCL,400000);

//........................................................................................................................
// LCD and TOUCH
  gfx = new Arduino_NV3041A(bus, GFX_NOT_DEFINED, 0, true);
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
  LCD_W = gfx->width();
  LCD_H = gfx->height();
  
  fillBPOS();
  drawScreen1_ONLY1();

  // Touch
  resetGT911();
  delay(100);


  // Serial 2
 // Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);    //Hardware Serial of ESP32
  
  // MIDI USB
//  MIDI.begin(MIDI_CHANNEL_OMNI);
//  MIDI.setHandleNoteOn(handleNoteOn);  
//  MIDI.setHandleControlChange(handleCC);
//  // check device mounted
//  if ( !TinyUSBDevice.mounted() ){
//    Serial.println("Error USB");
//  }
  // ==== MIDI USB ====
  midiUSB_begin();
 
  MainMenu_enter(); currentPage = PAGE_MAIN;


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

  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Error al montar el sistema de archivos SPIFFS");
    return;
  } 
  
  ///////////////////////////////////////////////////////
  // Inicializar otro bus SPI para la tarjeta SD
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  // Inicializar la tarjeta SD
  if (!SD.begin(SD_CS,sdSPI)) {
    Serial.println("Error inicializando la tarjeta SD por SPI");
    while (1);
  }
  Serial.println("Tarjeta SD inicializada correctamente");
  // Listar archivos .wav desde el directorio raíz
  //listWavFiles("/");

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
  

}

//////////////////////////////  L O O P  //////////////////////////////

void loop() {
   // MIDI USB polling (transport / notes / clock)
   midiUSB_poll();

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

  // === Touch toujours actif pour dispatcher les pages ===
  read_touch();

  // === Dispatcher des pages ===
  switch (currentPage) {
    case PAGE_MAIN:
      MainMenu_loop();
      break;
    case PAGE_SAMPLER:
      // ——— UI sampler d’origine ———
      DO_KEYPAD();
      REFRESH_KEYS();
      REFRESH_STATUS();
      showLastTouched();
      clearLastTouched();
      break;
    case PAGE_RECORDER:
      extern void Recorder_loop();
      Recorder_loop();
      break;
    case PAGE_FILES:
      extern void Files_loop();
      Files_loop();
      break;
    case PAGE_SETTINGS:
      extern void Settings_loop();
      Settings_loop();
      break;
    case PAGE_SETTINGS_MIDI:
      extern void SettingsMIDI_loop();
      SettingsMIDI_loop();
      break;
    case PAGE_WIFI:
      extern void WIFI_loop();
      WIFI_loop();
      break;
  }

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



