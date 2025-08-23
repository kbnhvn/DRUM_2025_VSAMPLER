# 👨‍💻 DRUM_2025_VSAMPLER - RÉFÉRENCE DÉVELOPPEUR

## 🎯 Guide Technique Complet pour Développeurs

Cette référence développeur fournit toutes les informations techniques nécessaires pour comprendre, modifier, étendre et maintenir le système DRUM_2025_VSAMPLER Extended.

---

## 🏗️ Architecture Système

### 📂 Structure du Projet

```
DRUM_2025_VSAMPLER/
├── DRUM_2025_VSAMPLER.ino    # Point d'entrée principal
├── Audio Engine/
│   ├── synthESP32.ino        # Moteur audio 16-voix
│   ├── wm8731.ino           # Driver codec audio bidirectionnel
│   ├── samples.ino          # Samples intégrés flash
│   └── reverb.ino           # Effets audio (expérimental)
├── Interface System/
│   ├── menu_system.ino      # Hub navigation 6 applications
│   ├── LCD_tools.ino        # Primitives affichage TFT
│   ├── touch.ino            # Gestion système tactile
│   └── keys.ino             # Routage événements touch
├── Storage & I/O/
│   ├── sd.ino               # Système SD Card + cache LRU
│   ├── file_browser.ino     # Navigation samples SD
│   └── files_tools.ino      # Utilitaires gestion fichiers
├── Connectivity/
│   ├── wifi_manager.ino     # Gestionnaire WiFi principal
│   ├── wifi_manager_interface.ino # Interface web serveur
│   └── midi.ino             # Communication MIDI UART
├── Applications/
│   ├── sampling_app.ino     # Studio enregistrement
│   ├── sequencer.ino        # Séquenceur patterns
│   └── rots.ino             # Contrôles rotatifs/ADS1115
└── System Services/
    ├── integration_tests.ino    # Framework tests validation
    ├── performance_monitor.ino  # Monitoring temps réel
    └── custom_partitions.csv    # Configuration mémoire ESP32
```

### 🧵 Architecture Multi-Tâches

**Core 0 - Audio Priority (Real-time)**
```cpp
// Tâche audio haute priorité
void audio_task(void* parameter) {
    while(1) {
        // Traitement audio temps réel
        synthesize_audio_buffer();    // synthESP32 processing
        i2s_write_buffer();          // WM8731 output
        i2s_read_buffer();           // WM8731 input (si sampling)
        
        // Yield minimal pour autres tâches
        vTaskDelay(1); // 1ms yield
    }
}

// Création tâche - Core 0, priorité max
xTaskCreatePinnedToCore(
    audio_task,                  // Function
    "audio",                     // Name
    8000,                       // Stack size
    NULL,                       // Parameters
    configMAX_PRIORITIES - 1,   // Priority (max)
    NULL,                       // Handle
    0                          // Core 0
);
```

**Core 1 - Interface & Services**
```cpp
void loop() {
    // Exécution séquentielle Core 1
    updateMenuSystem();          // Interface utilisateur
    performanceMonitorUpdate();  // Monitoring système
    wifiManagerUpdate();         // Services réseau
    fileBrowserUpdate();         // Navigation SD
    midiProcessMessages();       // Communication MIDI
    
    // Yield court pour autres tâches
    yield();
}
```

### 🔄 Communication Inter-Core

**Variables Partagées Protégées**
```cpp
// Synchronisation thread-safe
portMUX_TYPE audioMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint8_t activeVoices = 0;
volatile bool audioSystemActive = false;

// Macros protection critique
#define AUDIO_ENTER_CRITICAL() portENTER_CRITICAL(&audioMux)
#define AUDIO_EXIT_CRITICAL()  portEXIT_CRITICAL(&audioMux)

// Usage sécurisé
void setActiveVoices(uint8_t count) {
    AUDIO_ENTER_CRITICAL();
    activeVoices = count;
    AUDIO_EXIT_CRITICAL();
}
```

**Queues Communication**
```cpp
// Queue pour commandes audio
QueueHandle_t audioCommandQueue;

struct AudioCommand {
    enum Type { TRIGGER_VOICE, SET_PARAMETER, STOP_ALL } type;
    uint8_t voice;
    uint8_t parameter;
    uint16_t value;
};

// Envoi commande depuis Core 1
AudioCommand cmd = {TRIGGER_VOICE, voiceId, 0, velocity};
xQueueSend(audioCommandQueue, &cmd, 0); // Non-blocking

// Réception dans tâche audio Core 0
AudioCommand cmd;
if (xQueueReceive(audioCommandQueue, &cmd, 0) == pdTRUE) {
    processAudioCommand(&cmd);
}
```

---

## 🎵 Audio Engine API

### 🎛️ synthESP32 Core Engine

#### Initialisation Audio

```cpp
// Séquence initialisation CRITIQUE
void setup() {
    // 1. OBLIGATOIRE : WM8731 avant synthESP32
    if (!wm8731_begin()) {
        Serial.println("ERREUR: WM8731 échec initialisation");
        while(1) delay(1000);
    }
    
    // 2. Configuration WM8731
    wm8731_setOutputVolume(100);
    wm8731_setInputGain(20);
    wm8731_enableOutput(true);
    
    // 3. synthESP32 sans I2S (déjà configuré par WM8731)
    synthESP32_begin_without_i2s();
    
    // 4. Autres initialisations...
}
```

