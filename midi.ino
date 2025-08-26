#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);

// Canal MIDI global (persisté ailleurs si tu as settings_persist.ino)
uint8_t g_midiChannel = 1;   // 1..16
bool    g_sendClock   = false;
bool    g_recvClock   = true;

static inline bool _chMatch(uint8_t ch){ return ch==g_midiChannel || g_midiChannel==0; }

void midiUSB_begin(){
  USB.begin();
  delay(50);
  MIDI_USB.begin(MIDI_CHANNEL_OMNI); // lecture omni, on filtre nous-mêmes
}

// Hooks à connecter à ton moteur si besoin
extern void synthESP32_TRIGGER(int nkey);
extern void synthESP32_TRIGGER_P(int nkey, int ppitch);
extern uint8_t melodic[16][16];
extern uint16_t pattern[16];
extern byte selected_sound;

// Lecture des messages
void midiUSB_poll(){
  if (!MIDI_USB.read()) return;

  byte t  = MIDI_USB.getType();
  byte ch = MIDI_USB.getChannel();
  byte d1 = MIDI_USB.getData1();
  byte d2 = MIDI_USB.getData2();

  switch(t){
    case midi::NoteOn:
      if (d2==0) break;
      if (_chMatch(ch)){
        // Déclenche la voix sélectionnée à la hauteur reçue
        synthESP32_TRIGGER_P(selected_sound, d1);
      }
      break;
    case midi::NoteOff:
      // (optionnel) gestion NoteOff si tu veux latches plus courts
      break;
    case midi::ControlChange:
      // Exemple: CC1 → filtre master, CC7 → volume master
      if (_chMatch(ch)){
        extern void synthESP32_setMFilter(unsigned char);
        extern void synthESP32_setMVol(unsigned char);
        if (d1==1)  synthESP32_setMFilter(d2); // ModWheel
        if (d1==7)  synthESP32_setMVol(d2);    // Volume
      }
      break;
    case midi::Clock:
      if (g_recvClock){
        // tu peux raccorder à uClock.tick() si désiré
      }
      break;
    case midi::Start:
      // transport externe (si désiré)
      break;
    case midi::Stop:
      // transport externe (si désiré)
      break;
    default: break;
  }
}