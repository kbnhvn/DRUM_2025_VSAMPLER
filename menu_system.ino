////////////////////////////////////////////////////////////////////////////////////////////
//                           SYSTÈME DE MENU PRINCIPAL                                   //
//                              DRUM 2025 VSAMPLER                                       //
////////////////////////////////////////////////////////////////////////////////////////////
// Module de navigation entre applications
// Intégration avec l'architecture UI existante (LCD_tools.ino, keys.ino)
// Gestion des transitions et états des applications

// NOTE: Ce fichier doit être compilé après DRUM_2025_VSAMPLER.ino dans l'IDE Arduino
// toutes les définitions globales seront disponibles automatiquement

// Includes Arduino et ESP32
#include <Arduino.h>
#include <Wire.h>
#include <driver/uart.h>
#include <esp_err.h>

// Forward declarations pour les fonctions externes
extern Arduino_GFX* gfx;
extern bool refreshMODES;
extern bool refreshPATTERN;
extern void drawScreen1_ONLY1();
extern void drawBT(int zone, uint16_t color, String text);
extern bool sdCardPresent;
extern int bpm;
extern int master_vol;
extern int BPOS[48][4];

// Définitions de couleurs (compatibilité)
#define BLACK 0x0000
#define WHITE 0xFFFF
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define OSCURO 0x2104
#define ZGREEN 0x07E0
#define ZRED 0xF800
#define ZYELLOW 0xFFE0
#define ZBLUE 0x001F
#define ZCYAN 0x07FF
#define ZMAGENTA 0xF81F
#define ZGREENCLEAR 0x87E0

// Type byte pour compatibilité Arduino
typedef uint8_t byte;

////////////////////////////////////////////////////////////////////////////////////////////
//                               ÉNUMÉRATIONS ET TYPES                                   //
////////////////////////////////////////////////////////////////////////////////////////////

// Énumération des applications disponibles
enum AppMode {
    APP_MAIN_MENU = 0,
    APP_DRUM_MACHINE,      // Application existante améliorée
    APP_SAMPLING_STUDIO,   // Nouvelle - enregistrement audio
    APP_FILE_BROWSER,      // Nouvelle - navigation samples SD
    APP_MIDI_SETTINGS,     // Nouvelle - configuration MIDI
    APP_WIFI_SETTINGS,     // Nouvelle - WiFi File Manager
    APP_SYSTEM_SETTINGS    // Nouvelle - paramètres système
};

// Structure pour définir les boutons du menu principal
struct MenuButton {
    const char* title;
    const char* icon;
    int color;
    AppMode targetApp;
    byte zoneStart;  // Zone tactile de début
    byte zoneEnd;    // Zone tactile de fin
};

////////////////////////////////////////////////////////////////////////////////////////////
//                               VARIABLES GLOBALES                                      //
////////////////////////////////////////////////////////////////////////////////////////////

// État global du système de navigation
AppMode currentApp = APP_MAIN_MENU;
AppMode previousApp = APP_MAIN_MENU;
bool menuSystemInitialized = false;
bool inTransition = false;
unsigned long transitionStartTime = 0;
const int TRANSITION_DURATION = 300; // ms

// Configuration des boutons du menu principal
const MenuButton menuButtons[6] = {
    {"DRUM MACHINE",    "D", ZGREEN,     APP_DRUM_MACHINE,    0, 1},   // Zones 0-1
    {"SAMPLING STUDIO", "S", ZRED,       APP_SAMPLING_STUDIO, 2, 3},   // Zones 2-3
    {"FILE BROWSER",    "F", ZYELLOW,    APP_FILE_BROWSER,    4, 5},   // Zones 4-5
    {"MIDI SETTINGS",   "M", ZBLUE,      APP_MIDI_SETTINGS,   16, 16}, // Zone 16
    {"WIFI SETTINGS",   "W", ZCYAN,      APP_WIFI_SETTINGS,   17, 17}, // Zone 17
    {"SYSTEM SETTINGS", "Y", ZMAGENTA,   APP_SYSTEM_SETTINGS, 18, 18}  // Zone 18
};

// Variables pour l'état du système
bool wifiConnected = false;
bool midiConnected = true;
bool sdCardPresent = true;
bool needsMenuRedraw = true;
byte selectedMenuButton = 255;
unsigned long lastMenuUpdateTime = 0;

// Configuration Battery & Speaker
#define BATTERY_ADC_PIN 1        // GPIO1 pour monitoring batterie
#define SPEAKER_ENABLE_PIN 10    // GPIO10 pour contrôle speaker JST
bool speakerEnabled = true;      // Speaker activé par défaut
uint8_t batteryLevel = 100;      // Niveau batterie 0-100%
float batteryVoltage = 4.2;      // Voltage batterie
unsigned long lastBatteryCheck = 0;
const unsigned long BATTERY_CHECK_INTERVAL = 5000; // Check toutes les 5s

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS BATTERY & SPEAKER                            //
////////////////////////////////////////////////////////////////////////////////////////////

void initBatteryAndSpeaker() {
    // Initialiser le pin ADC pour batterie
    pinMode(BATTERY_ADC_PIN, INPUT);
    
    // Initialiser le pin de contrôle speaker
    pinMode(SPEAKER_ENABLE_PIN, OUTPUT);
    digitalWrite(SPEAKER_ENABLE_PIN, speakerEnabled ? HIGH : LOW);
    
    Serial.println("Battery monitoring et speaker control initialisés");
}

void updateBatteryLevel() {
    if (millis() - lastBatteryCheck < BATTERY_CHECK_INTERVAL) return;
    
    // Lire le voltage de la batterie via ADC
    uint16_t adcValue = analogRead(BATTERY_ADC_PIN);
    
    // Convertir ADC vers voltage (0-4095 = 0-3.3V, avec diviseur de tension)
    // Supposons diviseur 2:1 pour mesurer jusqu'à 6.6V
    batteryVoltage = (adcValue * 6.6) / 4095.0;
    
    // Convertir voltage vers pourcentage (3.0V = 0%, 4.2V = 100%)
    if (batteryVoltage >= 4.2) {
        batteryLevel = 100;
    } else if (batteryVoltage <= 3.0) {
        batteryLevel = 0;
    } else {
        batteryLevel = (uint8_t)((batteryVoltage - 3.0) / 1.2 * 100);
    }
    
    lastBatteryCheck = millis();
}

void toggleSpeaker() {
    speakerEnabled = !speakerEnabled;
    digitalWrite(SPEAKER_ENABLE_PIN, speakerEnabled ? HIGH : LOW);
    
    Serial.printf("Speaker %s\n", speakerEnabled ? "activé" : "désactivé");
    needsMenuRedraw = true;
}

String getBatteryIcon() {
    if (batteryLevel > 75) return "🔋";
    if (batteryLevel > 50) return "🔋";
    if (batteryLevel > 25) return "🪫";
    return "🪫";
}