#### API Voix Audio

```cpp
// Déclenchement voix
void synthESP32_TRIGGER(uint8_t voice, uint8_t velocity = 127);

// Contrôle paramètres voix
void synthESP32_setVolume(uint8_t voice, uint8_t volume);        // 0-127
void synthESP32_setPan(uint8_t voice, int8_t pan);              // -64 à +63
void synthESP32_setPitch(uint8_t voice, int16_t pitch);         // -24 à +24 semitones
void synthESP32_setFilter(uint8_t voice, uint8_t cutoff);       // 0-127
void synthESP32_setBeginEnd(uint8_t voice, uint16_t begin, uint16_t end);
void synthESP32_setReverse(uint8_t voice, bool reverse);

// Gestion samples
void synthESP32_assignSample(uint8_t voice, SampleSource source, uint8_t sampleIndex);

// Monitoring
uint8_t synthESP32_getActiveVoices();
uint16_t synthESP32_getLatencyMicros();
uint8_t synthESP32_getCPUUsage();
```

#### Système Hybride Samples

```cpp
enum SampleSource {
    SAMPLE_SOURCE_BUILTIN = 0,    // Samples intégrés SOUNDS/*.h
    SAMPLE_SOURCE_SD = 1          // Samples SD Card cache
};

// Structure sample unifié
struct Sample {
    int16_t* data;               // Pointeur données audio
    uint32_t length;             // Longueur échantillons
    uint8_t channels;            // 1=mono, 2=stéréo
    uint32_t sampleRate;         // Fréquence échantillonnage
    SampleSource source;         // Source origine
    uint8_t slot;               // Slot cache si SD
};

// API assignation samples
bool assignSampleToVoice(uint8_t voice, SampleSource source, uint8_t index) {
    if (voice >= MAX_VOICES) return false;
    
    Sample* sample = nullptr;
    
    switch(source) {
        case SAMPLE_SOURCE_BUILTIN:
            sample = getBuiltinSample(index);
            break;
        case SAMPLE_SOURCE_SD:
            sample = sdGetSampleData(index);
            break;
    }
    
    if (sample) {
        voiceSamples[voice] = sample;
        return true;
    }
    return false;
}
```

### 🎚️ WM8731 Audio Codec

#### API Complète

```cpp
// Initialisation
bool wm8731_begin();                          // Init I2S + I2C
bool wm8731_isReady();                        // Test présence codec

// Configuration sortie
void wm8731_setOutputVolume(uint8_t volume);  // 48-127 (0dB = 121)
void wm8731_enableOutput(bool enable);        // DAC on/off

// Configuration entrée
void wm8731_setInputGain(uint8_t gain);       // 0-31 (-34.5dB à +12dB)
void wm8731_setInputSource(uint8_t source);   // 0=Line, 1=Microphone
void wm8731_enableInput(bool enable);         // ADC on/off

// Lecture audio (sampling)
size_t wm8731_readAudio(uint8_t* buffer, size_t samples);

// Status et diagnostic
uint8_t wm8731_getInputGain();
uint8_t wm8731_getOutputVolume();
bool wm8731_isInputEnabled();
bool wm8731_isOutputEnabled();
void wm8731_testTone();                       // Tone test 1kHz
```

#### Registres WM8731 (Bas Niveau)

```cpp
// Adresses registres I2C
#define WM8731_REG_LLINEIN          0x00    // Left Line Input
#define WM8731_REG_RLINEIN          0x01    // Right Line Input  
#define WM8731_REG_LHEADOUT         0x02    // Left Headphone Output
#define WM8731_REG_RHEADOUT         0x03    // Right Headphone Output
#define WM8731_REG_ANALOG           0x04    // Analog Audio Path
#define WM8731_REG_DIGITAL          0x05    // Digital Audio Path
#define WM8731_REG_POWERDOWN        0x06    // Power Down Control
#define WM8731_REG_INTERFACE        0x07    // Digital Audio Interface
#define WM8731_REG_SAMPLING         0x08    // Sampling Control
#define WM8731_REG_ACTIVE           0x09    // Active Control
#define WM8731_REG_RESET            0x0F    // Reset Register

// Fonctions bas niveau
bool wm8731_writeRegister(uint8_t reg, uint16_t value);
uint16_t wm8731_readRegister(uint8_t reg);
void wm8731_reset();
```

---

## 💾 Storage System API

### 🗃️ SD Card System

#### Cache LRU Intelligent

```cpp
// Structure cache slot
struct CacheSlot {
    bool used;                   // Slot occupé
    uint32_t lastAccess;         // Timestamp dernier accès LRU
    char filename[128];          // Nom fichier complet
    int16_t* data;              // Données audio
    uint32_t length;            // Longueur échantillons
    uint8_t channels;           // Nombre canaux
    uint32_t sampleRate;        // Sample rate
    uint32_t fileSize;          // Taille fichier bytes
};

#define SD_CACHE_SLOTS 16
CacheSlot sdCache[SD_CACHE_SLOTS];

// API cache
bool sdLoadSample(uint8_t slot, const char* filename);
void sdUnloadSample(uint8_t slot);
bool sdIsSampleLoaded(uint8_t slot);
uint8_t sdFindAvailableSlot();               // LRU éviction si nécessaire
void sdEvictLRUSlot();                       // Éviction forcée LRU
void sdClearCache();                         // Vider cache complet
```

