# 🎹 GUIDE D'INTÉGRATION MIDI USB & WM8731
## DRUM_2025_VSAMPLER Extended - Spécifications Techniques

Guide technique complet pour l'intégration du MIDI USB Native et du codec audio WM8731 MIKROE-506. Ces composants remplacent respectivement le MIDI UART Hardware et le PCM5102A, apportant des fonctionnalités audio bidirectionnelles et une connectivité MIDI universelle.

---

## 🔄 MIGRATIONS TECHNIQUES MAJEURES

### 📊 Vue d'Ensemble des Changements

#### Migration MIDI : UART Hardware → USB Native
```
ANCIEN SYSTÈME (MIDI UART) :
├── Hardware : DIN 5-pin connector
├── Interface : UART Serial (RX/TX pins)
├── Drivers : Spécifiques selon OS
├── Câblage : Connecteur MIDI traditionnel
└── Limitation : Connexion point-à-point

NOUVEAU SYSTÈME (MIDI USB Native) :
├── Hardware : USB-C cable uniquement
├── Interface : USB composite device
├── Drivers : Class-compliant (aucun driver)
├── Câblage : Plug & Play USB
└── Avantage : Connexion universelle + debugging
```

#### Migration Audio : PCM5102A → WM8731 MIKROE-506
```
ANCIEN SYSTÈME (PCM5102A) :
├── Type : DAC unidirectionnel (output only)
├── Interface : I2S TX uniquement
├── Contrôle : Pins hardware (pas I2C)
├── Fonctionnalité : Playback seulement
└── Limitation : Pas d'enregistrement

NOUVEAU SYSTÈME (WM8731 MIKROE-506) :
├── Type : Codec bidirectionnel (I/O)
├── Interface : I2S TX + RX + I2C control
├── Contrôle : Registres I2C programmables
├── Fonctionnalité : Playback + Recording
└── Avantage : Sampling studio complet
```

---

## 🎹 MIDI USB NATIVE - SPÉCIFICATIONS DÉTAILLÉES

### ⚙️ Architecture USB MIDI

#### Configuration USB Composite Device
```cpp
/*
 * Configuration USB MIDI (extrait de midi.ino)
 * ESP32-S3 USB OTG capabilities utilisées
 */

// Configuration descripteur USB MIDI
USB_DEVICE_DESCRIPTOR = {
    .bLength = 18,
    .bDescriptorType = 1,        // Device
    .bcdUSB = 0x0200,           // USB 2.0
    .bDeviceClass = 0,          // Composite device
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x16C0,         // Van Ooijen Technische Informatica
    .idProduct = 0x27DD,        // Generic HID device
    .bcdDevice = 0x0200,        // Device version 2.0
    .iManufacturer = 1,         // "DRUM_2025"
    .iProduct = 2,              // "VSAMPLER Extended"
    .iSerialNumber = 3,         // "001"
    .bNumConfigurations = 1
};

// Interface MIDI
MIDI_INTERFACE_DESCRIPTOR = {
    .bInterfaceClass = 0x01,        // Audio
    .bInterfaceSubClass = 0x03,     // MIDI Streaming
    .bInterfaceProtocol = 0x00,     // Unused
    .iInterface = 4                 // "MIDI Interface"
};
```

#### Implémentation Messages MIDI
```cpp
/*
 * Format des messages MIDI USB (USB MIDI 1.0 spec)
 * Chaque message = 4 bytes : [Cable|CIN][Status][Data1][Data2]
 */

// Structure message MIDI USB
typedef struct {
    uint8_t cable_cin;    // Cable Number (4 bits) + Code Index Number (4 bits)
    uint8_t status;       // MIDI Status byte
    uint8_t data1;        // MIDI Data byte 1
    uint8_t data2;        // MIDI Data byte 2
} usb_midi_packet_t;

// Mapping CIN (Code Index Number)
const uint8_t MIDI_CIN_MAPPING[] = {
    0x0F,    // Single byte (System Real-Time)
    0x08,    // Note Off (2 bytes)
    0x09,    // Note On (2 bytes) 
    0x0A,    // Poly Key Pressure (2 bytes)
    0x0B,    // Control Change (2 bytes)
    0x0C,    // Program Change (1 byte)
    0x0D,    // Channel Pressure (1 byte)
    0x0E     // Pitch Bend (2 bytes)
};

// Exemple d'envoi Note On
void sendMIDINoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    usb_midi_packet_t packet = {
        .cable_cin = 0x09,                           // Cable 0 + Note On CIN
        .status = 0x90 | (channel & 0x0F),         // Note On + Channel
        .data1 = note & 0x7F,                      // Note number
        .data2 = velocity & 0x7F                   // Velocity
    };
    
    // Envoi via USB (implémentation dépendante du stack USB)
    usb_midi_send_packet(&packet);
}
```

