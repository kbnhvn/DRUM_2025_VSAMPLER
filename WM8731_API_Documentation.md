# Documentation API WM8731 Driver pour ESP32-S3

## Vue d'ensemble

Le driver WM8731 remplace le PCM5102A dans le projet DRUM_2025_VSAMPLER pour ajouter les fonctionnalités audio bidirectionnelles. Il permet l'enregistrement audio (sampling) tout en maintenant la compatibilité avec l'architecture audio existante.

## Configuration Hardware

### Pins I2S Audio MIKROE-506
- **BCK (Bit Clock)** : GPIO 37
- **WS (Word Select)** : GPIO 15
- **DOUT (Data Output)** : GPIO 16 (vers WM8731)
- **DIN (Data Input)** : GPIO 36 (depuis WM8731)

### Pins I2C Contrôle MIKROE-506
- **SDA** : GPIO 8
- **SCL** : GPIO 4
- **Adresse I2C** : 0x1A

### Spécifications Audio
- **Fréquence d'échantillonnage** : 44.1 kHz
- **Résolution** : 16-bit stéréo
- **Mode** : I2S Master
- **Canaux** : 2 (Stéréo)

## Classes et Fonctions

### Classe WM8731_Manager

#### Constructeur
```cpp
WM8731_Manager()
```
Initialise une instance du manager WM8731.

#### Méthodes Publiques

##### `bool begin()`
Initialise le driver WM8731 et configure l'I2S bidirectionnel.

**Retour :**
- `true` : Initialisation réussie
- `false` : Échec d'initialisation

**Exemple :**
```cpp
if (!wm8731.begin()) {
    Serial.println("Erreur initialisation WM8731");
}
```

##### `void setInputGain(uint8_t gain)`
Configure le gain d'entrée du codec.

**Paramètres :**
- `gain` : Gain 0-31
  - 0 = -34.5 dB
  - 23 = 0 dB (défaut)
  - 31 = +12 dB

**Exemple :**
```cpp
wm8731.setInputGain(25); // +3 dB environ
```

##### `void setOutputVolume(uint8_t volume)`
Configure le volume de sortie du codec.

**Paramètres :**
- `volume` : Volume 48-127
  - 48 = -73 dB
  - 121 = 0 dB (défaut)
  - 127 = +6 dB

**Exemple :**
```cpp
wm8731.setOutputVolume(100); // Volume modéré
```

##### `void enableInput(bool enable)`
Active ou désactive l'entrée audio (ADC).

**Paramètres :**
- `enable` : `true` pour activer, `false` pour désactiver

**Exemple :**
```cpp
wm8731.enableInput(true);  // Activer l'enregistrement
wm8731.enableInput(false); // Désactiver l'enregistrement
```

##### `void enableOutput(bool enable)`
Active ou désactive la sortie audio (DAC).

**Paramètres :**
- `enable` : `true` pour activer, `false` pour désactiver

**Exemple :**
```cpp
wm8731.enableOutput(true); // Activer la lecture
```

##### `void setInputSource(uint8_t source)`
Sélectionne la source d'entrée audio.

**Paramètres :**
- `source` : 
  - `0` = Line Input
  - `1` = Microphone

**Exemple :**
```cpp
wm8731.setInputSource(0); // Entrée ligne
wm8731.setInputSource(1); // Microphone
```

##### `bool isReady()`
Vérifie si le driver est initialisé et prêt.

**Retour :**
- `true` : Driver prêt
- `false` : Driver non initialisé

##### `size_t readAudio(uint8_t* buffer, size_t samples)`
Lit des échantillons audio depuis l'entrée.

**Paramètres :**
- `buffer` : Buffer pour stocker les données
- `samples` : Nombre d'échantillons à lire

**Retour :**
- Nombre d'octets lus

**Exemple :**
```cpp
uint8_t audioBuffer[256];
size_t bytesRead = wm8731.readAudio(audioBuffer, 64);
```

##### Getters
```cpp
uint8_t getInputGain()      // Retourne le gain d'entrée actuel
uint8_t getOutputVolume()   // Retourne le volume de sortie actuel  
uint8_t getInputSource()    // Retourne la source d'entrée actuelle
bool isInputEnabled()       // Retourne l'état de l'entrée
bool isOutputEnabled()      // Retourne l'état de la sortie
```

## API Fonctions Globales

Pour une intégration plus simple avec l'architecture existante, des fonctions globales sont disponibles :

### `bool wm8731_begin()`
Initialise le driver WM8731.

### `void wm8731_setInputGain(uint8_t gain)`
Configure le gain d'entrée (0-31).

### `void wm8731_setOutputVolume(uint8_t volume)`
Configure le volume de sortie (48-127).

### `void wm8731_enableInput(bool enable)`
Active/désactive l'entrée audio.

### `void wm8731_enableOutput(bool enable)`
Active/désactive la sortie audio.

### `void wm8731_setInputSource(uint8_t source)`
Sélectionne la source d'entrée (0=Line, 1=Mic).

### `bool wm8731_isReady()`
Vérifie l'état du driver.

### `size_t wm8731_readAudio(uint8_t* buffer, size_t samples)`
Lit des échantillons audio.

### `void wm8731_testTone()`
Mode de test pour vérifier la sortie audio.

## Intégration avec synthESP32

### Modification de synthESP32_begin()
Remplacer l'initialisation I2S existante :