#### Navigation Système

```cpp
// Structure informations sample
struct SampleInfo {
    char name[64];              // Nom affichage
    char path[128];             // Chemin complet
    uint8_t type;              // 0=parent, 1=dir, 2=sample
    uint32_t size;             // Taille fichier
    bool valid;                // Format valide
};

// Variables navigation globales
char fbCurrentPath[128];        // Répertoire courant
SampleInfo fbSamples[256];      // Liste samples scannés
uint16_t fbSampleCount;         // Nombre items
uint16_t fbSelectedIndex;       // Index sélectionné

// API navigation
uint16_t sdScanSamples(const char* directory);
bool sdNavigateToPath(const char* path);
bool sdNavigateUp();                         // Répertoire parent
const SampleInfo* sdGetSampleInfo(uint16_t index);
bool sdIsValidWAVFile(const char* filename);
```

#### WAV File Parser

```cpp
// Header WAV standard
struct WAVHeader {
    char chunkID[4];            // "RIFF"
    uint32_t chunkSize;         // Taille fichier - 8
    char format[4];             // "WAVE"
    char subchunk1ID[4];        // "fmt "
    uint32_t subchunk1Size;     // 16 pour PCM
    uint16_t audioFormat;       // 1 pour PCM
    uint16_t numChannels;       // 1=mono, 2=stéréo
    uint32_t sampleRate;        // Ex: 44100
    uint32_t byteRate;          // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;        // numChannels * bitsPerSample/8
    uint16_t bitsPerSample;     // 16-bit supporté
    char subchunk2ID[4];        // "data"
    uint32_t subchunk2Size;     // Taille données audio
};

// Parser WAV
bool parseWAVFile(const char* filename, WAVHeader* header, int16_t** data) {
    File file = SD.open(filename, FILE_READ);
    if (!file) return false;
    
    // Lecture header
    file.read((uint8_t*)header, sizeof(WAVHeader));
    
    // Validations
    if (strncmp(header->chunkID, "RIFF", 4) != 0) return false;
    if (strncmp(header->format, "WAVE", 4) != 0) return false;
    if (header->audioFormat != 1) return false;  // PCM seulement
    if (header->bitsPerSample != 16) return false;
    
    // Allocation mémoire
    uint32_t dataSize = header->subchunk2Size;
    *data = (int16_t*)malloc(dataSize);
    if (!*data) return false;
    
    // Lecture données
    size_t bytesRead = file.read((uint8_t*)*data, dataSize);
    file.close();
    
    return (bytesRead == dataSize);
}
```

---

## 📱 Interface System API

### 🖥️ Menu System Architecture

#### État Applications

```cpp
enum AppMode {
    APP_MAIN_MENU = 0,
    APP_DRUM_MACHINE,
    APP_SAMPLING_STUDIO,
    APP_FILE_BROWSER,
    APP_MIDI_SETTINGS,
    APP_WIFI_SETTINGS,
    APP_SYSTEM_SETTINGS
};

// Variables état globales
AppMode currentApplication = APP_MAIN_MENU;
AppMode previousApplication = APP_MAIN_MENU;
uint32_t lastTransitionTime = 0;
bool transitionInProgress = false;

// API navigation
void switchToApplication(AppMode app);
void returnToMainMenu();
AppMode getCurrentApplication();
bool isInMainMenu();
void updateMenuSystem();
```

#### Système Transitions

```cpp
// Transition animée fluide
void switchToApplication(AppMode app) {
    if (transitionInProgress) return;
    
    transitionInProgress = true;
    previousApplication = currentApplication;
    
    // Animation fade out (150ms)
    for (int i = 255; i >= 0; i -= 17) {
        fadeScreen(i);
        delay(10);
    }
    
    // Changement application
    currentApplication = app;
    
    // Cleanup application précédente
    cleanupApplication(previousApplication);
    
    // Initialisation nouvelle application
    initializeApplication(app);
    
    // Animation fade in (150ms)
    for (int i = 0; i <= 255; i += 17) {
        fadeScreen(i);
        drawCurrentApplication();
        delay(10);
    }
    
    transitionInProgress = false;
    lastTransitionTime = millis();
}
```

#### Boutons et Zones