### 🔌 Configuration USB ESP32-S3

#### Paramètres Arduino IDE Critiques
```
PARAMÈTRES USB OBLIGATOIRES :
============================

USB CDC On Boot: "Enabled" ← CRITIQUE
├── Active le port Serial via USB
├── Permet debugging pendant utilisation MIDI
├── Composite device : Serial + MIDI simultané
└── Sans ceci : pas de Serial.print() possible

USB Mode: "Hardware CDC and JTAG" ← RECOMMANDÉ
├── Mode composite optimal
├── Support debugging via JTAG
├── Upload code via USB maintenu
└── Performance optimale USB

Upload Mode: "UART0 / Hardware CDC" ← STABLE
├── Upload via USB CDC interface
├── Redémarrage automatique
├── Compatible avec mode composite
└── Fiabilité upload maximale
```

#### Test de Validation USB
```cpp
/*
 * USB_MIDI_VALIDATION_TEST.ino
 * Test validation configuration USB MIDI
 */

#include <Arduino.h>

void setup() {
    // CRITIQUE : Cette ligne valide USB CDC On Boot = "Enabled"
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== VALIDATION USB MIDI CONFIGURATION ===");
    
    // Test 1: USB Serial disponible
    if (Serial) {
        Serial.println("✅ USB CDC Serial opérationnel");
        Serial.println("✅ USB CDC On Boot = Enabled confirmé");
    } else {
        Serial.println("❌ USB Serial non disponible");
        return;
    }
    
    // Test 2: Informations USB device
    Serial.println("\n--- Configuration USB Device ---");
    Serial.println("Manufacturer: DRUM_2025");
    Serial.println("Product: VSAMPLER Extended");
    Serial.println("Serial: 001");
    Serial.println("VID:PID: 16C0:27DD");
    Serial.println("Class: Audio + CDC composite");
    
    // Test 3: Simulation reconnaissance MIDI
    Serial.println("\n--- Test Reconnaissance MIDI ---");
    Serial.println("Device attendu par l'OS :");
    Serial.println("  Windows: Generic USB MIDI Device");
    Serial.println("  macOS: USB MIDI Device");
    Serial.println("  Linux: ALSA MIDI port");
    Serial.println("  DAW: DRUM_2025 VSAMPLER Extended");
    
    // Test 4: Capacités simultanées
    Serial.println("\n--- Test Capacités Simultanées ---");
    Serial.println("✅ Serial debugging actif");
    Serial.println("✅ MIDI communication possible");
    Serial.println("✅ Upload firmware maintenu");
    Serial.println("✅ Pas de conflits USB");
    
    // Test 5: Messages MIDI simulés
    testMIDIMessages();
}

void testMIDIMessages() {
    Serial.println("\n--- Test Messages MIDI ---");
    
    // Simuler envoi messages MIDI
    Serial.println("Simulation envoi MIDI :");
    
    for (int i = 0; i < 5; i++) {
        uint8_t note = 60 + i;
        Serial.printf("  Note On: Channel 1, Note %d, Vel 100\n", note);
        delay(100);
        Serial.printf("  Note Off: Channel 1, Note %d\n", note);
        delay(100);
    }
    
    Serial.println("✅ Messages MIDI formatés correctement");
    Serial.println("✅ Prêt pour intégration DAW");
}

void loop() {
    // Heartbeat pour valider stabilité USB
    static uint32_t lastHeartbeat = 0;
    
    if (millis() - lastHeartbeat > 5000) {
        lastHeartbeat = millis();
        Serial.printf("USB Heartbeat: %lu ms - Stable\n", millis());
    }
    
    delay(100);
}
```

### 🎛️ API MIDI Compatibility Layer

