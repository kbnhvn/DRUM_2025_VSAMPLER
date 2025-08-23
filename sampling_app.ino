////////////////////////////////////////////////////////////////////////////////////////////
//                           SAMPLING STUDIO APPLICATION                                 //
//                              DRUM 2025 VSAMPLER                                       //
////////////////////////////////////////////////////////////////////////////////////////////
// Application d'enregistrement audio temps réel avec interface complète
// Intégration WM8731, affichage waveform, clavier tactile, sauvegarde SD
// 
// Auteur: ZCARLOS 2025 - Développé avec IA Assistant
// Version: 1.0
//
// NOTE: Ce module doit être compilé après DRUM_2025_VSAMPLER.ino
// Toutes les définitions globales seront disponibles automatiquement

////////////////////////////////////////////////////////////////////////////////////////////
//                               ÉNUMÉRATIONS ET TYPES                                   //
////////////////////////////////////////////////////////////////////////////////////////////

enum SamplingState {
    SAMPLING_IDLE = 0,      // Prêt à enregistrer
    SAMPLING_RECORDING,     // Enregistrement en cours
    SAMPLING_RECORDED,      // Sample enregistré, prêt playback
    SAMPLING_PLAYING,       // Lecture du sample
    SAMPLING_NAMING,        // Clavier tactile actif
    SAMPLING_SAVING         // Sauvegarde en cours
};

////////////////////////////////////////////////////////////////////////////////////////////
//                               CONFIGURATION ET CONSTANTES                             //
////////////////////////////////////////////////////////////////////////////////////////////

// Configuration enregistrement
#define SAMPLING_BUFFER_DURATION    10      // Durée max en secondes
#define SAMPLING_SAMPLE_RATE        44100   // Fréquence d'échantillonnage
#define SAMPLING_CHANNELS           2       // Stéréo par défaut
#define SAMPLING_BITS_PER_SAMPLE    16      // 16-bit audio
#define SAMPLING_MAX_SAMPLES        (SAMPLING_SAMPLE_RATE * SAMPLING_BUFFER_DURATION)
#define SAMPLING_BUFFER_SIZE        (SAMPLING_MAX_SAMPLES * 2) // bytes

// Configuration affichage waveform
#define WAVEFORM_STUDIO_WIDTH      380      // Largeur zone waveform
#define WAVEFORM_STUDIO_HEIGHT     80       // Hauteur zone waveform
#define WAVEFORM_STUDIO_X          50       // Position X
#define WAVEFORM_STUDIO_Y          70       // Position Y
#define WAVEFORM_REFRESH_RATE      50       // Refresh en ms

// Configuration zones tactiles (selon layout existant)
#define ZONE_GAIN_MINUS            25       // Zone diminuer gain
#define ZONE_GAIN_PLUS             28       // Zone augmenter gain
#define ZONE_MODE_TOGGLE           20       // Zone toggle mono/stéréo
#define ZONE_REC                   16       // Zone bouton REC
#define ZONE_STOP                  17       // Zone bouton STOP
#define ZONE_PLAY                  18       // Zone bouton PLAY
#define ZONE_KEYBOARD              19       // Zone activation clavier
#define ZONE_SAVE                  21       // Zone sauvegarde
#define ZONE_CANCEL                22       // Zone annulation
#define ZONE_BACK                  23       // Zone retour menu

// Configuration clavier tactile
#define KEYBOARD_MAX_NAME_LENGTH   31       // Longueur max nom sample

////////////////////////////////////////////////////////////////////////////////////////////
//                               VARIABLES GLOBALES                                      //
////////////////////////////////////////////////////////////////////////////////////////////

// État de l'application Sampling Studio
SamplingState samplingState = SAMPLING_IDLE;
bool samplingInitialized = false;
bool samplingNeedsRedraw = true;
unsigned long samplingLastUpdate = 0;
unsigned long samplingLastWaveformUpdate = 0;

// Buffer d'enregistrement
short* samplingRecordBuffer = nullptr;
unsigned long samplingRecordedSamples = 0;
unsigned long samplingMaxSamples = SAMPLING_MAX_SAMPLES;
unsigned long samplingRecordStartTime = 0;
unsigned long samplingRecordDuration = 0;

// Paramètres audio
byte samplingInputGain = 15;
byte samplingOutputVolume = 100;
bool samplingStereoMode = true;
byte samplingInputSource = 0;        // 0=Line, 1=Mic

// Nommage et sauvegarde
char samplingSampleName[KEYBOARD_MAX_NAME_LENGTH + 1] = "NewSample";
bool samplingKeyboardVisible = false;
byte samplingKeyboardCursorPos = 0;

// Affichage waveform
short* samplingWaveformBuffer = nullptr;
unsigned long samplingWaveformSamples = 0;
short samplingWaveformMin = 0, samplingWaveformMax = 0;
bool samplingWaveformNeedsUpdate = false;

// Playback
byte samplingPlaybackSlot = 15;
bool samplingIsPlaybackLooping = false;