```cpp
// Structure bouton menu
struct MenuButton {
    const char* name;           // Nom affiché
    const char* shortcut;       // Raccourci clavier
    uint16_t color;            // Couleur bouton
    AppMode targetApp;         // Application cible
    uint8_t zone1, zone2;      // Zones tactiles (étendue possible)
};

// Configuration boutons menu principal
MenuButton menuButtons[] = {
    {"DRUM MACHINE", "D", ZGREEN, APP_DRUM_MACHINE, 0, 1},
    {"SAMPLING STUDIO", "S", ZRED, APP_SAMPLING_STUDIO, 2, 3},
    {"FILE BROWSER", "F", ZBLUE, APP_FILE_BROWSER, 4, 5},
    {"MIDI SETTINGS", "M", ZCYAN, APP_MIDI_SETTINGS, 16, 16},
    {"WIFI SETTINGS", "W", ZMAGENTA, APP_WIFI_SETTINGS, 17, 17},
    {"SYSTEM SETTINGS", "Y", ZYELLOW, APP_SYSTEM_SETTINGS, 18, 18}
};

// Gestion touch menu
void handleMenuTouch(int touchX, int touchY) {
    uint8_t zone = getTouchZone(touchX, touchY);
    
    for (int i = 0; i < sizeof(menuButtons)/sizeof(MenuButton); i++) {
        MenuButton* btn = &menuButtons[i];
        if (zone >= btn->zone1 && zone <= btn->zone2) {
            showTouchFeedback(zone);
            switchToApplication(btn->targetApp);
            return;
        }
    }
}
```

### 🎨 Graphics Engine

#### Primitives Affichage

```cpp
// Couleurs système (16-bit RGB565)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define ZRED    0xF800    // Rouge pur
#define ZGREEN  0x07E0    // Vert pur
#define ZBLUE   0x001F    // Bleu pur
#define ZYELLOW 0xFFE0    // Jaune
#define ZCYAN   0x07FF    // Cyan
#define ZMAGENTA 0xF81F   // Magenta
#define DARKGREY 0x39C7   // Gris foncé

// API boutons standardisée
void drawBT(uint8_t zone, uint16_t color, const char* text);
void drawBTPAD(uint8_t zone, uint16_t color, const char* text);
void showLastTouched(uint8_t zone);
void clearLastTouched();

// Primitives graphiques avancées
void drawProgressBar(int x, int y, int width, int height, int progress, uint16_t color);
void drawWaveform(int16_t* data, int length, int x, int y, int width, int height);
void drawVUMeter(int level, int x, int y, int width, int height);
```

#### Gestion Zones Tactiles

```cpp
// Mapping zones tactiles BPOS[48][4] = {x1, y1, x2, y2}
int BPOS[48][4] = {
    {10, 80, 110, 130},     // Zone 0
    {120, 80, 220, 130},    // Zone 1
    {230, 80, 330, 130},    // Zone 2
    // ... 45 autres zones
};

// Détection zone touch
uint8_t getTouchZone(int touchX, int touchY) {
    for (int i = 0; i < 48; i++) {
        if (touchX >= BPOS[i][0] && touchX <= BPOS[i][2] &&
            touchY >= BPOS[i][1] && touchY <= BPOS[i][3]) {
            return i;
        }
    }
    return 255; // Zone invalide
}

// Feedback tactile
void showTouchFeedback(uint8_t zone) {
    if (zone >= 48) return;
    
    // Highlight zone
    gfx->fillRect(BPOS[zone][0], BPOS[zone][1], 
                  BPOS[zone][2] - BPOS[zone][0],
                  BPOS[zone][3] - BPOS[zone][1], ZYELLOW);
    delay(100);
    
    // Redraw normal
    drawCurrentApplication();
}
```

---

## 🌐 Connectivity APIs

### 📶 WiFi Manager

#### Configuration Réseau

```cpp
// États WiFi
enum WiFiState {
    WIFI_DISABLED,
    WIFI_SCANNING,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR,
    WIFI_AP_MODE
};

// Variables état WiFi
WiFiState wifiState = WIFI_DISABLED;
char wifiSSID[32];
char wifiPassword[64];
IPAddress localIP;
bool webServerRunning = false;

// API principale
bool wifiManagerBegin();
void wifiSetMode(WiFiMode mode);              // WIFI_STA ou WIFI_AP
bool wifiConnect(const char* ssid, const char* password);
void wifiStartAP(const char* ssid, const char* password);
void wifiStop();
WiFiState wifiGetState();
```

#### Serveur Web Intégré

```cpp
// Serveur HTTP
WebServer server(80);

// Handlers endpoint
void handleRoot();              // Page principale
void handleFileList();          // Liste fichiers JSON
void handleFileUpload();        // Upload fichier
void handleFileDownload();      // Download fichier
void handleFileDelete();        // Suppression fichier
void handleStatus();            // Status système JSON

// Initialisation serveur
void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/files", HTTP_GET, handleFileList);
    server.on("/api/upload", HTTP_POST, handleFileUpload);
    server.on("/download", HTTP_GET, handleFileDownload);
    server.on("/api/delete", HTTP_DELETE, handleFileDelete);
    server.on("/api/status", HTTP_GET, handleStatus);
    
    // Upload handler avec progress
    server.on("/api/upload", HTTP_POST, 
        []() { server.send(200, "text/plain", "OK"); },
        handleFileUploadData
    );
    
    server.begin();
    webServerRunning = true;
}
```

#### Système Priorité Audio

```cpp
// Monitoring audio priorité CRITIQUE
volatile bool audioSystemActive = false;
uint32_t lastAudioActivity = 0;

// Vérification automatique
void wifiCheckAudioPriority() {
    bool audioActive = isAudioSystemActive();
    
    if (audioActive && wifiGetState() == WIFI_CONNECTED) {
        Serial.println("🔊 AUDIO ACTIF - Arrêt WiFi immédiat");
        wifiForceDisable();
        audioSystemActive = true;
    }
    
    if (!audioActive && audioSystemActive) {
        uint32_t inactiveTime = millis() - lastAudioActivity;
        if (inactiveTime > 5000) {  // 5s grace period
            audioSystemActive = false;
            Serial.println("🔇 Audio libre - WiFi peut redémarrer");
        }
    }
}

// Détection activité audio
bool isAudioSystemActive() {
    return (synthESP32_getActiveVoices() > 0) ||
           wm8731_isInputEnabled() ||
           isPreviewPlaying() ||
           isSequencerRunning();
}
```

