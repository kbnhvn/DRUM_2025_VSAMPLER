/**
 * Exemple d'intégration du driver WM8731 dans DRUM_2025_VSAMPLER
 * 
 * Ce fichier montre comment modifier synthESP32.ino pour utiliser le WM8731
 * au lieu du PCM5102A, en ajoutant les fonctionnalités audio bidirectionnelles.
 * 
 * Auteur: ZCARLOS 2025
 * Version: 1.0
 */

// ==========================================
// MODIFICATIONS DANS DRUM_2025_VSAMPLER.ino
// ==========================================

/*
1. Ajouter l'include du driver WM8731 après les autres includes :

#include "wm8731.ino"

2. Modifier la section I2S pins dans DRUM_2025_VSAMPLER.ino :

// Remplacer les définitions existantes :
#ifdef mylcd_type2
  #define I2S_BCK_PIN 9        // Ancienne valeur
  #define I2S_WS_PIN 5         // Ancienne valeur  
  #define I2S_DATA_OUT_PIN 14  // Ancienne valeur
#endif

// Par les nouvelles valeurs pour WM8731 :
#ifdef mylcd_type2
  #define I2S_BCK_PIN 14       // Nouvelle valeur pour WM8731
  #define I2S_WS_PIN 15        // Nouvelle valeur pour WM8731
  #define I2S_DATA_OUT_PIN 13  // Nouvelle valeur pour WM8731
  #define I2S_DATA_IN_PIN 12   // Nouvelle pin pour input audio
#endif

3. Dans la fonction setup(), remplacer l'appel synthESP32_begin() par :

  // Initialiser WM8731 avant synthESP32
  if (!wm8731_begin()) {
    Serial.println("Erreur: WM8731 non initialisé!");
    while(1) delay(1000);
  }
  
  // Configuration initiale WM8731
  wm8731_setOutputVolume(100);    // Volume de sortie (48-127)
  wm8731_setInputGain(20);        // Gain d'entrée (0-31)
  wm8731_setInputSource(0);       // 0=Line, 1=Mic
  wm8731_enableOutput(true);      // Activer sortie
  wm8731_enableInput(false);      // Désactiver entrée par défaut
  
  // Puis initialiser synthESP32 (SANS la config I2S qui est déjà faite)
  synthESP32_begin_without_i2s();
*/

// ==========================================
// MODIFICATIONS DANS synthESP32.ino
// ==========================================

/*
1. Créer une nouvelle fonction synthESP32_begin_without_i2s() :

void synthESP32_begin_without_i2s(){
  // 16 filters + master L & R (18)
  for (int fi=0; fi < 18; fi++) {
    FILTROS[fi].setResonance(reso);
    FILTROS[fi].setCutoffFreq(cutoff);
  }

  // NE PAS configurer I2S ici - déjà fait par WM8731

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
*/

// ==========================================
// EXEMPLE D'UTILISATION DES NOUVELLES FONCTIONNALITÉS
// ==========================================

/**
 * Exemple de fonction pour enregistrer de l'audio
 */
void startAudioRecording() {
    if (!wm8731_isReady()) {
        Serial.println("WM8731 non prêt pour l'enregistrement");
        return;
    }
    
    Serial.println("Démarrage enregistrement audio...");
    
    // Activer l'entrée audio
    wm8731_enableInput(true);
    wm8731_setInputSource(0); // Line input
    wm8731_setInputGain(25);  // Gain d'entrée
    
    // Buffer pour l'enregistrement
    uint8_t audioBuffer[DMA_BUF_LEN * 4]; // 16-bit stéréo
    
    // Enregistrer quelques échantillons
    for (int i = 0; i < 10; i++) {
        size_t bytesRead = wm8731_readAudio(audioBuffer, DMA_BUF_LEN);
        if (bytesRead > 0) {
            Serial.printf("Enregistré %d octets\n", bytesRead);
            // Ici vous pouvez traiter les données audio
            // Par exemple, les sauvegarder sur SD card
        }
        delay(10);
    }
    
    // Désactiver l'entrée
    wm8731_enableInput(false);
    Serial.println("Enregistrement terminé");
}

/**
 * Exemple de fonction pour ajuster les paramètres audio en temps réel
 */
void adjustAudioParameters() {
    static uint8_t currentVolume = 100;
    static uint8_t currentGain = 20;
    static bool inputEnabled = false;
    
    // Exemple d'ajustement avec les potentiomètres existants
    #ifdef ads_ok
    if (selected_rot == 9) { // Si on édite le volume master
        uint8_t newVolume = map(ROTvalue[selected_sound][5], 0, 127, 48, 127);
        if (newVolume != currentVolume) {
            currentVolume = newVolume;
            wm8731_setOutputVolume(currentVolume);
            Serial.printf("Volume WM8731: %d\n", currentVolume);
        }
    }
    
    if (selected_rot == 10) { // Si on édite un paramètre custom pour gain input
        uint8_t newGain = map(ztranspose + 12, 0, 24, 0, 31);
        if (newGain != currentGain) {
            currentGain = newGain;
            wm8731_setInputGain(currentGain);
            Serial.printf("Gain input WM8731: %d\n", currentGain);
        }
    }
    #endif
}

/**
 * Exemple de fonction pour basculer entre Line et Mic
 */