// Monitoring niveaux
short samplingCurrentInputLevel = 0;
short samplingPeakInputLevel = 0;
unsigned long samplingLastLevelUpdate = 0;

////////////////////////////////////////////////////////////////////////////////////////////
//                               DÉCLARATIONS FONCTIONS                                  //
////////////////////////////////////////////////////////////////////////////////////////////

// Fonctions principales
bool samplingStudioBegin();
void samplingStudioUpdate();
void samplingStudioDraw();
void samplingStudioHandleTouch(int zone);

// Fonctions d'enregistrement
void samplingStartRecording();
void samplingStopRecording();
void samplingUpdateRecording();

// Fonctions de playback
void samplingStartPlayback();
void samplingStopPlayback();
void samplingUpdatePlayback();
bool samplingLoadRecordedSampleToSlot();

// Fonctions d'affichage
void samplingDrawHeader();
void samplingDrawAudioControls();
void samplingDrawWaveformArea();
void samplingDrawControlButtons();
void samplingDrawNamingArea();
void samplingDrawActionButtons();
void samplingDrawStatusBar();
void samplingDrawInputLevelMeter();
void samplingDrawRealtimeWaveform();
void samplingDrawCompleteWaveform();
void samplingDrawWaveformMarkers();
void samplingDrawTimerDisplay();
void samplingDrawKeyboard();

// Fonctions de gestion d'événements
void samplingHandleRecordButton();
void samplingHandleStopButton();
void samplingHandlePlayButton();
void samplingToggleKeyboard();
void samplingHandleSaveButton();
void samplingHandleCancelButton();
void samplingHandleBackButton();
void samplingAdjustInputGain(int delta);
void samplingToggleStereoMode();
void samplingHandleBarTouch(byte barIndex);

// Fonctions utilitaires
void samplingUpdateInputLevelMonitoring();
void samplingUpdateWaveformDisplay();
void samplingGenerateWaveformFromBuffer();

// Fonctions de sauvegarde
void samplingStartSaving();
void samplingUpdateSaving();
bool samplingSaveBufferAsWAV(const char* filename);
void samplingGenerateUniqueFilename(char* filename, int maxLen);

// Fonctions clavier tactile
bool samplingHandleKeyboardTouch(int zone);

////////////////////////////////////////////////////////////////////////////////////////////
//                               IMPLÉMENTATION FONCTIONS PRINCIPALES                    //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Initialisation de l'application Sampling Studio
 */
bool samplingStudioBegin() {
    Serial.println("=== INITIALISATION SAMPLING STUDIO ===");
    
    // Vérifier la disponibilité du WM8731
    if (!wm8731_isReady()) {
        Serial.println("ERREUR: WM8731 non disponible");
        return false;
    }
    
    // Vérifier la disponibilité de la SD
    if (!sdIsReady()) {
        Serial.println("AVERTISSEMENT: SD Card non disponible - sauvegarde désactivée");
    }
    
    // Allouer le buffer d'enregistrement
    samplingRecordBuffer = (short*)malloc(SAMPLING_BUFFER_SIZE);
    if (!samplingRecordBuffer) {
        Serial.println("ERREUR: Impossible d'allouer le buffer d'enregistrement");
        return false;
    }
    
    // Allouer le buffer waveform
    samplingWaveformBuffer = (short*)malloc(WAVEFORM_STUDIO_WIDTH * sizeof(short));
    if (!samplingWaveformBuffer) {
        Serial.println("ERREUR: Impossible d'allouer le buffer waveform");
        free(samplingRecordBuffer);
        samplingRecordBuffer = nullptr;
        return false;
    }
    
    // Configurer WM8731 pour l'enregistrement
    wm8731_setInputGain(samplingInputGain);
    wm8731_setOutputVolume(samplingOutputVolume);
    wm8731_setInputSource(samplingInputSource);
    wm8731_enableInput(true);
    wm8731_enableOutput(true);
    
    // Initialiser l'état
    samplingState = SAMPLING_IDLE;
    samplingRecordedSamples = 0;
    samplingKeyboardVisible = false;
    samplingNeedsRedraw = true;
    samplingInitialized = true;
    
    Serial.println("Sampling Studio initialisé avec succès");
    Serial.printf("Buffer d'enregistrement: %lu échantillons (%d seconds max)\n", 
                 samplingMaxSamples, SAMPLING_BUFFER_DURATION);
    return true;
}

/**
 * Mise à jour principale de l'application
 */