### 🎛️ MIDI Communication

#### UART Hardware

```cpp
// Configuration UART2
#define MIDI_RX_PIN 16
#define MIDI_TX_PIN 17
#define MIDI_BAUD_RATE 31250

// Variables MIDI globales
uint8_t midiChannel = 1;                     // Canal principal
bool midiEnabled = true;
bool midiClockExternal = false;              // false=master, true=slave
bool midiReceiving = true;
bool midiTransmitting = true;

// Buffer circular MIDI
#define MIDI_BUFFER_SIZE 256
uint8_t midiRXBuffer[MIDI_BUFFER_SIZE];
uint8_t midiTXBuffer[MIDI_BUFFER_SIZE];
volatile uint16_t midiRXHead = 0, midiRXTail = 0;
volatile uint16_t midiTXHead = 0, midiTXTail = 0;
```

#### Protocol MIDI

```cpp
// Messages MIDI standard
void midiSendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!midiTransmitting) return;
    
    uint8_t status = 0x90 | (channel - 1);   // Note On + canal
    midiSendByte(status);
    midiSendByte(note & 0x7F);
    midiSendByte(velocity & 0x7F);
}

void midiSendNoteOff(uint8_t channel, uint8_t note) {
    if (!midiTransmitting) return;
    
    uint8_t status = 0x80 | (channel - 1);   // Note Off + canal
    midiSendByte(status);
    midiSendByte(note & 0x7F);
    midiSendByte(0x00);                      // Velocity 0
}

void midiSendCC(uint8_t channel, uint8_t cc, uint8_t value) {
    if (!midiTransmitting) return;
    
    uint8_t status = 0xB0 | (channel - 1);   // Control Change + canal
    midiSendByte(status);
    midiSendByte(cc & 0x7F);
    midiSendByte(value & 0x7F);
}

// Réception messages
void midiProcessIncomingByte(uint8_t byte) {
    static uint8_t midiState = 0;
    static uint8_t midiData1 = 0;
    static uint8_t midiStatus = 0;
    
    if (byte & 0x80) {
        // Status byte
        midiStatus = byte;
        midiState = 1;
    } else {
        // Data byte
        switch(midiState) {
            case 1:
                midiData1 = byte;
                midiState = 2;
                break;
            case 2:
                // Message complet
                processMIDIMessage(midiStatus, midiData1, byte);
                midiState = 1;
                break;
        }
    }
}
```

---

## 🧪 Testing Framework

### ⚡ Integration Tests

#### Architecture Tests

```cpp
// Énumération types tests
enum TestMode {
    TEST_MODE_QUICK,        // 30s - validation essentielle
    TEST_MODE_FULL,         // 5min - validation complète
    TEST_MODE_STRESS,       // 10min - robustesse
    TEST_MODE_CONTINUOUS    // ∞ - endurance
};

// Structure résultats tests
struct IntegrationTestResults {
    bool overallPassed;              // Résultat global
    uint8_t overallScore;            // Score 0-100
    uint32_t duration;               // Durée tests ms
    
    // Scores par catégorie
    uint8_t audioScore;              // Performance audio
    uint8_t interfaceScore;          // Interface utilisateur
    uint8_t memoryScore;             // Gestion mémoire
    uint8_t connectivityScore;       // Connectivité
    uint8_t stressScore;             // Tests stress
    
    // Métriques détaillées
    uint16_t latencyMicros;          // Latence audio μs
    uint8_t maxVoices;               // Polyphonie atteinte
    uint16_t heapFreeKB;             // Heap libre KB
    uint8_t fragmentationPercent;    // Fragmentation %
    uint8_t cacheHitRate;            // Cache SD %
};
```

#### Métriques Temps Réel

```cpp
// Structure métriques audio
struct AudioMetrics {
    uint16_t latencyUs;              // Latence μs
    uint8_t activeVoices;            // Voix actives
    uint8_t cpuUsage;                // CPU usage %
    uint32_t underruns;              // Buffer underruns
    bool wm8731Ready;                // Codec OK
};

// Structure métriques mémoire
struct MemoryMetrics {
    uint16_t heapFree;               // Heap libre KB
    uint16_t heapMin;                // Heap minimum KB
    uint8_t fragmentation;           // Fragmentation %
    uint16_t largestBlock;           // Plus grand bloc KB
    uint16_t psramFree;              // PSRAM libre KB
};

// API monitoring
AudioMetrics* getAudioMetrics();
MemoryMetrics* getMemoryMetrics();
UIMetrics* getUIMetrics();
ConnectivityMetrics* getConnectivityMetrics();
```

#### Tests Automatiques