void toggleInputSource() {
    static uint8_t currentSource = 0; // 0=Line, 1=Mic
    
    currentSource = (currentSource == 0) ? 1 : 0;
    wm8731_setInputSource(currentSource);
    
    Serial.printf("Source input: %s\n", (currentSource == 0) ? "Line" : "Microphone");
}

/**
 * Exemple d'intégration dans la boucle principale
 */
void exampleMainLoop() {
    // Dans la fonction loop() du projet principal, ajouter :
    
    /*
    // Gestion des paramètres audio WM8731
    adjustAudioParameters();
    
    // Exemple de contrôle via touch/boutons
    if (modeZ == tSampling) { // Nouveau mode pour sampling
        if (some_button_pressed) {
            startAudioRecording();
        }
    }
    
    if (some_other_button_pressed) {
        toggleInputSource();
    }
    */
}

/**
 * Exemple d'intégration avec le séquenceur existant
 */
void exampleSequencerIntegration() {
    // Dans la fonction onSync24() ou équivalent, on peut ajouter :
    
    /*
    // Enregistrement synchronisé avec le séquenceur
    if (recording && (sstep == 0)) { // Début de pattern
        // Démarrer enregistrement
        wm8731_enableInput(true);
    }
    
    if (recording && (sstep == lastStep)) { // Fin de pattern
        // Arrêter enregistrement
        wm8731_enableInput(false);
    }
    */
}

/**
 * Test de fonctionnement du WM8731
 */
void testWM8731() {
    Serial.println("=== Test WM8731 ===");
    
    if (!wm8731_isReady()) {
        Serial.println("ERREUR: WM8731 non initialisé!");
        return;
    }
    
    Serial.println("WM8731 prêt");
    
    // Test sortie
    Serial.println("Test sortie audio...");
    wm8731_enableOutput(true);
    wm8731_setOutputVolume(80);
    
    // Jouer un son via l'architecture existante
    synthESP32_TRIGGER(0); // Déclencher le premier pad
    delay(1000);
    
    // Test entrée
    Serial.println("Test entrée audio...");
    wm8731_enableInput(true);
    wm8731_setInputSource(0); // Line input
    wm8731_setInputGain(20);
    
    uint8_t testBuffer[256];
    size_t bytesRead = wm8731_readAudio(testBuffer, 64);
    Serial.printf("Lu %d octets depuis l'entrée\n", bytesRead);
    
    wm8731_enableInput(false);
    
    Serial.println("Test terminé");
}

// ==========================================
// FONCTIONS HELPER POUR L'INTÉGRATION
// ==========================================

/**
 * Sauvegarde des paramètres WM8731 dans SPIFFS
 */
void saveWM8731Settings() {
    // Exemple de sauvegarde des paramètres
    File file = SPIFFS.open("/wm8731_settings.dat", FILE_WRITE);
    if (file) {
        file.write(wm8731.getOutputVolume());
        file.write(wm8731.getInputGain());
        file.write(wm8731.getInputSource());
        file.close();
        Serial.println("Paramètres WM8731 sauvegardés");
    }
}

/**
 * Chargement des paramètres WM8731 depuis SPIFFS
 */
void loadWM8731Settings() {
    File file = SPIFFS.open("/wm8731_settings.dat", FILE_READ);
    if (file) {
        uint8_t volume = file.read();
        uint8_t gain = file.read();
        uint8_t source = file.read();
        file.close();
        
        wm8731_setOutputVolume(volume);
        wm8731_setInputGain(gain);
        wm8731_setInputSource(source);
        
        Serial.println("Paramètres WM8731 chargés");
    }
}

/**
 * Gestion MIDI pour contrôle WM8731
 */
void handleWM8731MidiCC(byte channel, byte control, byte value) {
    switch (control) {
        case 7:  // Volume
            wm8731_setOutputVolume(map(value, 0, 127, 48, 127));
            break;
        case 71: // Input Gain
            wm8731_setInputGain(map(value, 0, 127, 0, 31));
            break;
        case 72: // Input Source
            wm8731_setInputSource(value > 63 ? 1 : 0);
            break;
        case 73: // Input Enable
            wm8731_enableInput(value > 63);
            break;
    }
}

// ==========================================
// NOTES D'INTÉGRATION
// ==========================================

/*
ÉTAPES D'INTÉGRATION COMPLÈTE :

1. Copier wm8731.ino dans le répertoire du projet
2. Inclure le fichier dans DRUM_2025_VSAMPLER.ino
3. Modifier les définitions de pins I2S
4. Adapter synthESP32_begin() pour utiliser wm8731_begin()
5. Ajouter les contrôles dans l'interface utilisateur
6. Intégrer avec le séquenceur pour le sampling
7. Tester la compatibilité avec l'architecture existante

PINS UTILISÉES :
- I2C : SDA=21, SCL=22 (contrôle WM8731)
- I2S : BCK=14, WS=15, DOUT=13, DIN=12 (audio bidirectionnel)

FONCTIONNALITÉS AJOUTÉES :
- Audio input (Line/Mic)
- Contrôle gain input (0-31)
- Volume output (48-127)
- Sampling en temps réel
- Enregistrement synchronisé avec séquenceur

COMPATIBILITÉ :
- Maintient l'architecture audio existante
- Compatible avec les 16 voix polyphoniques
- Même fréquence d'échantillonnage (44.1kHz)
- Même format (16-bit stéréo)
*/