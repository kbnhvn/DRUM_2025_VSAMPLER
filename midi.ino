#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// Crée une instance MIDI USB avec Adafruit TinyUSB
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB);

// Canal MIDI global (persisté ailleurs si tu as settings_persist.ino)
uint8_t g_midiChannel = 1;   // 1..16
bool g_sendClock = false;
bool g_recvClock = true;

static inline bool _chMatch(uint8_t ch) { 
  return ch == g_midiChannel || g_midiChannel == 0; 
}

void midiUSB_begin(){
  // Initialisation de l'USB avec TinyUSB
  USB.begin();
  delay(50);
  // Initialisation du MIDI USB (lecture omni)
  MIDI_USB.begin(MIDI_CHANNEL_OMNI); 
}

// Hooks à connecter à ton moteur si besoin
extern void synthESP32_TRIGGER(int nkey);
extern void synthESP32_TRIGGER_P(int nkey, int ppitch);
extern uint8_t melodic[16][16];
extern uint16_t pattern[16];
extern byte selected_sound;

// Lecture des messages MIDI
void midiUSB_poll(){
  // Si aucun message MIDI n'est reçu, on sort
  if (!MIDI_USB.read()) return;

  byte t  = MIDI_USB.getType(); // Type de message MIDI
  byte ch = MIDI_USB.getChannel(); // Canal MIDI
  byte d1 = MIDI_USB.getData1(); // Premier octet de données
  byte d2 = MIDI_USB.getData2(); // Deuxième octet de données

  switch(t){
    case midi::NoteOn:
      if (d2 == 0) break; // Ignore si vélocité = 0
      if (_chMatch(ch)){
        // Déclenche la voix sélectionnée à la hauteur reçue
        synthESP32_TRIGGER_P(selected_sound, d1); 
      }
      break;
    case midi::NoteOff:
      // Optionnel : gestion de NoteOff si tu veux latches plus courts
      break;
    case midi::ControlChange:
      if (_chMatch(ch)){
        extern void synthESP32_setMFilter(unsigned char);
        extern void synthESP32_setMVol(unsigned char);
        if (d1 == 1)  synthESP32_setMFilter(d2); // ModWheel
        if (d1 == 7)  synthESP32_setMVol(d2);    // Volume
      }
      break;
    case midi::Clock:
      if (g_recvClock){
        // Synchronisation avec un "clock" externe (si désiré)
      }
      break;
    case midi::Start:
      // Gestion du transport MIDI externe (si désiré)
      break;
    case midi::Stop:
      // Gestion du transport MIDI externe (si désiré)
      break;
    default:
      break;
  }
}