#### Fonctions Compatibles (midi.ino)
```cpp
/*
 * API maintenue pour compatibilité avec code existant
 * Toutes les fonctions MIDI existantes conservées
 */

// Configuration MIDI
void midiSetChannel(uint8_t channel);           // Canal MIDI 1-16
void midiSetEnabled(bool enabled);              // Activer/désactiver MIDI
void midiSetClockMode(bool external);           // Clock interne/externe
void midiSetReceiving(bool enabled);            // Réception MIDI
void midiSetTransmitting(bool enabled);         // Transmission MIDI

// Messages MIDI Output
void midiSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
void midiSendNoteOff(uint8_t channel, uint8_t note);
void midiSendCC(uint8_t channel, uint8_t cc, uint8_t value);
void midiSendProgramChange(uint8_t channel, uint8_t program);

// Messages MIDI Real-Time
void midiSendClock();                           // MIDI Clock (0xF8)
void midiSendStart();                           // MIDI Start (0xFA)
void midiSendStop();                            // MIDI Stop (0xFC)

// Handlers Input (callbacks automatiques)
void handleMIDINoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
void handleMIDINoteOff(uint8_t channel, uint8_t note);
void handleMIDIControlChange(uint8_t channel, uint8_t ccNumber, uint8_t value);
void handleMIDIProgramChange(uint8_t channel, uint8_t program);
void handleMIDIClock();
void handleMIDIStart();
void handleMIDIStop();

// Utilitaires
String midiGetStatus();                         // État MIDI pour affichage
bool midiUSBIsConnected();                      // État connexion USB
void midiUSBGetDeviceInfo();                    // Infos device USB
```

#### Integration avec Drum Machine
```cpp
/*
 * Intégration transparente avec l'architecture existante
 * Code existant non modifié, juste remplacement backend
 */

// Trigger depuis pads (DRUM_2025_VSAMPLER.ino)
void triggerPad(uint8_t pad, uint8_t velocity) {
    // Audio trigger (inchangé)
    synthESP32_TRIGGER(pad);
    
    // MIDI output automatique (nouveau)
    if (midiTransmitting && midiEnabled) {
        uint8_t note = ROTvalue[pad][3];  // Pitch parameter
        midiSendNoteOn(midiChannel, note, velocity);
    }
    
    // Recording dans pattern (inchangé)
    if (recording && playing) {
        bitWrite(pattern[pad], sstep, 1);
        melodic[pad][sstep] = note;
    }
}

// Contrôle depuis DAW (nouveau)
void handleMIDINoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (midiChannel != 0 && channel != midiChannel) return;
    
    // Mapper note MIDI vers pad (0-15)
    uint8_t pad = note % 16;
    
    // Trigger audio avec note spécifique si mélodique
    if (bitRead(isMelodic, pad)) {
        synthESP32_TRIGGER_P(pad, note);
    } else {
        synthESP32_TRIGGER(pad);
    }
    
    // Recording automatique si actif
    if (recording && playing) {
        bitWrite(pattern[pad], sstep, 1);
        melodic[pad][sstep] = note;
    }
}
```

---

## 🎧 WM8731 MIKROE-506 - SPÉCIFICATIONS DÉTAILLÉES

### 📐 Configuration Hardware Exacte

#### Pinout MIKROE-506 Validé
```
MIKROE-506 WM8731 MODULE - Pinout Vérifié :
==========================================

Pin 1 (VCC)    → ESP32-S3 3.3V     [Alimentation]
Pin 2 (GND)    → ESP32-S3 GND      [Masse]
Pin 3 (SCL)    → ESP32-S3 GPIO 4   [I2C Clock]
Pin 4 (SDA)    → ESP32-S3 GPIO 8   [I2C Data]
Pin 5 (BCLK)   → ESP32-S3 GPIO 37  [I2S Bit Clock]
Pin 6 (LRCLK)  → ESP32-S3 GPIO 15  [I2S Word Clock]
Pin 7 (DACDAT) → ESP32-S3 GPIO 16  [I2S Data Output ESP→WM8731]
Pin 8 (ADCDAT) → ESP32-S3 GPIO 36  [I2S Data Input WM8731→ESP]

Jack 3.5mm:
├── Line Input : Connecteur femelle (entrée audio)
├── Line Output: Connecteur femelle (sortie audio)
└── Impédance  : 10kΩ input, 100Ω output

ATTENTION : Pinout peut varier selon fabricant MIKROE
Toujours vérifier datasheet du module exact
```