void samplingStudioUpdate() {
    if (!samplingInitialized) return;
    
    unsigned long currentTime = millis();
    
    // Mise à jour selon l'état
    switch (samplingState) {
        case SAMPLING_RECORDING:
            samplingUpdateRecording();
            break;
        case SAMPLING_PLAYING:
            samplingUpdatePlayback();
            break;
        case SAMPLING_SAVING:
            samplingUpdateSaving();
            break;
        default:
            break;
    }
    
    // Mise à jour du monitoring des niveaux
    if (currentTime - samplingLastLevelUpdate >= 50) { // 20Hz
        samplingUpdateInputLevelMonitoring();
        samplingLastLevelUpdate = currentTime;
    }
    
    // Mise à jour du waveform
    if (currentTime - samplingLastWaveformUpdate >= WAVEFORM_REFRESH_RATE) {
        if (samplingWaveformNeedsUpdate || samplingState == SAMPLING_RECORDING) {
            samplingUpdateWaveformDisplay();
            samplingLastWaveformUpdate = currentTime;
        }
    }
    
    // Redessin si nécessaire
    if (samplingNeedsRedraw) {
        samplingStudioDraw();
        samplingNeedsRedraw = false;
    }
    
    samplingLastUpdate = currentTime;
}

/**
 * Affichage principal de l'interface
 */
void samplingStudioDraw() {
    if (!samplingInitialized) return;
    
    // Effacer l'écran
    gfx->fillScreen(BLACK);
    
    // Dessiner les différentes sections
    samplingDrawHeader();
    samplingDrawAudioControls();
    samplingDrawWaveformArea();
    samplingDrawControlButtons();
    samplingDrawNamingArea();
    samplingDrawActionButtons();
    
    // Dessiner le clavier si visible
    if (samplingKeyboardVisible) {
        samplingDrawKeyboard();
    }
    
    // Dessiner la barre de statut
    samplingDrawStatusBar();
}

/**
 * Gestion des événements tactiles
 */