uint16_t getBatteryColor() {
    if (batteryLevel > 50) return ZGREEN;
    if (batteryLevel > 25) return ZYELLOW;
    return ZRED;
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS MENU PRINCIPAL                                //
////////////////////////////////////////////////////////////////////////////////////////////

void initMenuSystem() {
    if (menuSystemInitialized) return;
    
    Serial.println("=== INITIALISATION MENU SYSTEM ===");
    
    // Initialiser l'état du menu
    currentApp = APP_MAIN_MENU;
    previousApp = APP_MAIN_MENU;
    needsMenuRedraw = true;
    inTransition = false;
    selectedMenuButton = 255;
    
    // Marquer comme initialisé
    menuSystemInitialized = true;
    
    Serial.println("Menu System initialise avec succes");
    Serial.println("Applications disponibles :");
    for (int i = 0; i < 6; i++) {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(menuButtons[i].title);
    }
    Serial.println("============================");
}

void updateMenuSystem() {
    if (!menuSystemInitialized) return;
    
    unsigned long currentTime = millis();
    
    // Gestion des transitions
    if (inTransition) {
        if (currentTime - transitionStartTime >= TRANSITION_DURATION) {
            inTransition = false;
            needsMenuRedraw = true;
        }
        return;
    }
    
    // Mise à jour périodique du menu principal
    if (currentApp == APP_MAIN_MENU) {
        if (currentTime - lastMenuUpdateTime >= 1000) { // Mise à jour chaque seconde
            lastMenuUpdateTime = currentTime;
            drawMenuStatusBar(); // Mettre à jour les indicateurs de statut
        }
    }
    
    // Redessiner si nécessaire
    if (needsMenuRedraw && currentApp == APP_MAIN_MENU) {
        drawMainMenu();
        needsMenuRedraw = false;
    }
}

void drawMainMenu() {
    if (!menuSystemInitialized || currentApp != APP_MAIN_MENU) return;
    
    // Effacer l'écran
    gfx->fillScreen(BLACK);
    
    // Dessiner l'en-tête
    drawMenuHeader();
    
    // Dessiner les boutons du menu principal
    for (byte i = 0; i < 6; i++) {
        drawMenuButtonAt(i, i == selectedMenuButton);
    }
    
    // Dessiner la barre de statut
    drawMenuStatusBar();
}

void drawMenuHeader() {
    // Titre principal
    gfx->setTextColor(ZGREENCLEAR, BLACK);
    gfx->setCursor(80, 15);
    gfx->print("DRUM 2025 VSAMPLER");
    
    gfx->setTextColor(DARKGREY, BLACK);
    gfx->setCursor(180, 30);
    gfx->print("v2.0");
    
    // Ligne de séparation
    gfx->drawLine(0, 45, 479, 45, DARKGREY);
}

void drawMenuButtonAt(byte buttonIndex, bool selected) {
    if (buttonIndex >= 6) return;
    
    const MenuButton& btn = menuButtons[buttonIndex];
    int color = selected ? WHITE : btn.color;
    
    // Calculer les positions selon le layout
    int x, y, w, h;
    
    if (buttonIndex < 3) {
        // Boutons principaux (lignes 1-3)
        x = 20;
        y = 60 + (buttonIndex * 55);
        w = 440;
        h = 45;
        
        // Dessiner le bouton manuellement
        gfx->drawRect(x, y, w, h, color);
        gfx->drawRect(x + 1, y + 1, w - 2, h - 2, color);
        
        if (selected) {
            gfx->fillRect(x + 2, y + 2, w - 4, h - 4, OSCURO);
        }
        
        gfx->setTextColor(color, BLACK);
        gfx->setCursor(x + 10, y + 20);
        gfx->print(btn.icon);
        gfx->print(" ");
        gfx->print(btn.title);
        
    } else {
        // Boutons secondaires : utiliser les zones tactiles existantes
        if (btn.zoneStart < 48) {
            String buttonText = "";
            buttonText += btn.icon;
            buttonText += " ";
            buttonText += btn.title;
            drawBT(btn.zoneStart, selected ? WHITE : color, buttonText);
        }
    }
}

void drawMenuStatusBar() {
    // Position de la barre de statut
    int statusY = 280;
    
    // Ligne de séparation
    gfx->drawLine(0, statusY - 5, 479, statusY - 5, DARKGREY);
    
    // Effacer la zone de statut
    gfx->fillRect(0, statusY, 480, 40, BLACK);
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, statusY + 15);
    gfx->print("Status: ");
    
    // Indicateur carte SD
    gfx->setTextColor(sdCardPresent ? ZGREEN : ZRED, BLACK);
    gfx->print("SD ");
    if (sdCardPresent) {
        gfx->print("OK");
    } else {
        gfx->print("--");
    }
    
    // Indicateur MIDI
    gfx->setTextColor(midiConnected ? ZGREEN : DARKGREY, BLACK);
    gfx->print(" MIDI ");
    if (midiConnected) {
        gfx->print("OK");
    } else {
        gfx->print("--");
    }
    
    // Indicateur WiFi
    gfx->setTextColor(wifiConnected ? ZGREEN : DARKGREY, BLACK);
    gfx->print(" WiFi ");
    if (wifiConnected) {
        gfx->print("OK");
    } else {
        gfx->print("--");
    }
    
    // Affichage du niveau de batterie avec icône et couleur adaptive
    gfx->setCursor(300, statusY + 15);
    gfx->setTextColor(getBatteryColor(), BLACK);
    gfx->print(getBatteryIcon());
    gfx->print(" ");
    gfx->print(batteryLevel);
    gfx->print("%");
    
    // Affichage de l'état speaker/headphone
    gfx->setCursor(350, statusY + 15);
    gfx->setTextColor(speakerEnabled ? ZGREEN : LIGHTGREY, BLACK);
    gfx->print(speakerEnabled ? "SPK" : "HP");
    
    // Informations système (BPM et volume)
    gfx->setTextColor(DARKGREY, BLACK);
    gfx->setCursor(390, statusY + 15);
    gfx->print("BPM:");
    gfx->print(bpm);
    gfx->print(" Vol:");
    gfx->print(master_vol);
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               GESTION DES TOUCHES                                     //
////////////////////////////////////////////////////////////////////////////////////////////

void handleMenuTouch(int touchX, int touchY) {
    if (!menuSystemInitialized || currentApp != APP_MAIN_MENU || inTransition) return;
    
    Serial.print("Menu touch: x=");
    Serial.print(touchX);
    Serial.print(", y=");
    Serial.println(touchY);
    
    // Détecter quel bouton a été touché
    for (byte i = 0; i < 6; i++) {
        const MenuButton& btn = menuButtons[i];
        bool touched = false;
        
        if (i < 3) {
            // Boutons principaux - zones manuelles
            int btnX = 20;
            int btnY = 60 + (i * 55);
            int btnW = 440;
            int btnH = 45;
            
            if (touchX >= btnX && touchX <= (btnX + btnW) && touchY >= btnY && touchY <= (btnY + btnH)) {
                touched = true;
            }
        } else {
            // Boutons secondaires - zones tactiles existantes
            if (btn.zoneStart < 48) {
                int zoneX = BPOS[btn.zoneStart][0];
                int zoneY = BPOS[btn.zoneStart][1];
                int zoneW = BPOS[btn.zoneStart][2];
                int zoneH = BPOS[btn.zoneStart][3];
                
                if (touchX >= zoneX && touchX <= (zoneX + zoneW) && touchY >= zoneY && touchY <= (zoneY + zoneH)) {
                    touched = true;
                }
            }
        }
        
        if (touched) {
            Serial.print("Bouton touche: ");
            Serial.println(btn.title);
            selectedMenuButton = i;
            
            // Feedback visuel immédiat
            drawMenuButtonAt(i, true);
            
            // Délai pour feedback tactile
            delay(150);
            
            // Lancer l'application
            switchToApplication(btn.targetApp);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               NAVIGATION APPLICATIONS                                 //
////////////////////////////////////////////////////////////////////////////////////////////

void switchToApplication(AppMode app) {
    if (currentApp == app || inTransition) return;
    
    Serial.print("Transition vers application: ");
    Serial.println(app);
    
    // Sauvegarder l'état actuel
    saveCurrentAppState();
    
    // Marquer le début de la transition
    previousApp = currentApp;
    inTransition = true;
    transitionStartTime = millis();
    
    // Animation de transition simple
    animateTransition(currentApp, app);
    
    // Changer d'application
    currentApp = app;
    
    // Restaurer l'état de la nouvelle application
    restoreAppState(app);
    
    // Notifier le changement
    notifyAppChange();
}

void returnToMainMenu() {
    if (currentApp == APP_MAIN_MENU) return;
    
    Serial.println("Retour au menu principal");
    
    // Sauvegarder l'état de l'application actuelle
    saveCurrentAppState();
    
    // Transition vers le menu
    switchToApplication(APP_MAIN_MENU);
}

void animateTransition(AppMode fromApp, AppMode toApp) {
    // Animation simple de fondu
    for (int alpha = 255; alpha >= 0; alpha -= 15) {
        // Ici on pourrait implémenter un vrai fondu
        // Pour l'instant, juste un délai
        delay(20);
    }
}

void saveCurrentAppState() {
    // Sauvegarder les variables importantes selon l'application actuelle
    switch (currentApp) {
        case APP_DRUM_MACHINE:
            // Sauvegarder l'état de la drum machine (patterns, sounds, etc.)
            break;
        case APP_SAMPLING_STUDIO:
            // Sauvegarder l'état du studio d'enregistrement
            break;
        case APP_FILE_BROWSER:
            // Sauvegarder la position dans le navigateur
            break;
        default:
            break;
    }
}

void restoreAppState(AppMode app) {
    // Restaurer l'état de l'application
    switch (app) {
        case APP_MAIN_MENU:
            needsMenuRedraw = true;
            selectedMenuButton = 255;
            break;
        case APP_DRUM_MACHINE:
            // Restaurer l'interface de la drum machine
            refreshMODES = true;
            refreshPATTERN = true;
            drawScreen1_ONLY1();
            break;
        default:
            break;
    }
}

void notifyAppChange() {
    // Notifier les autres modules du changement d'application
    Serial.print("Application active: ");
    Serial.println(currentApp);
    
    // Réinitialiser les flags d'affichage
    switch (currentApp) {
        case APP_DRUM_MACHINE:
            refreshMODES = true;
            refreshPATTERN = true;
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS UTILITAIRES                                   //
////////////////////////////////////////////////////////////////////////////////////////////

// Fonction pour obtenir l'application actuelle
AppMode getCurrentApplication() {
    return currentApp;
}

// Fonction pour vérifier si on est dans le menu principal
bool isInMainMenu() {
    return currentApp == APP_MAIN_MENU;
}

// Fonction pour forcer le redessin du menu
void forceMenuRedraw() {
    needsMenuRedraw = true;
}

// Fonction pour mettre à jour les statuts des connexions
void updateSystemStatus(bool sdPresent, bool midiStatus, bool wifiStatus) {
    sdCardPresent = sdPresent;
    midiConnected = midiStatus;
    wifiConnected = wifiStatus;
    
    if (isInMainMenu()) {
        forceMenuRedraw();
    }
}

// Fonction pour gérer le bouton retour universel
void handleBackButton() {
    if (!isInMainMenu()) {
        returnToMainMenu();
    }
}

// Fonctions de navigation directe
void navigateToDrumMachine() {
    switchToApplication(APP_DRUM_MACHINE);
}

void navigateToSamplingStudio() {
    switchToApplication(APP_SAMPLING_STUDIO);
}

void navigateToFileBrowser() {
    switchToApplication(APP_FILE_BROWSER);
}

void navigateToMidiSettings() {
    switchToApplication(APP_MIDI_SETTINGS);
}

void navigateToWifiSettings() {
    switchToApplication(APP_WIFI_SETTINGS);
}

void navigateToSystemSettings() {
    switchToApplication(APP_SYSTEM_SETTINGS);
}

////////////////////////////////////////////////////////////////////////////////////////////
//                                   APPLICATIONS STUB                                   //
////////////////////////////////////////////////////////////////////////////////////////////

// Fonctions stub pour les nouvelles applications
// À implémenter dans des modules séparés

////////////////////////////////////////////////////////////////////////////////////////////
//                           SAMPLING STUDIO APPLICATION                                 //
////////////////////////////////////////////////////////////////////////////////////////////

// Configuration Sampling Studio
#define SAMPLING_BUFFER_DURATION    10      // Durée max en secondes
#define SAMPLING_MAX_SAMPLES        (44100 * SAMPLING_BUFFER_DURATION)
#define SAMPLING_BUFFER_SIZE        (SAMPLING_MAX_SAMPLES * 2) // bytes
#define WAVEFORM_STUDIO_WIDTH      380      // Largeur zone waveform
#define WAVEFORM_STUDIO_HEIGHT     80       // Hauteur zone waveform
#define WAVEFORM_STUDIO_X          50       // Position X
#define WAVEFORM_STUDIO_Y          70       // Position Y
#define KEYBOARD_MAX_NAME_LENGTH   31       // Longueur max nom sample

// États Sampling Studio
enum SamplingState {
    SAMPLING_IDLE = 0,
    SAMPLING_RECORDING,
    SAMPLING_RECORDED,
    SAMPLING_PLAYING,
    SAMPLING_NAMING,
    SAMPLING_SAVING
};

// Variables globales Sampling Studio
SamplingState samplingState = SAMPLING_IDLE;
bool samplingInitialized = false;
bool samplingNeedsRedraw = true;
int16_t* samplingRecordBuffer = nullptr;
uint32_t samplingRecordedSamples = 0;
uint32_t samplingMaxSamples = SAMPLING_MAX_SAMPLES;
uint32_t samplingRecordStartTime = 0;
uint32_t samplingRecordDuration = 0;
uint8_t samplingInputGain = 15;
uint8_t samplingOutputVolume = 100;
bool samplingStereoMode = true;
uint8_t samplingInputSource = 0;
char samplingSampleName[KEYBOARD_MAX_NAME_LENGTH + 1] = "NewSample";
bool samplingKeyboardVisible = false;
int16_t* samplingWaveformBuffer = nullptr;
uint32_t samplingWaveformSamples = 0;
int16_t samplingWaveformMin = 0, samplingWaveformMax = 0;
bool samplingWaveformNeedsUpdate = false;
uint8_t samplingPlaybackSlot = 15;
int16_t samplingCurrentInputLevel = 0;
int16_t samplingPeakInputLevel = 0;
unsigned long samplingLastLevelUpdate = 0;
unsigned long samplingLastWaveformUpdate = 0;

// Fonctions Sampling Studio
bool samplingStudioBegin();
void samplingStudioUpdate();
void samplingStudioHandleTouch(uint16_t zone);
void samplingStartRecording();
void samplingStopRecording();
void samplingUpdateRecording();
void samplingStartPlayback();
void samplingStopPlayback();
void samplingDrawHeader();
void samplingDrawAudioControls();
void samplingDrawWaveformArea();
void samplingDrawControlButtons();
void samplingDrawNamingArea();
void samplingDrawActionButtons();
void samplingDrawStatusBar();
void samplingDrawRealtimeWaveform();
void samplingDrawCompleteWaveform();
void samplingHandleKeyboardTouch(uint16_t zone);
void samplingDrawKeyboard();
bool samplingSaveBufferAsWAV(const char* filename);
void samplingGenerateWaveformFromBuffer();

bool samplingStudioBegin() {
    Serial.println("=== INITIALISATION SAMPLING STUDIO ===");
    
    if (!wm8731_isReady()) {
        Serial.println("ERREUR: WM8731 non disponible");
        return false;
    }
    
    samplingRecordBuffer = (int16_t*)malloc(SAMPLING_BUFFER_SIZE);
    if (!samplingRecordBuffer) {
        Serial.println("ERREUR: Impossible d'allouer le buffer d'enregistrement");
        return false;
    }
    
    samplingWaveformBuffer = (int16_t*)malloc(WAVEFORM_STUDIO_WIDTH * sizeof(int16_t));
    if (!samplingWaveformBuffer) {
        Serial.println("ERREUR: Impossible d'allouer le buffer waveform");
        free(samplingRecordBuffer);
        samplingRecordBuffer = nullptr;
        return false;
    }
    
    wm8731_setInputGain(samplingInputGain);
    wm8731_setOutputVolume(samplingOutputVolume);
    wm8731_setInputSource(samplingInputSource);
    wm8731_enableInput(true);
    wm8731_enableOutput(true);
    
    samplingState = SAMPLING_IDLE;
    samplingRecordedSamples = 0;
    samplingKeyboardVisible = false;
    samplingNeedsRedraw = true;
    samplingInitialized = true;
    
    Serial.println("Sampling Studio initialisé avec succès");
    return true;
}

void samplingStudioUpdate() {
    if (!samplingInitialized) return;
    
    unsigned long currentTime = millis();
    
    switch (samplingState) {
        case SAMPLING_RECORDING:
            samplingUpdateRecording();
            break;
        case SAMPLING_PLAYING:
            if (!latch[samplingPlaybackSlot]) {
                samplingStopPlayback();
            }
            break;
        default:
            break;
    }
    
    // Mise à jour du monitoring des niveaux
    if (currentTime - samplingLastLevelUpdate >= 50) {
        if (wm8731_isReady()) {
            uint8_t monitorBuffer[32];
            size_t bytesRead = wm8731_readAudio(monitorBuffer, 16);
            
            if (bytesRead > 0) {
                int16_t maxLevel = 0;
                for (size_t i = 0; i < bytesRead; i += 2) {
                    int16_t sample = *((int16_t*)(monitorBuffer + i));
                    if (abs(sample) > maxLevel) {
                        maxLevel = abs(sample);
                    }
                }
                
                samplingCurrentInputLevel = maxLevel;
                
                if (maxLevel > samplingPeakInputLevel) {
                    samplingPeakInputLevel = maxLevel;
                } else if (samplingPeakInputLevel > 0) {
                    samplingPeakInputLevel = (samplingPeakInputLevel * 95) / 100;
                }
            }
        }
        samplingLastLevelUpdate = currentTime;
    }
    
    // Mise à jour du waveform
    if (currentTime - samplingLastWaveformUpdate >= 50) {
        if (samplingWaveformNeedsUpdate || samplingState == SAMPLING_RECORDING) {
            if (samplingState == SAMPLING_RECORDING) {
                samplingWaveformNeedsUpdate = true;
            } else if (samplingWaveformNeedsUpdate) {
                samplingGenerateWaveformFromBuffer();
                samplingWaveformNeedsUpdate = false;
            }
            samplingLastWaveformUpdate = currentTime;
        }
    }
}

void samplingStudioHandleTouch(uint16_t zone) {
    if (!samplingInitialized) return;
    
    Serial.printf("Sampling Studio - Touch zone: %d\n", zone);
    
    if (samplingKeyboardVisible && samplingHandleKeyboardTouch(zone)) {
        return;
    }
    
    switch (zone) {
        case 16: // REC
            if (samplingState == SAMPLING_IDLE || samplingState == SAMPLING_RECORDED) {
                samplingStartRecording();
            }
            break;
        case 17: // STOP
            if (samplingState == SAMPLING_RECORDING) {
                samplingStopRecording();
            } else if (samplingState == SAMPLING_PLAYING) {
                samplingStopPlayback();
            }
            break;
        case 18: // PLAY
            if (samplingState == SAMPLING_RECORDED) {
                samplingStartPlayback();
            } else if (samplingState == SAMPLING_PLAYING) {
                samplingStopPlayback();
            }
            break;
        case 19: // KEYBOARD
            if (samplingState == SAMPLING_RECORDED || samplingState == SAMPLING_IDLE) {
                samplingKeyboardVisible = !samplingKeyboardVisible;
                if (samplingKeyboardVisible) {
                    samplingState = SAMPLING_NAMING;
                } else {
                    samplingState = (samplingRecordedSamples > 0) ? SAMPLING_RECORDED : SAMPLING_IDLE;
                }
            }
            break;
        case 20: // Toggle stereo/mono
            samplingStereoMode = !samplingStereoMode;
            Serial.printf("Mode audio: %s\n", samplingStereoMode ? "Stéréo" : "Mono");
            break;
        case 21: // SAVE
            if (samplingState == SAMPLING_RECORDED && sdIsReady()) {
                char filename[64];
                snprintf(filename, sizeof(filename), "/SAMPLES/USER/%s.wav", samplingSampleName);
                if (samplingSaveBufferAsWAV(filename)) {
                    Serial.printf("Sample sauvegardé: %s\n", filename);
                }
            }
            break;
        case 22: // CANCEL
            switch (samplingState) {
                case SAMPLING_RECORDED:
                case SAMPLING_PLAYING:
                    samplingRecordedSamples = 0;
                    samplingState = SAMPLING_IDLE;
                    strcpy(samplingSampleName, "NewSample");
                    break;
                case SAMPLING_NAMING:
                    samplingKeyboardVisible = false;
                    samplingState = SAMPLING_RECORDED;
                    break;
                default:
                    break;
            }
            break;
        case 23: // RETOUR
            if (samplingState == SAMPLING_RECORDING) {
                samplingStopRecording();
            }
            if (samplingState == SAMPLING_PLAYING) {
                samplingStopPlayback();
            }
            
            if (samplingRecordBuffer) {
                free(samplingRecordBuffer);
                samplingRecordBuffer = nullptr;
            }
            if (samplingWaveformBuffer) {
                free(samplingWaveformBuffer);
                samplingWaveformBuffer = nullptr;
            }
            
            returnToMainMenu();
            break;
        case 25: // Gain -
            if (samplingInputGain > 0) {
                samplingInputGain -= 2;
                wm8731_setInputGain(samplingInputGain);
                Serial.printf("Gain input: %d\n", samplingInputGain);
            }
            break;
        case 28: // Gain +
            if (samplingInputGain < 31) {
                samplingInputGain += 2;
                wm8731_setInputGain(samplingInputGain);
                Serial.printf("Gain input: %d\n", samplingInputGain);
            }
            break;
    }
    
    samplingNeedsRedraw = true;
}

void samplingStartRecording() {
    Serial.println("Démarrage enregistrement...");
    
    memset(samplingRecordBuffer, 0, SAMPLING_BUFFER_SIZE);
    samplingRecordedSamples = 0;
    samplingRecordStartTime = millis();
    
    wm8731_enableInput(true);
    
    samplingState = SAMPLING_RECORDING;
    samplingWaveformNeedsUpdate = true;
    samplingNeedsRedraw = true;
    
    Serial.println("Enregistrement démarré");
}

void samplingStopRecording() {
    Serial.printf("Arrêt enregistrement - %d échantillons capturés\n", samplingRecordedSamples);
    
    samplingRecordDuration = millis() - samplingRecordStartTime;
    samplingState = SAMPLING_RECORDED;
    samplingWaveformNeedsUpdate = true;
    samplingNeedsRedraw = true;
    
    samplingGenerateWaveformFromBuffer();
}

void samplingUpdateRecording() {
    if (samplingState != SAMPLING_RECORDING) return;
    
    uint8_t audioBuffer[512];
    size_t bytesRead = wm8731_readAudio(audioBuffer, 256);
    
    if (bytesRead > 0) {
        uint32_t samplesAvailable = bytesRead / (samplingStereoMode ? 4 : 2);
        
        for (uint32_t i = 0; i < samplesAvailable && samplingRecordedSamples < samplingMaxSamples; i++) {
            int16_t sample;
            
            if (samplingStereoMode) {
                int16_t left = *((int16_t*)(audioBuffer + (i * 4)));
                int16_t right = *((int16_t*)(audioBuffer + (i * 4) + 2));
                sample = (left + right) >> 1;
            } else {
                sample = *((int16_t*)(audioBuffer + (i * 2)));
            }
            
            samplingRecordBuffer[samplingRecordedSamples] = sample;
            samplingRecordedSamples++;
        }
    }
    
    if (samplingRecordedSamples >= samplingMaxSamples ||
        (millis() - samplingRecordStartTime) >= (SAMPLING_BUFFER_DURATION * 1000)) {
        samplingStopRecording();
    }
}

void samplingStartPlayback() {
    Serial.println("Démarrage playback...");
    
    char tempPath[] = "/SAMPLES/USER/temp_preview.wav";
    if (samplingSaveBufferAsWAV(tempPath)) {
        if (sdLoadSample(samplingPlaybackSlot, tempPath)) {
            synthESP32_TRIGGER(samplingPlaybackSlot);
            samplingState = SAMPLING_PLAYING;
            samplingNeedsRedraw = true;
            Serial.println("Playback démarré");
        }
    }
}

void samplingStopPlayback() {
    Serial.println("Arrêt playback");
    latch[samplingPlaybackSlot] = 0;
    samplingState = SAMPLING_RECORDED;
    samplingNeedsRedraw = true;
}

bool samplingHandleKeyboardTouch(uint16_t zone) {
    if (zone >= 0 && zone <= 15) {
        const char* keyLayout = "QWERTYUIOPASDFGH";
        if (zone < 16 && strlen(samplingSampleName) < KEYBOARD_MAX_NAME_LENGTH) {
            char newChar = keyLayout[zone];
            strncat(samplingSampleName, &newChar, 1);
            samplingNeedsRedraw = true;
            return true;
        }
    } else if (zone == 25) { // Backspace
        int len = strlen(samplingSampleName);
        if (len > 0) {
            samplingSampleName[len - 1] = '\0';
            samplingNeedsRedraw = true;
        }
        return true;
    } else if (zone == 28) { // Enter/Valider
        samplingKeyboardVisible = false;
        samplingState = SAMPLING_RECORDED;
        samplingNeedsRedraw = true;
        return true;
    }
    
    return false;
}

bool samplingSaveBufferAsWAV(const char* filename) {
    if (!sdIsReady() || !samplingRecordBuffer || samplingRecordedSamples == 0) return false;
    
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.printf("Impossible de créer le fichier: %s\n", filename);
        return false;
    }
    
    uint32_t dataSize = samplingRecordedSamples * 2;
    uint32_t fileSize = 44 + dataSize;
    uint16_t channels = samplingStereoMode ? 2 : 1;
    
    // En-tête WAV
    file.write((uint8_t*)"RIFF", 4);
    file.write((uint8_t*)&fileSize, 4);
    file.write((uint8_t*)"WAVE", 4);
    
    // Chunk fmt
    file.write((uint8_t*)"fmt ", 4);
    uint32_t fmtSize = 16;
    file.write((uint8_t*)&fmtSize, 4);
    uint16_t audioFormat = 1;
    file.write((uint8_t*)&audioFormat, 2);
    file.write((uint8_t*)&channels, 2);
    uint32_t sampleRate = 44100;
    file.write((uint8_t*)&sampleRate, 4);
    uint32_t byteRate = sampleRate * channels * 2;
    file.write((uint8_t*)&byteRate, 4);
    uint16_t blockAlign = channels * 2;
    file.write((uint8_t*)&blockAlign, 2);
    uint16_t bitsPerSample = 16;
    file.write((uint8_t*)&bitsPerSample, 2);
    
    // Chunk data
    file.write((uint8_t*)"data", 4);
    file.write((uint8_t*)&dataSize, 4);
    
    // Données audio
    file.write((uint8_t*)samplingRecordBuffer, dataSize);
    
    file.close();
    return true;
}

void samplingGenerateWaveformFromBuffer() {
    if (!samplingRecordBuffer || !samplingWaveformBuffer || samplingRecordedSamples == 0) return;
    
    uint32_t step = samplingRecordedSamples / WAVEFORM_STUDIO_WIDTH;
    if (step == 0) step = 1;
    
    samplingWaveformMin = 32767;
    samplingWaveformMax = -32768;
    
    for (int x = 0; x < WAVEFORM_STUDIO_WIDTH && x < samplingRecordedSamples; x++) {
        uint32_t sampleIndex = x * step;
        if (sampleIndex < samplingRecordedSamples) {
            samplingWaveformBuffer[x] = samplingRecordBuffer[sampleIndex];
            
            if (samplingWaveformBuffer[x] < samplingWaveformMin) samplingWaveformMin = samplingWaveformBuffer[x];
            if (samplingWaveformBuffer[x] > samplingWaveformMax) samplingWaveformMax = samplingWaveformBuffer[x];
        }
    }
    
    samplingWaveformSamples = min(WAVEFORM_STUDIO_WIDTH, samplingRecordedSamples);
    
    if (samplingWaveformMax - samplingWaveformMin < 1000) {
        int center = (samplingWaveformMax + samplingWaveformMin) / 2;
        samplingWaveformMin = center - 500;
        samplingWaveformMax = center + 500;
    }
}

void samplingDrawHeader() {
    gfx->setTextColor(ZRED, BLACK);
    gfx->setCursor(120, 15);
    gfx->print("SAMPLING STUDIO");
    
    int stateColor = DARKGREY;
    String stateText = "IDLE";
    
    switch (samplingState) {
        case SAMPLING_RECORDING:
            stateColor = ZRED;
            stateText = "● REC";
            break;
        case SAMPLING_RECORDED:
            stateColor = ZGREEN;
            stateText = "READY";
            break;
        case SAMPLING_PLAYING:
            stateColor = ZBLUE;
            stateText = "▶ PLAY";
            break;
        case SAMPLING_NAMING:
            stateColor = ZYELLOW;
            stateText = "NAMING";
            break;
        case SAMPLING_SAVING:
            stateColor = ZCYAN;
            stateText = "SAVING";
            break;
        default:
            break;
    }
    
    gfx->setTextColor(stateColor, BLACK);
    gfx->setCursor(350, 15);
    gfx->print(stateText);
}

void samplingDrawAudioControls() {
    int yPos = 35;
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, yPos);
    gfx->print("Input:");
    
    gfx->setCursor(80, yPos);
    gfx->print(samplingInputSource == 0 ? "Line" : "Mic");
    
    gfx->setCursor(150, yPos);
    gfx->print("Gain:");
    
    int gainBarWidth = 100;
    int gainLevel = map(samplingInputGain, 0, 31, 0, gainBarWidth);
    gfx->drawRect(200, yPos - 3, gainBarWidth + 2, 12, DARKGREY);
    gfx->fillRect(201, yPos - 2, gainLevel, 10, ZGREEN);
    
    gfx->setCursor(320, yPos);
    gfx->printf("%d", samplingInputGain);
    
    gfx->setCursor(380, yPos);
    gfx->setTextColor(samplingStereoMode ? ZGREEN : DARKGREY, BLACK);
    gfx->print(samplingStereoMode ? "[STEREO]" : "[MONO]");
    
    // VU-mètre
    if (samplingState == SAMPLING_RECORDING || samplingState == SAMPLING_IDLE) {
        int meterX = 400;
        int meterY = 50;
        int meterWidth = 60;
        int meterHeight = 8;
        
        gfx->drawRect(meterX, meterY, meterWidth + 2, meterHeight + 2, DARKGREY);
        gfx->fillRect(meterX + 1, meterY + 1, meterWidth, meterHeight, BLACK);
        
        if (samplingCurrentInputLevel > 0) {
            int levelWidth = map(abs(samplingCurrentInputLevel), 0, 32767, 0, meterWidth);
            int levelColor = ZGREEN;
            
            if (levelWidth > meterWidth * 0.8) {
                levelColor = ZRED;
            } else if (levelWidth > meterWidth * 0.6) {
                levelColor = ZYELLOW;
            }
            
            gfx->fillRect(meterX + 1, meterY + 1, levelWidth, meterHeight, levelColor);
        }
        
        if (samplingPeakInputLevel > 0) {
            int peakPos = map(abs(samplingPeakInputLevel), 0, 32767, 0, meterWidth);
            gfx->drawLine(meterX + peakPos, meterY, meterX + peakPos, meterY + meterHeight + 1, WHITE);
        }
    }
}

void samplingDrawWaveformArea() {
    gfx->drawRect(WAVEFORM_STUDIO_X - 2, WAVEFORM_STUDIO_Y - 2,
                  WAVEFORM_STUDIO_WIDTH + 4, WAVEFORM_STUDIO_HEIGHT + 4, DARKGREY);
    gfx->fillRect(WAVEFORM_STUDIO_X, WAVEFORM_STUDIO_Y, WAVEFORM_STUDIO_WIDTH, WAVEFORM_STUDIO_HEIGHT, BLACK);
    
    int centerY = WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT / 2;
    gfx->drawLine(WAVEFORM_STUDIO_X, centerY, WAVEFORM_STUDIO_X + WAVEFORM_STUDIO_WIDTH - 1, centerY, OSCURO);
    
    if (samplingState == SAMPLING_RECORDING) {
        samplingDrawRealtimeWaveform();
    } else if (samplingState == SAMPLING_RECORDED || samplingState == SAMPLING_PLAYING) {
        samplingDrawCompleteWaveform();
    }
}

void samplingDrawRealtimeWaveform() {
    if (!samplingRecordBuffer || samplingRecordedSamples < 2) return;
    
    uint32_t startSample = samplingRecordedSamples > WAVEFORM_STUDIO_WIDTH ?
                          samplingRecordedSamples - WAVEFORM_STUDIO_WIDTH : 0;
    
    for (int x = 0; x < WAVEFORM_STUDIO_WIDTH - 1 && (startSample + x) < samplingRecordedSamples; x++) {
        int16_t sample1 = samplingRecordBuffer[startSample + x];
        int16_t sample2 = samplingRecordBuffer[startSample + x + 1];
        
        int y1 = map(sample1, -32768, 32767, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        int y2 = map(sample2, -32768, 32767, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        
        gfx->drawLine(WAVEFORM_STUDIO_X + x, WAVEFORM_STUDIO_Y + y1,
                     WAVEFORM_STUDIO_X + x + 1, WAVEFORM_STUDIO_Y + y2, ZGREEN);
    }
    
    if (samplingRecordedSamples > 0) {
        int progressWidth = map(samplingRecordedSamples, 0, samplingMaxSamples, 0, WAVEFORM_STUDIO_WIDTH);
        gfx->drawLine(WAVEFORM_STUDIO_X, WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT + 5,
                     WAVEFORM_STUDIO_X + progressWidth, WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT + 5, ZRED);
    }
}

void samplingDrawCompleteWaveform() {
    if (!samplingWaveformBuffer || samplingWaveformSamples == 0) return;
    
    for (int x = 0; x < WAVEFORM_STUDIO_WIDTH - 1; x++) {
        if (x >= samplingWaveformSamples) break;
        
        int16_t sample1 = samplingWaveformBuffer[x];
        int16_t sample2 = (x + 1 < samplingWaveformSamples) ? samplingWaveformBuffer[x + 1] : sample1;
        
        int y1 = map(sample1, samplingWaveformMin, samplingWaveformMax, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        int y2 = map(sample2, samplingWaveformMin, samplingWaveformMax, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        
        int waveColor = ZCYAN;
        if (samplingState == SAMPLING_PLAYING) {
            waveColor = ZGREEN;
        }
        
        gfx->drawLine(WAVEFORM_STUDIO_X + x, WAVEFORM_STUDIO_Y + y1,
                     WAVEFORM_STUDIO_X + x + 1, WAVEFORM_STUDIO_Y + y2, waveColor);
    }
}

void samplingDrawControlButtons() {
    int recColor = (samplingState == SAMPLING_RECORDING) ? ZRED : DARKGREY;
    if (samplingState == SAMPLING_IDLE || samplingState == SAMPLING_RECORDED) {
        recColor = ZGREEN;
    }
    drawBT(16, recColor, "●REC");
    
    int stopColor = (samplingState == SAMPLING_RECORDING || samplingState == SAMPLING_PLAYING) ? ZRED : DARKGREY;
    drawBT(17, stopColor, "■STOP");
    
    int playColor = DARKGREY;
    if (samplingState == SAMPLING_RECORDED) {
        playColor = ZGREEN;
    } else if (samplingState == SAMPLING_PLAYING) {
        playColor = ZBLUE;
    }
    drawBT(18, playColor, "▶PLAY");
    
    // Timer
    int timerX = 350;
    int timerY = WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT + 30;
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(timerX, timerY);
    
    uint32_t currentDuration = 0;
    if (samplingState == SAMPLING_RECORDING) {
        currentDuration = millis() - samplingRecordStartTime;
    } else if (samplingState == SAMPLING_RECORDED || samplingState == SAMPLING_PLAYING) {
        currentDuration = samplingRecordDuration;
    }
    
    uint32_t seconds = currentDuration / 1000;
    uint32_t milliseconds = (currentDuration % 1000) / 10;
    
    gfx->printf("%02lu.%02lus", seconds, milliseconds);
}

void samplingDrawNamingArea() {
    int namingY = WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT + 70;
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, namingY);
    gfx->print("Name:");
    
    int nameBoxX = 80;
    int nameBoxWidth = 200;
    int nameBoxHeight = 20;
    
    gfx->drawRect(nameBoxX, namingY - 5, nameBoxWidth, nameBoxHeight, DARKGREY);
    gfx->fillRect(nameBoxX + 1, namingY - 4, nameBoxWidth - 2, nameBoxHeight - 2, BLACK);
    
    gfx->setTextColor(ZGREEN, BLACK);
    gfx->setCursor(nameBoxX + 5, namingY);
    gfx->print(samplingSampleName);
    
    if (samplingKeyboardVisible && (millis() / 500) % 2) {
        int cursorX = nameBoxX + 5 + (strlen(samplingSampleName) * 6);
        gfx->drawLine(cursorX, namingY - 2, cursorX, namingY + 12, ZGREEN);
    }
    
    int keyboardColor = samplingKeyboardVisible ? ZYELLOW : DARKGREY;
    drawBT(19, keyboardColor, "KEYBOARD");
}

void samplingDrawActionButtons() {
    int saveColor = DARKGREY;
    if ((samplingState == SAMPLING_RECORDED) && sdIsReady()) {
        saveColor = ZGREEN;
    } else if (!sdIsReady()) {
        saveColor = ZRED;
    }
    drawBT(21, saveColor, "SAVE TO SD");
    
    int cancelColor = (samplingState != SAMPLING_IDLE) ? ZYELLOW : DARKGREY;
    drawBT(22, cancelColor, "CANCEL");
    
    drawBT(23, DARKGREY, "RETOUR");
    
    // Contrôles gain
    drawBT(25, DARKGREY, "GAIN-");
    drawBT(28, DARKGREY, "GAIN+");
    
    // Toggle mode
    int modeColor = samplingStereoMode ? ZGREEN : ZYELLOW;
    drawBT(20, modeColor, samplingStereoMode ? "STEREO" : " MONO ");
}

void samplingDrawStatusBar() {
    int statusY = 280;
    
    gfx->drawLine(0, statusY - 5, 479, statusY - 5, DARKGREY);
    gfx->fillRect(0, statusY, 480, 40, BLACK);
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, statusY + 15);
    gfx->print("Buffer: ");
    
    if (samplingRecordedSamples > 0) {
        int bufferPercent = (samplingRecordedSamples * 100) / samplingMaxSamples;
        gfx->printf("%d%% ", bufferPercent);
    } else {
        gfx->print("0% ");
    }
    
    gfx->printf("(%lu samples)", samplingRecordedSamples);
    
    gfx->setCursor(250, statusY + 15);
    gfx->setTextColor(sdIsReady() ? ZGREEN : ZRED, BLACK);
    gfx->print("SD: ");
    gfx->print(sdIsReady() ? "OK" : "ERROR");
    
    gfx->setCursor(350, statusY + 15);
    gfx->setTextColor(wm8731_isReady() ? ZGREEN : ZRED, BLACK);
    gfx->print("AUDIO: ");
    gfx->print(wm8731_isReady() ? "OK" : "ERROR");
}

void samplingDrawKeyboard() {
    if (!samplingKeyboardVisible) return;
    
    gfx->fillRect(20, 200, 440, 100, OSCURO);
    gfx->drawRect(20, 200, 440, 100, ZYELLOW);
    
    gfx->setTextColor(ZYELLOW, OSCURO);
    gfx->setCursor(30, 210);
    gfx->print("CLAVIER TACTILE - Utilisez les pads pour taper");
    
    gfx->setCursor(30, 230);
    gfx->print("Q W E R T Y U I O P");
    gfx->setCursor(30, 250);
    gfx->print("A S D F G H [BACK] [ENTER]");
    
    gfx->setCursor(30, 280);
    gfx->print("Nom actuel: ");
    gfx->setTextColor(ZGREEN, OSCURO);
    gfx->print(samplingSampleName);
}

void drawSamplingStudio() {
    if (samplingInitialized) {
        samplingStudioUpdate();
        
        gfx->fillScreen(BLACK);
        
        samplingDrawHeader();
        samplingDrawAudioControls();
        samplingDrawWaveformArea();
        samplingDrawControlButtons();
        samplingDrawNamingArea();
        samplingDrawActionButtons();
        
        if (samplingKeyboardVisible) {
            samplingDrawKeyboard();
        }
        
        samplingDrawStatusBar();
        
        samplingNeedsRedraw = false;
    } else {
        if (samplingStudioBegin()) {
            drawSamplingStudio();
        } else {
            gfx->fillScreen(BLACK);
            gfx->setTextColor(ZRED, BLACK);
            gfx->setCursor(100, 100);
            gfx->print("ERREUR SAMPLING STUDIO");
            gfx->setCursor(100, 120);
            gfx->print("Vérifiez WM8731 et SD Card");
            drawBT(23, DARKGREY, " RETOUR ");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FILE BROWSER INTÉGRÉ                                    //
////////////////////////////////////////////////////////////////////////////////////////////

// Variables globales du File Browser
#define FB_MAX_PATH_LENGTH 128
#define FB_MAX_FILENAME_LENGTH 64
#define FB_MAX_ITEMS_VISIBLE 10
#define FB_PREVIEW_TIMEOUT 5000

// Variables d'état du File Browser
char fbCurrentPath[FB_MAX_PATH_LENGTH] = "/SAMPLES/";
char fbFileNames[256][FB_MAX_FILENAME_LENGTH];
char fbFilePaths[256][FB_MAX_PATH_LENGTH];
byte fbFileTypes[256];                              // 0=parent, 1=dir, 2=sample
bool fbFileValid[256];
uint16_t fbFileCount = 0;
uint16_t fbSelectedIndex = 0;
uint16_t fbScrollOffset = 0;
byte fbTargetSlot = 0;
bool fbNeedsRefresh = true;
bool fbIsPreviewPlaying = false;
unsigned long fbPreviewStartTime = 0;
bool fbIsInitialized = false;

// Zones tactiles File Browser
#define FB_ZONE_SCROLL_UP 16
#define FB_ZONE_SCROLL_DOWN 17
#define FB_ZONE_PLAY 18
#define FB_ZONE_LOAD 19
#define FB_ZONE_BACK 20

void fileBrowserBegin() {
    Serial.println("=== INITIALISATION FILE BROWSER ===");
    
    strcpy(fbCurrentPath, "/SAMPLES/");
    fbFileCount = 0;
    fbSelectedIndex = 0;
    fbScrollOffset = 0;
    fbTargetSlot = 0;
    fbNeedsRefresh = true;
    fbIsPreviewPlaying = false;
    fbIsInitialized = false;
    
    if (!sdIsReady()) {
        Serial.println("ERREUR: SD Card non disponible");
        return;
    }
    
    fileBrowserScanDirectory();
    fbIsInitialized = true;
    
    Serial.println("File Browser initialisé avec succès");
}

void fileBrowserScanDirectory() {
    Serial.print("Scan du répertoire: ");
    Serial.println(fbCurrentPath);
    
    fbFileCount = 0;
    fbSelectedIndex = 0;
    fbScrollOffset = 0;
    
    if (!sdIsReady()) return;
    
    File root = SD.open(fbCurrentPath);
    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return;
    }
    
    // Ajouter l'entrée parent si pas à la racine
    if (strcmp(fbCurrentPath, "/SAMPLES/") != 0) {
        strcpy(fbFileNames[fbFileCount], "..");
        fbFileTypes[fbFileCount] = 0; // parent
        fbFileValid[fbFileCount] = true;
        fbFileCount++;
    }
    
    // Scanner fichiers et répertoires
    File file = root.openNextFile();
    while (file && fbFileCount < 255) {
        if (file.isDirectory()) {
            strcpy(fbFileNames[fbFileCount], file.name());
            snprintf(fbFilePaths[fbFileCount], sizeof(fbFilePaths[fbFileCount]),
                    "%s%s/", fbCurrentPath, file.name());
            fbFileTypes[fbFileCount] = 1; // directory
            fbFileValid[fbFileCount] = true;
            fbFileCount++;
        } else {
            String fileName = file.name();
            fileName.toLowerCase();
            
            if (fileName.endsWith(".wav")) {
                strcpy(fbFileNames[fbFileCount], file.name());
                snprintf(fbFilePaths[fbFileCount], sizeof(fbFilePaths[fbFileCount]),
                        "%s%s", fbCurrentPath, file.name());
                fbFileTypes[fbFileCount] = 2; // sample
                fbFileValid[fbFileCount] = true;
                fbFileCount++;
            }
        }
        
        file.close();
        file = root.openNextFile();
    }
    
    root.close();
    Serial.print("Items trouvés: ");
    Serial.println(fbFileCount);
}

void fileBrowserUpdate() {
    if (!fbIsInitialized) return;
    
    if (fbIsPreviewPlaying) {
        if (millis() - fbPreviewStartTime > FB_PREVIEW_TIMEOUT) {
            fbIsPreviewPlaying = false;
            fbNeedsRefresh = true;
        }
    }
    
    if (fbNeedsRefresh) {
        drawFileBrowser();
        fbNeedsRefresh = false;
    }
}

void fileBrowserHandleTouch(uint16_t zone) {
    if (!fbIsInitialized) {
        if (zone == FB_ZONE_BACK) {
            returnToMainMenu();
        }
        return;
    }
    
    // Zones liste (0-9)
    if (zone >= 0 && zone <= 9) {
        uint16_t itemIndex = fbScrollOffset + zone;
        if (itemIndex < fbFileCount) {
            fbSelectedIndex = itemIndex;
            fileBrowserSelectItem();
        }
        fbNeedsRefresh = true;
        return;
    }
    
    // Boutons contrôle
    switch (zone) {
        case FB_ZONE_SCROLL_UP:
            if (fbSelectedIndex > 0) {
                fbSelectedIndex--;
                fileBrowserUpdateScrollPosition();
                fbNeedsRefresh = true;
            }
            break;
        case FB_ZONE_SCROLL_DOWN:
            if (fbSelectedIndex < fbFileCount - 1) {
                fbSelectedIndex++;
                fileBrowserUpdateScrollPosition();
                fbNeedsRefresh = true;
            }
            break;
        case FB_ZONE_PLAY:
            fileBrowserPlayPreview();
            break;
        case FB_ZONE_LOAD:
            fileBrowserLoadToSlot();
            break;
        case FB_ZONE_BACK:
            returnToMainMenu();
            break;
    }
}

void fileBrowserSelectItem() {
    if (fbSelectedIndex >= fbFileCount) return;
    
    if (fbFileTypes[fbSelectedIndex] == 0) {
        // Parent
        fileBrowserGoToParentDirectory();
    } else if (fbFileTypes[fbSelectedIndex] == 1) {
        // Répertoire
        fileBrowserEnterDirectory(fbFileNames[fbSelectedIndex]);
    }
}

void fileBrowserEnterDirectory(const char* dirname) {
    char newPath[FB_MAX_PATH_LENGTH];
    snprintf(newPath, sizeof(newPath), "%s%s/", fbCurrentPath, dirname);
    
    strcpy(fbCurrentPath, newPath);
    fileBrowserScanDirectory();
    fbNeedsRefresh = true;
}

void fileBrowserGoToParentDirectory() {
    String pathStr = String(fbCurrentPath);
    
    if (pathStr.endsWith("/")) {
        pathStr = pathStr.substring(0, pathStr.length() - 1);
    }
    
    int lastSlash = pathStr.lastIndexOf('/');
    if (lastSlash >= 0) {
        pathStr = pathStr.substring(0, lastSlash + 1);
        
        if (pathStr.length() < 9) {
            pathStr = "/SAMPLES/";
        }
        
        strcpy(fbCurrentPath, pathStr.c_str());
        fileBrowserScanDirectory();
        fbNeedsRefresh = true;
    }
}

void fileBrowserPlayPreview() {
    if (fbSelectedIndex >= fbFileCount || fbFileTypes[fbSelectedIndex] != 2) return;
    
    fbIsPreviewPlaying = false;
    
    byte previewSlot = 15;
    
    if (sdLoadSample(previewSlot, fbFilePaths[fbSelectedIndex])) {
        if (assignSampleToVoice(previewSlot, SAMPLE_SOURCE_SD, previewSlot)) {
            synthESP32_TRIGGER(previewSlot);
            fbIsPreviewPlaying = true;
            fbPreviewStartTime = millis();
            fbNeedsRefresh = true;
        }
    }
}

void fileBrowserLoadToSlot() {
    if (fbSelectedIndex >= fbFileCount || fbFileTypes[fbSelectedIndex] != 2) return;
    
    if (sdLoadSample(fbTargetSlot, fbFilePaths[fbSelectedIndex])) {
        if (assignSampleToVoice(fbTargetSlot, SAMPLE_SOURCE_SD, fbTargetSlot)) {
            fbTargetSlot = (fbTargetSlot + 1) % 15;
            fbNeedsRefresh = true;
        }
    }
}

void fileBrowserUpdateScrollPosition() {
    if (fbSelectedIndex < fbScrollOffset) {
        fbScrollOffset = fbSelectedIndex;
    } else if (fbSelectedIndex >= fbScrollOffset + FB_MAX_ITEMS_VISIBLE) {
        fbScrollOffset = fbSelectedIndex - FB_MAX_ITEMS_VISIBLE + 1;
    }
}

void drawFileBrowser() {
    if (!fbIsInitialized) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(ZRED, BLACK);
        gfx->setCursor(100, 100);
        gfx->print("ERREUR: SD CARD NON DISPONIBLE");
        drawBT(FB_ZONE_BACK, DARKGREY, " RETOUR ");
        return;
    }
    
    gfx->fillScreen(BLACK);
    
    // En-tête
    gfx->setTextColor(ZYELLOW, BLACK);
    gfx->setCursor(10, 15);
    gfx->print("FILE BROWSER");
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(10, 35);
    gfx->print("Path: ");
    gfx->print(fbCurrentPath);
    
    gfx->drawLine(0, 50, 479, 50, DARKGREY);
    
    // Liste fichiers
    for (int i = 0; i < FB_MAX_ITEMS_VISIBLE && (fbScrollOffset + i) < fbFileCount; i++) {
        int itemIndex = fbScrollOffset + i;
        bool selected = (itemIndex == fbSelectedIndex);
        
        int y = 60 + (i * 18);
        int color = selected ? ZGREEN : LIGHTGREY;
        
        gfx->fillRect(10, y, 460, 16, BLACK);
        
        if (selected) {
            gfx->setTextColor(ZGREEN, BLACK);
            gfx->setCursor(10, y);
            gfx->print(">");
        }
        
        gfx->setCursor(25, y);
        
        if (fbFileTypes[itemIndex] == 0) {
            gfx->setTextColor(ZCYAN, BLACK);
            gfx->print("..");
        } else if (fbFileTypes[itemIndex] == 1) {
            gfx->setTextColor(ZBLUE, BLACK);
            gfx->print("DIR");
        } else {
            gfx->setTextColor(fbFileValid[itemIndex] ? ZGREEN : ZRED, BLACK);
            gfx->print("WAV");
        }
        
        gfx->setCursor(70, y);
        gfx->setTextColor(color, BLACK);
        gfx->print(fbFileNames[itemIndex]);
    }
    
    // Boutons
    drawBT(FB_ZONE_SCROLL_UP, DARKGREY, "   UP   ");
    drawBT(FB_ZONE_SCROLL_DOWN, DARKGREY, "  DOWN  ");
    drawBT(FB_ZONE_PLAY, fbIsPreviewPlaying ? ZRED : ZGREEN, "  PLAY  ");
    drawBT(FB_ZONE_LOAD, ZYELLOW, "  LOAD  ");
    drawBT(FB_ZONE_BACK, DARKGREY, " RETOUR ");
    
    // Barre statut
    int statusY = 250;
    gfx->drawLine(0, statusY - 5, 479, statusY - 5, DARKGREY);
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(10, statusY + 10);
    gfx->print("Items: ");
    gfx->print(fbSelectedIndex + 1);
    gfx->print("/");
    gfx->print(fbFileCount);
    
    gfx->setCursor(150, statusY + 10);
    gfx->print("Target Slot: ");
    gfx->print(fbTargetSlot);
    
    if (fbIsPreviewPlaying) {
        gfx->setTextColor(ZRED, BLACK);
        gfx->setCursor(300, statusY + 10);
        gfx->print("PREVIEW PLAYING...");
    }
}

void drawMidiSettings() {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(ZBLUE, BLACK);
    gfx->setCursor(100, 100);
    gfx->print("MIDI SETTINGS");
    gfx->setCursor(100, 120);
    gfx->print("En cours de developpement...");
    
    // Bouton retour
    drawBT(23, DARKGREY, " RETOUR ");
}

void drawWifiSettings() {
    // Utiliser l'interface WiFi Manager complète
    drawWifiManager();
}

void drawSystemSettings() {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(ZMAGENTA, BLACK);
    gfx->setCursor(100, 100);
    gfx->print("SYSTEM SETTINGS");
    gfx->setCursor(100, 120);
    gfx->print("En cours de developpement...");
    
    // Bouton retour
    drawBT(23, DARKGREY, " RETOUR ");
}

// Fonction pour dessiner l'application actuelle
void drawCurrentApplication() {
    switch (currentApp) {
        case APP_MAIN_MENU:
            drawMainMenu();
            break;
        case APP_DRUM_MACHINE:
            // L'interface drum machine est gérée par le code existant
            break;
        case APP_SAMPLING_STUDIO:
            drawSamplingStudio();
            break;
        case APP_FILE_BROWSER:
            drawFileBrowser();
            break;
        case APP_MIDI_SETTINGS:
            drawMidiSettings();
            break;
        case APP_WIFI_SETTINGS:
            drawWifiSettings();
            break;
        case APP_SYSTEM_SETTINGS:
            drawSystemSettings();
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                                    FIN DU MODULE                                      //
////////////////////////////////////////////////////////////////////////////////////////////