```cpp
// Tests audio essentiels
bool runAudioLatencyTest() {
    uint32_t startTime = esp_timer_get_time();
    synthESP32_TRIGGER(0, 127);
    // Mesure temps jusqu'à sortie I2S
    uint32_t latencyUs = measureAudioLatency();
    
    bool passed = (latencyUs < 10000);  // <10ms requis
    logTestResult("Audio Latency", passed, 
                  String(latencyUs/1000.0) + "ms");
    return passed;
}

bool runPolyphonyTest() {
    // Déclencher 16 voix simultanément
    for (int i = 0; i < 16; i++) {
        synthESP32_TRIGGER(i, 100);
        delay(10);
    }
    
    delay(100);  // Laisser stabiliser
    
    uint8_t activeVoices = synthESP32_getActiveVoices();
    bool passed = (activeVoices >= 16);
    
    logTestResult("Polyphony 16-voice", passed, 
                  String(activeVoices) + "/16");
    return passed;
}

// Tests mémoire
bool runMemoryStabilityTest() {
    uint32_t startHeap = ESP.getFreeHeap();
    
    // Stress allocation/liberation
    for (int i = 0; i < 1000; i++) {
        void* ptr = malloc(1024);
        if (ptr) free(ptr);
        
        if (i % 100 == 0) {
            uint32_t currentHeap = ESP.getFreeHeap();
            if (currentHeap < startHeap - 10240) { // -10KB threshold
                logTestResult("Memory Stability", false, "Leak detected");
                return false;
            }
        }
    }
    
    logTestResult("Memory Stability", true, "1000 cycles OK");
    return true;
}
```

### 📊 Performance Monitor

#### Monitoring Continu

```cpp
// Configuration monitoring
#define MONITOR_UPDATE_FAST_MS 100       // Mise à jour rapide
#define MONITOR_UPDATE_SLOW_MS 1000      // Mise à jour lente
#define MONITOR_HISTORY_SIZE 60          // 60 points historique

// Structure point historique
struct HistoryPoint {
    uint32_t timestamp;                  // Horodatage
    uint8_t cpuUsage;                    // Usage CPU %
    uint16_t heapFree;                   // Heap libre KB
    uint8_t activeVoices;                // Voix actives
    uint8_t systemStability;             // Stabilité système %
    uint16_t latencyUs;                  // Latence audio μs
};

HistoryPoint performanceHistory[MONITOR_HISTORY_SIZE];
uint8_t historyIndex = 0;

// Update monitoring
void performanceMonitorUpdate() {
    static uint32_t lastFastUpdate = 0;
    static uint32_t lastSlowUpdate = 0;
    
    uint32_t now = millis();
    
    // Mise à jour rapide (100ms)
    if (now - lastFastUpdate > MONITOR_UPDATE_FAST_MS) {
        updateFastMetrics();
        updateDisplayIfNeeded();
        lastFastUpdate = now;
    }
    
    // Mise à jour lente (1s)
    if (now - lastSlowUpdate > MONITOR_UPDATE_SLOW_MS) {
        updateSlowMetrics();
        addHistoryPoint();
        checkSystemHealth();
        lastSlowUpdate = now;
    }
}
```

---

## 🔧 Development Tools

### 🛠️ Debug Utilities

#### Logging System

```cpp
// Niveaux debug configurables
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARNING 2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_DEBUG   4
#define DEBUG_LEVEL_VERBOSE 5

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

// Macros debug
#define DEBUG_ERROR(msg)   if(DEBUG_LEVEL >= 1) Serial.println("[ERROR] " + String(msg))
#define DEBUG_WARNING(msg) if(DEBUG_LEVEL >= 2) Serial.println("[WARN] " + String(msg))
#define DEBUG_INFO(msg)    if(DEBUG_LEVEL >= 3) Serial.println("[INFO] " + String(msg))
#define DEBUG_DEBUG(msg)   if(DEBUG_LEVEL >= 4) Serial.println("[DEBUG] " + String(msg))
#define DEBUG_VERBOSE(msg) if(DEBUG_LEVEL >= 5) Serial.println("[VERBOSE] " + String(msg))

// Debug conditionnel compilateur
#ifdef DRUM_DEBUG
    #define DBG_PRINT(x) Serial.print(x)
    #define DBG_PRINTLN(x) Serial.println(x)
#else
    #define DBG_PRINT(x)
    #define DBG_PRINTLN(x)
#endif
```

#### Profiling Tools

```cpp
// Macro profiling précis
#define PROFILE_START(name) \
    uint32_t profile_##name##_start = esp_timer_get_time()

#define PROFILE_END(name) \
    uint32_t profile_##name##_end = esp_timer_get_time(); \
    uint32_t profile_##name##_duration = profile_##name##_end - profile_##name##_start; \
    DEBUG_DEBUG("PROFILE " #name ": " + String(profile_##name##_duration) + "μs")

// Profiling avec statistiques
class Profiler {
private:
    struct ProfileData {
        uint32_t totalTime;
        uint32_t callCount;
        uint32_t minTime;
        uint32_t maxTime;
    };
    
    ProfileData profiles[16];
    uint8_t profileCount = 0;
    
public:
    uint8_t addProfile(const char* name);
    void startProfile(uint8_t id);
    void endProfile(uint8_t id);
    void printStatistics();
};

// Usage
Profiler profiler;
uint8_t audioProfileId = profiler.addProfile("Audio Processing");

void audioCallback() {
    profiler.startProfile(audioProfileId);
    // Code audio...
    profiler.endProfile(audioProfileId);
}
```