void samplingStudioHandleTouch(int zone) {
    if (!samplingInitialized) return;
    
    Serial.printf("Sampling Studio - Touch zone: %d\n", zone);
    
    // Gestion du clavier si visible
    if (samplingKeyboardVisible && samplingHandleKeyboardTouch(zone)) {
        return;
    }
    
    // Gestion des contrôles selon la zone
    switch (zone) {
        case ZONE_GAIN_MINUS:
            samplingAdjustInputGain(-2);
            break;
        case ZONE_GAIN_PLUS:
            samplingAdjustInputGain(2);
            break;
        case ZONE_MODE_TOGGLE:
            samplingToggleStereoMode();
            break;
        case ZONE_REC:
            samplingHandleRecordButton();
            break;
        case ZONE_STOP:
            samplingHandleStopButton();
            break;
        case ZONE_PLAY:
            samplingHandlePlayButton();
            break;
        case ZONE_KEYBOARD:
            samplingToggleKeyboard();
            break;
        case ZONE_SAVE:
            samplingHandleSaveButton();
            break;
        case ZONE_CANCEL:
            samplingHandleCancelButton();
            break;
        case ZONE_BACK:
            samplingHandleBackButton();
            break;
        default:
            // Zones de gain direct (zones 32-47 correspondent aux barres)
            if (zone >= 32 && zone <= 47) {
                samplingHandleBarTouch(zone - 32);
            }
            break;
    }
    
    samplingNeedsRedraw = true;
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS D'ENREGISTREMENT                             //
////////////////////////////////////////////////////////////////////////////////////////////

void samplingStartRecording() {
    if (samplingState != SAMPLING_IDLE && samplingState != SAMPLING_RECORDED) return;
    
    Serial.println("Démarrage enregistrement...");
    
    // Réinitialiser le buffer
    memset(samplingRecordBuffer, 0, SAMPLING_BUFFER_SIZE);
    samplingRecordedSamples = 0;
    samplingRecordStartTime = millis();
    
    // Activer l'entrée audio
    wm8731_enableInput(true);
    
    // Changer d'état
    samplingState = SAMPLING_RECORDING;
    samplingWaveformNeedsUpdate = true;
    samplingNeedsRedraw = true;
    
    Serial.println("Enregistrement démarré");
}

void samplingStopRecording() {
    if (samplingState != SAMPLING_RECORDING) return;
    
    Serial.printf("Arrêt enregistrement - %lu échantillons capturés\n", samplingRecordedSamples);
    
    samplingRecordDuration = millis() - samplingRecordStartTime;
    samplingState = SAMPLING_RECORDED;
    samplingWaveformNeedsUpdate = true;
    samplingNeedsRedraw = true;
    
    // Générer le waveform complet
    samplingGenerateWaveformFromBuffer();
}

void samplingUpdateRecording() {
    if (samplingState != SAMPLING_RECORDING) return;
    
    // Lire les données audio depuis WM8731
    byte audioBuffer[512]; // Buffer temporaire
    int bytesRead = wm8731_readAudio(audioBuffer, 256);
    
    if (bytesRead > 0) {
        // Convertir et stocker dans le buffer d'enregistrement
        unsigned long samplesAvailable = bytesRead / (samplingStereoMode ? 4 : 2);
        
        for (unsigned long i = 0; i < samplesAvailable && samplingRecordedSamples < samplingMaxSamples; i++) {
            short sample;
            
            if (samplingStereoMode) {
                // Stéréo - mixer les deux canaux
                short left = *((short*)(audioBuffer + (i * 4)));
                short right = *((short*)(audioBuffer + (i * 4) + 2));
                sample = (left + right) >> 1;
            } else {
                // Mono
                sample = *((short*)(audioBuffer + (i * 2)));
            }
            
            samplingRecordBuffer[samplingRecordedSamples] = sample;
            samplingRecordedSamples++;
        }
    }
    
    // Arrêt automatique si buffer plein ou durée max atteinte
    if (samplingRecordedSamples >= samplingMaxSamples || 
        (millis() - samplingRecordStartTime) >= (SAMPLING_BUFFER_DURATION * 1000)) {
        samplingStopRecording();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS DE PLAYBACK                                   //
////////////////////////////////////////////////////////////////////////////////////////////

void samplingStartPlayback() {
    if (samplingState != SAMPLING_RECORDED || samplingRecordedSamples == 0) return;
    
    Serial.println("Démarrage playback...");
    
    // Charger le sample dans un slot SD temporaire
    if (!samplingLoadRecordedSampleToSlot()) {
        Serial.println("ERREUR: Impossible de charger le sample pour playback");
        return;
    }
    
    // Déclencher la lecture
    synthESP32_TRIGGER(samplingPlaybackSlot);
    
    samplingState = SAMPLING_PLAYING;
    samplingNeedsRedraw = true;
    
    Serial.println("Playback démarré");
}

void samplingStopPlayback() {
    if (samplingState != SAMPLING_PLAYING) return;
    
    Serial.println("Arrêt playback");
    
    // Arrêter la lecture (mettre latch à 0)
    latch[samplingPlaybackSlot] = 0;
    
    samplingState = SAMPLING_RECORDED;
    samplingNeedsRedraw = true;
}

void samplingUpdatePlayback() {
    // Vérifier si le playback est terminé
    if (!latch[samplingPlaybackSlot]) {
        if (samplingIsPlaybackLooping) {
            // Relancer si en mode loop
            synthESP32_TRIGGER(samplingPlaybackSlot);
        } else {
            samplingStopPlayback();
        }
    }
}

bool samplingLoadRecordedSampleToSlot() {
    if (!samplingRecordBuffer || samplingRecordedSamples == 0) return false;
    
    // Sauvegarder temporairement sur SD puis charger
    char tempPath[] = "/SAMPLES/USER/temp_preview.wav";
    if (samplingSaveBufferAsWAV(tempPath)) {
        return sdLoadSample(samplingPlaybackSlot, tempPath);
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS D'AFFICHAGE                                   //
////////////////////////////////////////////////////////////////////////////////////////////

void samplingDrawHeader() {
    gfx->setTextColor(ZRED, BLACK);
    gfx->setCursor(120, 15);
    gfx->print("SAMPLING STUDIO");
    
    // Indicateur d'état
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
    
    // Contrôles input
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, yPos);
    gfx->print("Input:");
    
    gfx->setCursor(80, yPos);
    gfx->print(samplingInputSource == 0 ? "Line" : "Mic");
    
    gfx->setCursor(150, yPos);
    gfx->print("Gain:");
    
    // Barre de gain visuelle
    int gainBarWidth = 100;
    int gainLevel = map(samplingInputGain, 0, 31, 0, gainBarWidth);
    gfx->drawRect(200, yPos - 3, gainBarWidth + 2, 12, DARKGREY);
    gfx->fillRect(201, yPos - 2, gainLevel, 10, ZGREEN);
    
    gfx->setCursor(320, yPos);
    gfx->printf("%d", samplingInputGain);
    
    // Mode stéréo/mono
    gfx->setCursor(380, yPos);
    gfx->setTextColor(samplingStereoMode ? ZGREEN : DARKGREY, BLACK);
    gfx->print(samplingStereoMode ? "[STEREO]" : "[MONO]");
    
    // Niveau d'entrée en temps réel
    if (samplingState == SAMPLING_RECORDING || samplingState == SAMPLING_IDLE) {
        samplingDrawInputLevelMeter();
    }
}

void samplingDrawInputLevelMeter() {
    int meterX = 400;
    int meterY = 40;
    int meterWidth = 60;
    int meterHeight = 8;
    
    // Cadre du VU-mètre
    gfx->drawRect(meterX, meterY, meterWidth + 2, meterHeight + 2, DARKGREY);
    gfx->fillRect(meterX + 1, meterY + 1, meterWidth, meterHeight, BLACK);
    
    // Niveau actuel
    if (samplingCurrentInputLevel > 0) {
        int levelWidth = map(abs(samplingCurrentInputLevel), 0, 32767, 0, meterWidth);
        int levelColor = ZGREEN;
        
        if (levelWidth > meterWidth * 0.8) {
            levelColor = ZRED; // Niveau élevé
        } else if (levelWidth > meterWidth * 0.6) {
            levelColor = ZYELLOW; // Niveau moyen
        }
        
        gfx->fillRect(meterX + 1, meterY + 1, levelWidth, meterHeight, levelColor);
    }
    
    // Peak hold
    if (samplingPeakInputLevel > 0) {
        int peakPos = map(abs(samplingPeakInputLevel), 0, 32767, 0, meterWidth);
        gfx->drawLine(meterX + peakPos, meterY, meterX + peakPos, meterY + meterHeight + 1, WHITE);
    }
}

void samplingDrawWaveformArea() {
    // Cadre de la zone waveform
    gfx->drawRect(WAVEFORM_STUDIO_X - 2, WAVEFORM_STUDIO_Y - 2, 
                  WAVEFORM_STUDIO_WIDTH + 4, WAVEFORM_STUDIO_HEIGHT + 4, DARKGREY);
    gfx->fillRect(WAVEFORM_STUDIO_X, WAVEFORM_STUDIO_Y, WAVEFORM_STUDIO_WIDTH, WAVEFORM_STUDIO_HEIGHT, BLACK);
    
    // Ligne de référence au centre
    int centerY = WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT / 2;
    gfx->drawLine(WAVEFORM_STUDIO_X, centerY, WAVEFORM_STUDIO_X + WAVEFORM_STUDIO_WIDTH - 1, centerY, OSCURO);
    
    // Afficher le waveform selon l'état
    if (samplingState == SAMPLING_RECORDING) {
        samplingDrawRealtimeWaveform();
    } else if (samplingState == SAMPLING_RECORDED || samplingState == SAMPLING_PLAYING) {
        samplingDrawCompleteWaveform();
    }
    
    // Marqueurs de début/fin si échantillon enregistré
    if (samplingState == SAMPLING_RECORDED || samplingState == SAMPLING_PLAYING) {
        samplingDrawWaveformMarkers();
    }
}

void samplingDrawRealtimeWaveform() {
    if (!samplingRecordBuffer || samplingRecordedSamples < 2) return;
    
    // Afficher les dernières données en temps réel
    unsigned long startSample = samplingRecordedSamples > WAVEFORM_STUDIO_WIDTH ? 
                          samplingRecordedSamples - WAVEFORM_STUDIO_WIDTH : 0;
    
    for (int x = 0; x < WAVEFORM_STUDIO_WIDTH - 1 && (startSample + x) < samplingRecordedSamples; x++) {
        short sample1 = samplingRecordBuffer[startSample + x];
        short sample2 = samplingRecordBuffer[startSample + x + 1];
        
        int y1 = map(sample1, -32768, 32767, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        int y2 = map(sample2, -32768, 32767, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        
        gfx->drawLine(WAVEFORM_STUDIO_X + x, WAVEFORM_STUDIO_Y + y1, 
                     WAVEFORM_STUDIO_X + x + 1, WAVEFORM_STUDIO_Y + y2, ZGREEN);
    }
    
    // Indicateur de progression
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
        
        short sample1 = samplingWaveformBuffer[x];
        short sample2 = (x + 1 < samplingWaveformSamples) ? samplingWaveformBuffer[x + 1] : sample1;
        
        int y1 = map(sample1, samplingWaveformMin, samplingWaveformMax, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        int y2 = map(sample2, samplingWaveformMin, samplingWaveformMax, WAVEFORM_STUDIO_HEIGHT - 1, 0);
        
        int waveColor = ZCYAN;
        if (samplingState == SAMPLING_PLAYING) {
            waveColor = ZGREEN; // Couleur différente pendant lecture
        }
        
        gfx->drawLine(WAVEFORM_STUDIO_X + x, WAVEFORM_STUDIO_Y + y1, 
                     WAVEFORM_STUDIO_X + x + 1, WAVEFORM_STUDIO_Y + y2, waveColor);
    }
}

void samplingDrawWaveformMarkers() {
    // Marqueur de début (vert)
    gfx->drawLine(WAVEFORM_STUDIO_X + 2, WAVEFORM_STUDIO_Y, 
                 WAVEFORM_STUDIO_X + 2, WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT - 1, ZGREEN);
    
    // Marqueur de fin (rouge)
    int endX = WAVEFORM_STUDIO_X + WAVEFORM_STUDIO_WIDTH - 3;
    gfx->drawLine(endX, WAVEFORM_STUDIO_Y, endX, WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT - 1, ZRED);
}

void samplingDrawControlButtons() {
    // Bouton REC
    int recColor = (samplingState == SAMPLING_RECORDING) ? ZRED : DARKGREY;
    if (samplingState == SAMPLING_IDLE || samplingState == SAMPLING_RECORDED) {
        recColor = ZGREEN; // Prêt à enregistrer
    }
    drawBT(ZONE_REC, recColor, "●REC");
    
    // Bouton STOP
    int stopColor = (samplingState == SAMPLING_RECORDING || samplingState == SAMPLING_PLAYING) ? ZRED : DARKGREY;
    drawBT(ZONE_STOP, stopColor, "■STOP");
    
    // Bouton PLAY
    int playColor = DARKGREY;
    if (samplingState == SAMPLING_RECORDED) {
        playColor = ZGREEN; // Prêt à jouer
    } else if (samplingState == SAMPLING_PLAYING) {
        playColor = ZBLUE; // En cours de lecture
    }
    drawBT(ZONE_PLAY, playColor, "▶PLAY");
    
    // Affichage de la durée
    samplingDrawTimerDisplay();
}

void samplingDrawTimerDisplay() {
    int timerX = 350;
    int timerY = WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT + 30;
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(timerX, timerY);
    
    unsigned long currentDuration = 0;
    if (samplingState == SAMPLING_RECORDING) {
        currentDuration = millis() - samplingRecordStartTime;
    } else if (samplingState == SAMPLING_RECORDED || samplingState == SAMPLING_PLAYING) {
        currentDuration = samplingRecordDuration;
    }
    
    unsigned long seconds = currentDuration / 1000;
    unsigned long milliseconds = (currentDuration % 1000) / 10;
    
    gfx->printf("%02lu.%02lus", seconds, milliseconds);
}

void samplingDrawNamingArea() {
    int namingY = WAVEFORM_STUDIO_Y + WAVEFORM_STUDIO_HEIGHT + 70;
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, namingY);
    gfx->print("Name:");
    
    // Zone de texte du nom
    int nameBoxX = 80;
    int nameBoxWidth = 200;
    int nameBoxHeight = 20;
    
    gfx->drawRect(nameBoxX, namingY - 5, nameBoxWidth, nameBoxHeight, DARKGREY);
    gfx->fillRect(nameBoxX + 1, namingY - 4, nameBoxWidth - 2, nameBoxHeight - 2, BLACK);
    
    gfx->setTextColor(ZGREEN, BLACK);
    gfx->setCursor(nameBoxX + 5, namingY);
    gfx->print(samplingSampleName);
    
    // Curseur clignotant si clavier actif
    if (samplingKeyboardVisible && (millis() / 500) % 2) {
        int cursorX = nameBoxX + 5 + (strlen(samplingSampleName) * 6);
        gfx->drawLine(cursorX, namingY - 2, cursorX, namingY + 12, ZGREEN);
    }
    
    // Bouton KEYBOARD
    int keyboardColor = samplingKeyboardVisible ? ZYELLOW : DARKGREY;
    drawBT(ZONE_KEYBOARD, keyboardColor, "KEYBOARD");
}

void samplingDrawActionButtons() {
    // Bouton SAVE TO SD
    int saveColor = DARKGREY;
    if ((samplingState == SAMPLING_RECORDED) && sdIsReady()) {
        saveColor = ZGREEN;
    } else if (!sdIsReady()) {
        saveColor = ZRED; // SD non disponible
    }
    drawBT(ZONE_SAVE, saveColor, "SAVE TO SD");
    
    // Bouton CANCEL
    int cancelColor = (samplingState != SAMPLING_IDLE) ? ZYELLOW : DARKGREY;
    drawBT(ZONE_CANCEL, cancelColor, "CANCEL");
    
    // Bouton RETOUR
    drawBT(ZONE_BACK, DARKGREY, "RETOUR");
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
    
    // Statut SD
    gfx->setCursor(250, statusY + 15);
    gfx->setTextColor(sdIsReady() ? ZGREEN : ZRED, BLACK);
    gfx->print("SD: ");
    gfx->print(sdIsReady() ? "OK" : "ERROR");
    
    // Statut WM8731
    gfx->setCursor(350, statusY + 15);
    gfx->setTextColor(wm8731_isReady() ? ZGREEN : ZRED, BLACK);
    gfx->print("AUDIO: ");
    gfx->print(wm8731_isReady() ? "OK" : "ERROR");
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               GESTION D'ÉVÉNEMENTS                                    //
////////////////////////////////////////////////////////////////////////////////////////////

void samplingHandleRecordButton() {
    switch (samplingState) {
        case SAMPLING_IDLE:
        case SAMPLING_RECORDED:
            samplingStartRecording();
            break;
        default:
            break;
    }
}

void samplingHandleStopButton() {
    switch (samplingState) {
        case SAMPLING_RECORDING:
            samplingStopRecording();
            break;
        case SAMPLING_PLAYING:
            samplingStopPlayback();
            break;
        default:
            break;
    }
}

void samplingHandlePlayButton() {
    switch (samplingState) {
        case SAMPLING_RECORDED:
            samplingStartPlayback();
            break;
        case SAMPLING_PLAYING:
            samplingStopPlayback();
            break;
        default:
            break;
    }
}

void samplingToggleKeyboard() {
    if (samplingState == SAMPLING_RECORDED || samplingState == SAMPLING_IDLE) {
        samplingKeyboardVisible = !samplingKeyboardVisible;
        if (samplingKeyboardVisible) {
            samplingState = SAMPLING_NAMING;
        } else {
            samplingState = (samplingRecordedSamples > 0) ? SAMPLING_RECORDED : SAMPLING_IDLE;
        }
        samplingNeedsRedraw = true;
    }
}

void samplingHandleSaveButton() {
    if (samplingState == SAMPLING_RECORDED && sdIsReady()) {
        samplingStartSaving();
    }
}

void samplingHandleCancelButton() {
    switch (samplingState) {
        case SAMPLING_RECORDED:
        case SAMPLING_PLAYING:
            // Effacer l'enregistrement
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
    samplingNeedsRedraw = true;
}

void samplingHandleBackButton() {
    // Sauvegarder l'état si nécessaire
    if (samplingState == SAMPLING_RECORDING) {
        samplingStopRecording();
    }
    if (samplingState == SAMPLING_PLAYING) {
        samplingStopPlayback();
    }
    
    // Libérer la mémoire
    if (samplingRecordBuffer) {
        free(samplingRecordBuffer);
        samplingRecordBuffer = nullptr;
    }
    if (samplingWaveformBuffer) {
        free(samplingWaveformBuffer);
        samplingWaveformBuffer = nullptr;
    }
    
    // Retourner au menu principal
    returnToMainMenu();
}

void samplingAdjustInputGain(int delta) {
    int newGain = samplingInputGain + delta;
    samplingInputGain = constrain(newGain, 0, 31);
    wm8731_setInputGain(samplingInputGain);
    samplingNeedsRedraw = true;
    
    Serial.printf("Gain input ajusté: %d\n", samplingInputGain);
}

void samplingToggleStereoMode() {
    samplingStereoMode = !samplingStereoMode;
    samplingNeedsRedraw = true;
    
    Serial.printf("Mode audio: %s\n", samplingStereoMode ? "Stéréo" : "Mono");
}

void samplingHandleBarTouch(byte barIndex) {
    // Gestion des barres pour contrôles avancés
    if (barIndex == 0) { // Gain
        samplingInputGain = map(BPOS[barIndex + 32][1], 0, BPOS[barIndex + 32][3], 0, 31);
        wm8731_setInputGain(samplingInputGain);
    } else if (barIndex == 1) { // Volume output
        samplingOutputVolume = map(BPOS[barIndex + 32][1], 0, BPOS[barIndex + 32][3], 48, 127);
        wm8731_setOutputVolume(samplingOutputVolume);
    }
    samplingNeedsRedraw = true;
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS UTILITAIRES                                   //
////////////////////////////////////////////////////////////////////////////////////////////

void samplingUpdateInputLevelMonitoring() {
    if (!wm8731_isReady()) return;
    
    // Lire un petit échantillon pour monitoring
    byte monitorBuffer[32];
    int bytesRead = wm8731_readAudio(monitorBuffer, 16);
    
    if (bytesRead > 0) {
        short maxLevel = 0;
        for (int i = 0; i < bytesRead; i += 2) {
            short sample = *((short*)(monitorBuffer + i));
            if (abs(sample) > maxLevel) {
                maxLevel = abs(sample);
            }
        }
        
        samplingCurrentInputLevel = maxLevel;
        
        // Peak hold avec décroissance lente
        if (maxLevel > samplingPeakInputLevel) {
            samplingPeakInputLevel = maxLevel;
        } else if (samplingPeakInputLevel > 0) {
            samplingPeakInputLevel = (samplingPeakInputLevel * 95) / 100; // Décroissance lente
        }
    }
}

void samplingUpdateWaveformDisplay() {
    if (samplingState == SAMPLING_RECORDING) {
        samplingWaveformNeedsUpdate = true; // Toujours mettre à jour en temps réel
    } else if (samplingWaveformNeedsUpdate) {
        samplingGenerateWaveformFromBuffer();
        samplingWaveformNeedsUpdate = false;
    }
}

void samplingGenerateWaveformFromBuffer() {
    if (!samplingRecordBuffer || !samplingWaveformBuffer || samplingRecordedSamples == 0) return;
    
    // Décimer le buffer pour l'affichage
    unsigned long step = samplingRecordedSamples / WAVEFORM_STUDIO_WIDTH;
    if (step == 0) step = 1;
    
    samplingWaveformMin = 32767;
    samplingWaveformMax = -32768;
    
    for (int x = 0; x < WAVEFORM_STUDIO_WIDTH && x < samplingRecordedSamples; x++) {
        unsigned long sampleIndex = x * step;
        if (sampleIndex < samplingRecordedSamples) {
            samplingWaveformBuffer[x] = samplingRecordBuffer[sampleIndex];
            
            if (samplingWaveformBuffer[x] < samplingWaveformMin) samplingWaveformMin = samplingWaveformBuffer[x];
            if (samplingWaveformBuffer[x] > samplingWaveformMax) samplingWaveformMax = samplingWaveformBuffer[x];
        }
    }
    
    samplingWaveformSamples = min(WAVEFORM_STUDIO_WIDTH, samplingRecordedSamples);
    
    // Assurer une plage minimum pour l'affichage
    if (samplingWaveformMax - samplingWaveformMin < 1000) {
        int center = (samplingWaveformMax + samplingWaveformMin) / 2;
        samplingWaveformMin = center - 500;
        samplingWaveformMax = center + 500;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS DE SAUVEGARDE                                 //
////////////////////////////////////////////////////////////////////////////////////////////

void samplingStartSaving() {
    if (!sdIsReady() || samplingRecordedSamples == 0) return;
    
    Serial.println("Démarrage sauvegarde...");
    samplingState = SAMPLING_SAVING;
    samplingNeedsRedraw = true;
    
    // Créer le nom de fichier
    char filename[64];
    snprintf(filename, sizeof(filename), "/SAMPLES/USER/%s.wav", samplingSampleName);
    
    // Vérifier si le fichier existe et incrémenter si nécessaire
    samplingGenerateUniqueFilename(filename, sizeof(filename));
    
    // Sauvegarder
    if (samplingSaveBufferAsWAV(filename)) {
        Serial.printf("Sample sauvegardé: %s\n", filename);
        
        // Retour à l'état enregistré après un délai
        delay(1000);
        samplingState = SAMPLING_RECORDED;
    } else {
        Serial.println("ERREUR: Échec de la sauvegarde");
        samplingState = SAMPLING_RECORDED;
    }
    
    samplingNeedsRedraw = true;
}

void samplingUpdateSaving() {
    // Gestion de l'état de sauvegarde (animation, etc.)
    // Pour l'instant, la sauvegarde est bloquante
}

bool samplingSaveBufferAsWAV(const char* filename) {
    if (!sdIsReady() || !samplingRecordBuffer || samplingRecordedSamples == 0) return false;
    
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.printf("Impossible de créer le fichier: %s\n", filename);
        return false;
    }
    
    // Calculer les tailles
    unsigned long dataSize = samplingRecordedSamples * 2; // 16-bit samples
    unsigned long fileSize = 44 + dataSize;
    int channels = samplingStereoMode ? 2 : 1;
    
    // En-tête WAV
    file.write((byte*)"RIFF", 4);
    file.write((byte*)&fileSize, 4);
    file.write((byte*)"WAVE", 4);
    
    // Chunk fmt
    file.write((byte*)"fmt ", 4);
    unsigned long fmtSize = 16;
    file.write((byte*)&fmtSize, 4);
    int audioFormat = 1; // PCM
    file.write((byte*)&audioFormat, 2);
    file.write((byte*)&channels, 2);
    unsigned long sampleRate = SAMPLING_SAMPLE_RATE;
    file.write((byte*)&sampleRate, 4);
    unsigned long byteRate = sampleRate * channels * 2;
    file.write((byte*)&byteRate, 4);
    int blockAlign = channels * 2;
    file.write((byte*)&blockAlign, 2);
    int bitsPerSample = 16;
    file.write((byte*)&bitsPerSample, 2);
    
    // Chunk data
    file.write((byte*)"data", 4);
    file.write((byte*)&dataSize, 4);
    
    // Données audio
    file.write((byte*)samplingRecordBuffer, dataSize);
    
    file.close();
    return true;
}

void samplingGenerateUniqueFilename(char* filename, int maxLen) {
    if (!SD.exists(filename)) return; // Le nom est unique
    
    // Extraire le nom de base et l'extension
    String baseName = String(filename);
    int lastDot = baseName.lastIndexOf('.');
    String nameWithoutExt = baseName.substring(0, lastDot);
    String extension = baseName.substring(lastDot);
    
    // Essayer avec des suffixes numériques
    for (int i = 1; i < 1000; i++) {
        String newName = nameWithoutExt + "_" + String(i) + extension;
        if (!SD.exists(newName.c_str())) {
            strncpy(filename, newName.c_str(), maxLen - 1);
            filename[maxLen - 1] = '\0';
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               CLAVIER TACTILE                                         //
////////////////////////////////////////////////////////////////////////////////////////////

bool samplingHandleKeyboardTouch(int zone) {
    // Implémentation simplifiée du clavier tactile
    // Utiliser les zones pads comme clavier QWERTY simplifié
    
    if (zone >= 0 && zone <= 15) {
        // Zones pads utilisées comme clavier QWERTY simplifié
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

void samplingDrawKeyboard() {
    if (!samplingKeyboardVisible) return;
    
    // Zone sombre pour le clavier
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

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS POUR INTÉGRATION MENU                        //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Fonction pour dessiner l'application depuis le menu system
 * Remplace le stub dans menu_system.ino
 */
void drawSamplingStudio() {
    if (samplingInitialized) {
        samplingStudioDraw();
    } else {
        // Première initialisation
        if (samplingStudioBegin()) {
            samplingStudioDraw();
        } else {
            // Affichage d'erreur
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
//                                    FIN DU MODULE                                      //
////////////////////////////////////////////////////////////////////////////////////////////