```cpp
// Ancienne version
void synthESP32_begin() {
    // Configuration des filtres...
    
    // Configuration I2S (à supprimer)
    i2s_config_t i2s_config = { ... };
    // ...
}

// Nouvelle version
void synthESP32_begin_without_i2s() {
    // Configuration des filtres uniquement
    for (int fi=0; fi < 18; fi++) {
        FILTROS[fi].setResonance(reso);
        FILTROS[fi].setCutoffFreq(cutoff);
    }
    
    // Tâche audio (I2S déjà configuré par WM8731)
    xTaskCreatePinnedToCore(audio_task, "audio", 8000, NULL, 
                           configMAX_PRIORITIES - 1, NULL, 0);
}
```

### Séquence d'initialisation dans setup()
```cpp
void setup() {
    // ... autres initialisations ...
    
    // 1. Initialiser WM8731 AVANT synthESP32
    if (!wm8731_begin()) {
        Serial.println("Erreur: WM8731 non initialisé!");
        while(1) delay(1000);
    }
    
    // 2. Configuration initiale
    wm8731_setOutputVolume(100);
    wm8731_setInputGain(20);
    wm8731_setInputSource(0); // Line input
    wm8731_enableOutput(true);
    wm8731_enableInput(false); // Désactivé par défaut
    
    // 3. Initialiser synthESP32 sans I2S
    synthESP32_begin_without_i2s();
    
    // ... suite de setup() ...
}
```

## Gestion des Erreurs

### Codes d'erreur I2C
- **0** : Succès
- **1** : Buffer trop long
- **2** : NACK sur adresse
- **3** : NACK sur données
- **4** : Autres erreurs
- **5** : Timeout

### Diagnostic
```cpp
if (!wm8731_isReady()) {
    Serial.println("WM8731 non prêt");
    // Vérifier les connexions I2C
    // Vérifier l'alimentation du codec
}
```

## Exemples d'Usage

### Enregistrement Simple
```cpp
void recordAudio() {
    wm8731_enableInput(true);
    wm8731_setInputGain(25);
    
    uint8_t buffer[512];
    for (int i = 0; i < 100; i++) {
        size_t bytes = wm8731_readAudio(buffer, 128);
        // Traiter les données...
    }
    
    wm8731_enableInput(false);
}
```

### Contrôle de Volume en Temps Réel
```cpp
void updateVolume(int potValue) {
    uint8_t volume = map(potValue, 0, 1023, 48, 127);
    wm8731_setOutputVolume(volume);
}
```

### Basculement Source d'Entrée
```cpp
void toggleInputSource() {
    static uint8_t source = 0;
    source = (source == 0) ? 1 : 0;
    wm8731_setInputSource(source);
    Serial.printf("Source: %s\n", (source == 0) ? "Line" : "Mic");
}
```

## Performance et Optimisation

### Latence
- **Latence minimale** : ~1.5 ms (64 échantillons @ 44.1kHz)
- **Buffers DMA** : 8 x 64 échantillons
- **Priorité tâche audio** : Maximum (configMAX_PRIORITIES - 1)

### Consommation Mémoire
- **RAM statique** : ~2 KB
- **Buffers DMA** : ~2 KB
- **Stack tâche audio** : 8 KB

### Recommandations
1. **Éviter les appels I2C** fréquents (> 100 Hz)
2. **Utiliser les getters** pour éviter les lectures I2C inutiles
3. **Configurer une seule fois** au démarrage si possible
4. **Désactiver l'entrée** quand elle n'est pas utilisée

## Compatibilité

### ESP32-S3
- ✅ **Compatible** avec ESP32-S3
- ✅ **PSRAM** supporté
- ✅ **Dual Core** optimisé

### Architecture Existante
- ✅ **16 voix polyphoniques** maintenues
- ✅ **Filtres** compatibles
- ✅ **Séquenceur** compatible  
- ✅ **MIDI** compatible
- ✅ **44.1kHz/16-bit** maintenu

### Limitations
- ❌ **Un seul codec** WM8731 supporté
- ❌ **Pas de monitoring direct** (entrée vers sortie)
- ⚠️ **I2C partagé** avec d'autres périphériques

## Dépannage

### Problèmes Fréquents

**Pas de son en sortie :**
1. Vérifier `wm8731_enableOutput(true)`
2. Vérifier le volume : `wm8731_setOutputVolume(80)`
3. Vérifier les connexions I2S

**Pas d'enregistrement :**
1. Vérifier `wm8731_enableInput(true)`
2. Vérifier la source : `wm8731_setInputSource(0)`
3. Vérifier le gain : `wm8731_setInputGain(20)`

**Erreurs I2C :**
1. Vérifier les connexions SDA/SCL
2. Vérifier l'adresse (0x1A)
3. Vérifier l'alimentation 3.3V du codec

### Messages de Debug
Le driver affiche des messages pour le diagnostic :
```
WM8731 Manager prêt
Gain input: 25
Volume output: 100
Source input: Line
Input activé
```

## Notes de Version

### Version 1.0
- ✅ Configuration I2S bidirectionnelle
- ✅ Contrôle I2C complet
- ✅ API simple d'intégration
- ✅ Compatible architecture existante
- ✅ Support Line/Mic input
- ✅ Contrôle gain/volume
- ✅ Fonctions de lecture audio