### 🔍 Memory Analysis

#### Heap Debugging

```cpp
// Memory leak detector
class MemoryWatcher {
private:
    struct AllocationRecord {
        void* ptr;
        size_t size;
        const char* file;
        int line;
        uint32_t timestamp;
    };
    
    AllocationRecord allocations[100];
    uint8_t allocationCount = 0;
    
public:
    void recordAllocation(void* ptr, size_t size, const char* file, int line);
    void recordDeallocation(void* ptr);
    void printLeaks();
    size_t getTotalAllocated();
};

// Macros debug malloc/free
#ifdef MEMORY_DEBUG
    extern MemoryWatcher memWatcher;
    #define DEBUG_MALLOC(size) ({ \
        void* ptr = malloc(size); \
        memWatcher.recordAllocation(ptr, size, __FILE__, __LINE__); \
        ptr; \
    })
    #define DEBUG_FREE(ptr) ({ \
        memWatcher.recordDeallocation(ptr); \
        free(ptr); \
    })
#else
    #define DEBUG_MALLOC(size) malloc(size)
    #define DEBUG_FREE(ptr) free(ptr)
#endif
```

#### Stack Analysis

```cpp
// Stack usage analysis
void analyzeStackUsage() {
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    DEBUG_INFO("Stack high water mark: " + String(highWaterMark * 4) + " bytes");
    
    // Pour toutes les tâches
    TaskStatus_t taskStatus[10];
    UBaseType_t taskCount = uxTaskGetSystemState(taskStatus, 10, NULL);
    
    for (int i = 0; i < taskCount; i++) {
        DEBUG_DEBUG("Task " + String(taskStatus[i].pcTaskName) + 
                   " stack free: " + String(taskStatus[i].usStackHighWaterMark * 4));
    }
}
```

---

## 📚 Best Practices

### 🎯 Code Standards

#### Style Guide

```cpp
// Naming conventions
class AudioProcessor { };           // PascalCase pour classes
void processAudioBuffer();          // camelCase pour fonctions
uint8_t sampleRate = 44100;        // camelCase pour variables
#define MAX_VOICES 16               // UPPER_CASE pour constantes
enum AudioState { };               // PascalCase pour enums

// File organization
/* 
 * DRUM_2025_VSAMPLER - Audio Module
 * Description: 16-voice polyphonic audio engine
 * Author: Developer Name
 * Date: 2025-01-23
 * Dependencies: WM8731, synthESP32
 */

// Includes order
#include <Arduino.h>              // Standard libraries first
#include <WiFi.h>
#include "wm8731.h"              // Project headers
#include "audio_engine.h"

// Constants
const uint8_t MAX_VOICES = 16;
const uint32_t SAMPLE_RATE = 44100;

// Global variables (minimize)
extern AudioProcessor audioProcessor;

// Functions
void setup();
void loop();
```

#### Error Handling

```cpp
// Error codes system
enum ErrorCode {
    ERROR_NONE = 0,
    ERROR_INIT_FAILED,
    ERROR_INVALID_PARAMETER,
    ERROR_OUT_OF_MEMORY,
    ERROR_HARDWARE_FAILURE,
    ERROR_FILE_NOT_FOUND
};

// Error handling pattern
ErrorCode initializeAudioSystem() {
    if (!wm8731_begin()) {
        DEBUG_ERROR("WM8731 initialization failed");
        return ERROR_HARDWARE_FAILURE;
    }
    
    if (!allocateAudioBuffers()) {
        DEBUG_ERROR("Audio buffer allocation failed");
        return ERROR_OUT_OF_MEMORY;
    }
    
    return ERROR_NONE;
}

// Usage with error propagation
ErrorCode result = initializeAudioSystem();
if (result != ERROR_NONE) {
    handleError(result);
    return result;
}
```

### ⚡ Performance Guidelines

#### Memory Management

```cpp
// Préférer allocation statique
int16_t audioBuffer[512];          // Statique - OK
// vs
int16_t* audioBuffer = malloc(1024); // Dynamique - éviter si possible

// Pool d'objets pour allocations fréquentes
template<typename T, size_t N>
class ObjectPool {
private:
    T objects[N];
    bool used[N];
    
public:
    T* acquire() {
        for (size_t i = 0; i < N; i++) {
            if (!used[i]) {
                used[i] = true;
                return &objects[i];
            }
        }
        return nullptr;
    }
    
    void release(T* obj) {
        size_t index = obj - objects;
        if (index < N) {
            used[index] = false;
        }
    }
};

// Usage
ObjectPool<AudioBuffer, 16> audioBufferPool;
```

#### Real-time Guidelines