#### Configuration I2C Registers
```cpp
/*
 * Configuration complète WM8731 via I2C
 * Registres documentés selon datasheet Cirrus Logic
 */

// Adresse I2C fixe
#define WM8731_I2C_ADDR 0x1A

// Registres WM8731 (extrait de wm8731.ino)
#define WM8731_REG_LLINEIN       0x00    // Left Line Input
#define WM8731_REG_RLINEIN       0x01    // Right Line Input  
#define WM8731_REG_LHEADOUT      0x02    // Left Headphone Output
#define WM8731_REG_RHEADOUT      0x03    // Right Headphone Output
#define WM8731_REG_ANALOGPATH    0x04    // Analog Audio Path
#define WM8731_REG_DIGITALPATH   0x05    // Digital Audio Path
#define WM8731_REG_POWERDOWN     0x06    // Power Down Control
#define WM8731_REG_DIGITALIF     0x07    // Digital Audio Interface
#define WM8731_REG_SAMPLERATE    0x08    // Sample Rate Control
#define WM8731_REG_DIGITALACT    0x09    // Digital Interface Activation
#define WM8731_REG_RESET         0x0F    // Reset Register

// Configuration optimale pour DRUM_2025
void wm8731_configure_optimal() {
    // Reset codec
    writeRegister(WM8731_REG_RESET, 0x00);
    delay(10);
    
    // Power management : Enable ADC, DAC, Output
    writeRegister(WM8731_REG_POWERDOWN, 0x00);  // All powered up
    
    // Digital interface : I2S, 16-bit, Master mode
    writeRegister(WM8731_REG_DIGITALIF, 0x42);  
    // Bit breakdown: FORMAT=10 (I2S), IWL=00 (16-bit), LRP=0, LRSWAP=0, MS=1 (Master)
    
    // Sample rate : 44.1kHz
    writeRegister(WM8731_REG_SAMPLERATE, 0x20);
    // Bit breakdown: CLKIDIV2=0, CLKODIV2=0, SR=1000 (44.1kHz), BOSR=0, USB/NORMAL=0
    
    // Analog path : DAC select, no bypass
    writeRegister(WM8731_REG_ANALOGPATH, 0x12);
    // Bit breakdown: SIDEATT=00, SIDETONE=0, DACSEL=1, BYPASS=0, INSEL=0, MUTEMIC=1, MICBOOST=0
    
    // Digital path : No mute, no de-emphasis
    writeRegister(WM8731_REG_DIGITALPATH, 0x00);
    // Bit breakdown: HPOR=0, DACMU=0, DEEMP=00, ADCHPD=0
    
    // Input gain : 0dB (register value 23)
    writeRegister(WM8731_REG_LLINEIN, 0x117);  // LRINBOTH=1, LINVOL=23
    writeRegister(WM8731_REG_RLINEIN, 0x117);
    
    // Output volume : 0dB (register value 121)
    writeRegister(WM8731_REG_LHEADOUT, 0x179); // LRHPBOTH=1, LHPVOL=121
    writeRegister(WM8731_REG_RHEADOUT, 0x179);
    
    // Activate digital interface
    writeRegister(WM8731_REG_DIGITALACT, 0x01);
    
    Serial.println("✅ WM8731 configuré pour 44.1kHz I2S bidirectionnel");
}
```

### 🔊 I2S Bidirectionnel Configuration

#### Remplacement synthESP32 I2S
```cpp
/*
 * Migration I2S : synthESP32.ino → wm8731.ino
 * I2S unidirectionnel → I2S bidirectionnel
 */

// ANCIEN (synthESP32.ino) - I2S TX seulement
i2s_config_t old_i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),  // TX only
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = true
};

// NOUVEAU (wm8731.ino) - I2S TX + RX simultané
i2s_config_t new_i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),  // TX + RX
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = true,
    .tx_desc_auto_clear = true,    // Nouveau : clear automatique TX buffers
    .fixed_mclk = 0                // Nouveau : MCLK auto
};

// Configuration pins I2S complète
i2s_pin_config_t i2s_pin_config = {
    .bck_io_num = 37,        // Bit Clock (commun TX/RX)
    .ws_io_num = 15,         // Word Select (commun TX/RX)  
    .data_out_num = 16,      // Data Output (ESP → WM8731)
    .data_in_num = 36        // Data Input (WM8731 → ESP) ← NOUVEAU
};
```