```cpp
// DO - Code temps réel
void audioProcessingFunction() {
    // Accès direct, pas d'allocation
    for (int i = 0; i < bufferSize; i++) {
        outputBuffer[i] = processAudioSample(inputBuffer[i]);
    }
}

// DON'T - Éviter en temps réel
void badAudioFunction() {
    malloc(1024);                  // Allocation dynamique - NON
    Serial.println("debug");       // I/O - NON
    delay(10);                     // Blocking delay - NON
    complex_math_function();       // Calculs lourds - NON
}

// Communication thread-safe
void safeParameterUpdate(uint8_t voice, uint8_t parameter, uint8_t value) {
    // Atomic pour variables simples
    voiceParameters[voice][parameter] = value;
    
    // Ou utiliser queue pour structures complexes
    ParameterUpdate update = {voice, parameter, value};
    xQueueSend(parameterQueue, &update, 0); // Non-blocking
}
```

---

## 🔗 Extension Examples

### 🎛️ Adding New Effect

```cpp
// 1. Créer nouvelle classe effet
class DelayEffect {
private:
    int16_t* delayBuffer;
    uint32_t delayLength;
    uint32_t writeIndex;
    uint8_t feedback;
    uint8_t mix;
    
public:
    bool begin(uint32_t maxDelayMs) {
        delayLength = (maxDelayMs * SAMPLE_RATE) / 1000;
        delayBuffer = (int16_t*)malloc(delayLength * sizeof(int16_t));
        return (delayBuffer != nullptr);
    }
    
    void setDelay(uint16_t delayMs) {
        uint32_t newLength = (delayMs * SAMPLE_RATE) / 1000;
        if (newLength <= delayLength) {
            // Reset write index pour éviter glitches
            writeIndex = 0;
        }
    }
    
    void setFeedback(uint8_t feedback) {
        this->feedback = feedback;
    }
    
    void setMix(uint8_t mix) {
        this->mix = mix;
    }
    
    int16_t process(int16_t input) {
        // Lecture delayed sample
        int16_t delayed = delayBuffer[writeIndex];
        
        // Écriture nouveau sample avec feedback
        delayBuffer[writeIndex] = input + ((delayed * feedback) / 127);
        
        // Avancer index circulaire
        writeIndex = (writeIndex + 1) % delayLength;
        
        // Mix wet/dry
        return input + ((delayed * mix) / 127);
    }
};

// 2. Intégrer dans synthESP32
DelayEffect voiceDelays[MAX_VOICES];

void synthESP32_begin() {
    // ... init existant ...
    
    for (int i = 0; i < MAX_VOICES; i++) {
        voiceDelays[i].begin(1000); // 1s max delay
    }
}

// 3. Appliquer dans boucle audio
int16_t processVoice(uint8_t voice) {
    // ... traitement sample existant ...
    
    // Appliquer delay
    sample = voiceDelays[voice].process(sample);
    
    return sample;
}

// 4. Ajouter contrôles interface
void handleDelayControls(uint8_t voice, uint8_t zone) {
    switch(zone) {
        case 24: // Delay time
            voiceDelays[voice].setDelay(map(getZoneValue(zone), 0, 127, 0, 1000));
            break;
        case 25: // Feedback
            voiceDelays[voice].setFeedback(getZoneValue(zone));
            break;
        case 26: // Mix
            voiceDelays[voice].setMix(getZoneValue(zone));
            break;
    }
}
```

### 📱 Adding New Application

```cpp
// 1. Ajouter enum
enum AppMode {
    // ... existantes ...
    APP_MY_NEW_APP = 7
};

// 2. Créer draw function
void drawMyNewApp() {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(ZGREEN, BLACK);
    gfx->setCursor(50, 50);
    gfx->print("MY NEW APPLICATION");
    
    // Interface spécifique
    drawBT(0, ZBLUE, "ACTION1");
    drawBT(1, ZRED, "ACTION2");
    drawBT(2, ZYELLOW, "ACTION3");
    
    // Bouton retour universel
    drawBT(23, DARKGREY, " RETOUR ");
}

// 3. Ajouter dans switch
void drawCurrentApplication() {
    switch(currentApplication) {
        // ... cas existants ...
        case APP_MY_NEW_APP:
            drawMyNewApp();
            break;
    }
}

// 4. Gestion événements
void handleMyNewAppTouch(uint8_t zone) {
    switch(zone) {
        case 0:
            // Action 1
            Serial.println("Action 1 triggered");
            break;
        case 1:
            // Action 2
            Serial.println("Action 2 triggered");
            break;
        case 2:
            // Action 3
            Serial.println("Action 3 triggered");
            break;
        case 23:
            returnToMainMenu();
            break;
    }
}

// 5. Intégrer dans keys.ino
void DO_KEYPAD() {
    // ... routing existant ...
    
    if (getCurrentApplication() == APP_MY_NEW_APP) {
        handleMyNewAppTouch(nkey);
        return;
    }
}

// 6. Ajouter bouton menu
MenuButton menuButtons[] = {
    // ... boutons existants ...
    {"MY NEW APP", "N", ZGREEN, APP_MY_NEW_APP, 19, 19}
};
```

---

**🔧 DÉVELOPPEMENT DRUM_2025_VSAMPLER EXTENDED**

Cette référence développeur fournit tous les éléments techniques nécessaires pour comprendre, maintenir et étendre le système DRUM_2025_VSAMPLER Extended. L'architecture modulaire et les APIs bien définies permettent une évolution continue et des contributions communautaires.

---

*DRUM_2025_VSAMPLER Extended - Developer Reference v1.0*  
*Guide technique complet pour développeurs - 23 Août 2025*