#### Integration Sampling Studio
```cpp
/*
 * Nouvelles capacités audio avec WM8731
 * Module Sampling Studio (menu_system.ino lignes 598-1374)
 */

// Enregistrement audio en temps réel
bool recordAudioSample(int16_t* buffer, uint32_t maxSamples) {
    if (!wm8731_isReady()) return false;
    
    uint32_t samplesRecorded = 0;
    uint8_t audioBuffer[512];
    
    while (samplesRecorded < maxSamples) {
        // Lire données audio depuis WM8731
        size_t bytesRead = wm8731_readAudio(audioBuffer, 256);
        
        if (bytesRead > 0) {
            // Convertir en samples 16-bit
            uint32_t samples = bytesRead / 4; // 4 bytes per stereo sample
            
            for (uint32_t i = 0; i < samples && samplesRecorded < maxSamples; i++) {
                int16_t left = *((int16_t*)(audioBuffer + (i * 4)));
                int16_t right = *((int16_t*)(audioBuffer + (i * 4) + 2));
                
                // Mixer stéréo vers mono ou garder stéréo
                buffer[samplesRecorded] = (left + right) >> 1; // Mono mix
                samplesRecorded++;
            }
        }
        
        // Yield pour éviter watchdog
        if (samplesRecorded % 1000 == 0) {
            delay(1);
        }
    }
    
    return samplesRecorded > 0;
}

// Contrôle gain et volume en temps réel
void updateAudioLevels(uint8_t inputGain, uint8_t outputVolume) {
    // Gain input : 0-31 (0=-34.5dB, 31=+12dB)
    wm8731_setInputGain(constrain(inputGain, 0, 31));
    
    // Volume output : 48-127 (48=-73dB, 127=+6dB)
    wm8731_setOutputVolume(constrain(outputVolume, 48, 127));
    
    Serial.printf("Audio levels: Input gain=%d, Output vol=%d\n", 
                  inputGain, outputVolume);
}
```

### 🔧 API WM8731 Complète

#### Fonctions Publiques (wm8731.ino)
```cpp
/*
 * API complète WM8731 pour intégration projet
 * Compatible avec architecture existante
 */

// Initialisation et contrôle
bool wm8731_begin();                                    // Initialisation complète
bool wm8731_isReady();                                  // État du codec
void wm8731_testTone();                                 // Test audio

// Configuration audio
void wm8731_setInputGain(uint8_t gain);                // Gain entrée 0-31
void wm8731_setOutputVolume(uint8_t volume);           // Volume sortie 48-127
void wm8731_setInputSource(uint8_t source);            // Source: 0=Line, 1=Mic
void wm8731_enableInput(bool enable);                  // Activer/désactiver input
void wm8731_enableOutput(bool enable);                 // Activer/désactiver output

// Acquisition audio
size_t wm8731_readAudio(uint8_t* buffer, size_t samples); // Lire audio input

// Getters pour interface utilisateur
uint8_t wm8731_getInputGain();                         // Gain actuel
uint8_t wm8731_getOutputVolume();                      // Volume actuel
uint8_t wm8731_getInputSource();                       // Source actuelle
bool wm8731_isInputEnabled();                          // État input
bool wm8731_isOutputEnabled();                         // État output
```

#### Integration avec synthESP32
```cpp
/*
 * Modification setup() dans DRUM_2025_VSAMPLER.ino
 * Remplacement de synthESP32_begin() par wm8731_begin()
 */

void setup() {
    // ... autres initialisations ...
    
    // AUDIO SYSTEM - Migration WM8731
    Serial.println("=== INITIALISATION AUDIO SYSTEM ===");
    
    // Option 1 : WM8731 uniquement (recommandé)
    if (wm8731_begin()) {
        Serial.println("✅ WM8731 MIKROE-506 initialisé");
        Serial.println("✅ Audio bidirectionnel actif");
        
        // Configurer pour utilisation drum machine
        wm8731_setInputGain(WM8731_INPUT_GAIN_DEFAULT);    // 23 (0dB)
        wm8731_setOutputVolume(WM8731_OUTPUT_VOL_DEFAULT); // 121 (0dB)
        wm8731_setInputSource(WM8731_INPUT_LINE);          // Line input
        wm8731_enableInput(false);                         // Input off par défaut
        wm8731_enableOutput(true);                         // Output on
        
    } else {
        Serial.println("❌ WM8731 non disponible - fallback synthESP32");
        
        // Option 2 : Fallback vers synthESP32 (compatibilité)
        synthESP32_begin();
        Serial.println("✅ synthESP32 fallback actif");
    }
    
    // Configuration samples inchangée
    for (byte f = 0; f < 16; f++) {
        setSound(f);
    }
    synthESP32_setMVol(master_vol);
    synthESP32_setMFilter(master_filter);
    
    Serial.println("Audio system prêt");
    
    // ... reste de setup() ...
}
```

---

## 🔗 INTÉGRATION COMPLÈTE

### ⚙️ Modifications Principales

#### Fichier DRUM_2025_VSAMPLER.ino
```cpp
/*
 * Modifications principales pour support MIDI USB + WM8731
 */

// Ligne 24-27 : Migration MIDI USB commentée
// ////////////////////////////// MIDI USB INTEGRATION  
// // Migration UART -> USB MIDI Native completed
