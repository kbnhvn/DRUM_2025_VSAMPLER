# DRUM_2025_VSAMPLER - Documentation Système SD Card

## Vue d'ensemble

Le système SD Card du DRUM_2025_VSAMPLER permet le chargement dynamique de samples WAV depuis une carte SD, remplaçant ou complétant les samples intégrés en mémoire flash. Ce système offre une flexibilité considérable pour étendre la banque de sons.

## Architecture du Système

### Composants Principaux

1. **Module `sd.ino`** - Gestion complète de la carte SD
2. **Intégration `synthESP32.ino`** - Système hybride samples intégrés/SD  
3. **Interface `DRUM_2025_VSAMPLER.ino`** - Contrôles utilisateur

### Configuration Hardware

```cpp
// Pins SPI SD Card (configurables dans sd.ino)
#define SD_CS_PIN   5     // Chip Select
#define SD_MOSI_PIN 23    // Master Out Slave In  
#define SD_MISO_PIN 19    // Master In Slave Out
#define SD_SCK_PIN  18    // Serial Clock
```

## Structure des Fichiers sur SD Card

### Répertoire Recommandé

```
/SAMPLES/
├── KICKS/          # Samples de kicks/grosse caisse
├── SNARES/         # Samples de snares/caisse claire  
├── HIHATS/         # Samples de hi-hats/charleston
├── PERCUSSION/     # Autres percussions
├── SYNTHS/         # Samples synthétiques
└── USER/           # Samples utilisateur
```

### Format de Fichiers Supporté

- **Format** : WAV (PCM 16-bit uniquement)
- **Sample Rate** : 44.1 kHz recommandé
- **Canaux** : Mono ou stéréo
- **Taille max** : 200KB par sample
- **Noms** : Caractères alphanumériques, éviter les caractères spéciaux

## API du Module SD

### Fonctions d'Initialisation

```cpp
bool sdBegin()                    // Initialiser le système SD
bool sdIsReady()                  // Vérifier si SD prête
uint16_t sdScanSamples(const char* directory)  // Scanner samples
```

### Gestion du Cache

```cpp
bool sdLoadSample(uint8_t slot, const char* filename)  // Charger sample
void sdUnloadSample(uint8_t slot)                      // Décharger sample
bool sdIsSampleLoaded(uint8_t slot)                    // Vérifier si chargé
void sdClearCache()                                    // Vider cache
uint8_t sdGetCacheUsage()                              // Utilisation cache (%)
```

### Accès aux Données

```cpp
int16_t* sdGetSampleData(uint8_t slot)        // Données audio
uint32_t sdGetSampleLength(uint8_t slot)      // Longueur en échantillons  
uint8_t sdGetSampleChannels(uint8_t slot)     // Nombre de canaux
uint32_t sdGetSampleRate(uint8_t slot)        // Fréquence d'échantillonnage
```

### Navigation des Fichiers

```cpp
uint16_t sdGetSampleCount()                   // Nombre total de samples
const char* sdGetSampleName(uint16_t index)   // Nom d'affichage
const char* sdGetSamplePath(uint16_t index)   // Chemin complet
const SampleInfo* sdGetSampleInfo(uint16_t index)  // Informations complètes
```

## Système Hybride

### Sources de Samples

Le système supporte deux sources :

1. **SAMPLE_SOURCE_BUILTIN** - Samples intégrés (mémoire flash)
2. **SAMPLE_SOURCE_SD** - Samples chargés depuis SD Card

### Assignation des Voix

```cpp
// Assigner un sample SD à une voix
bool assignSampleToVoice(uint8_t voice, SampleSource source, uint8_t sampleIndex);

// Exemples
assignSampleToVoice(0, SAMPLE_SOURCE_BUILTIN, 16);  // Sample intégré #16 → voix 0
assignSampleToVoice(1, SAMPLE_SOURCE_SD, 2);        // Sample SD slot #2 → voix 1
```

## Interface Utilisateur

### Modes de Fonctionnement

1. **Mode 0** - Samples intégrés uniquement
2. **Mode 1** - Samples SD uniquement  
3. **Mode 2** - Mode hybride (intégrés + SD)

### Commandes Tactiles

- **SD BROWSE** - Activer/désactiver navigateur de samples
- **SD LOAD** - Charger sample sélectionné sur voix active
- Navigation : Utiliser encodeur rotatif pour parcourir les samples

### Fonctions de Test

```cpp
void sdPerformanceTest()          // Test de performance
void sdPrintCacheStats()          // Afficher statistiques
void sdTestLoadMultipleSamples()  // Test chargement multiple
void sdToggleMode()               // Basculer entre modes
```

## Gestion Mémoire

### Cache LRU (Least Recently Used)

- **Slots disponibles** : 16 maximum
- **Mémoire par sample** : 200KB maximum
- **Mémoire totale cache** : ~3.2MB maximum
- **Stratégie** : Éviction automatique des samples les moins utilisés

### Optimisation

- Les samples sont chargés à la demande
- Cache intelligent avec gestion LRU
- Conversion automatique stéréo → mono si nécessaire
- Protection contre le dépassement mémoire

## Utilisation Pratique

### 1. Préparation de la SD Card

```bash
# Créer la structure de répertoires
mkdir -p /SAMPLES/{KICKS,SNARES,HIHATS,PERCUSSION,SYNTHS,USER}

# Copier les fichiers WAV dans les répertoires appropriés
cp kick01.wav /SAMPLES/KICKS/
cp snare01.wav /SAMPLES/SNARES/
# etc.
```

### 2. Initialisation

Le système s'initialise automatiquement au démarrage :

```cpp
void setup() {
    // ... autres initialisations ...
    
    if (sdBegin()) {
        Serial.println("Système SD prêt");
        sdScanSamples("/SAMPLES/");
    }
}
```

### 3. Chargement Manuel

```cpp
// Charger un sample spécifique
if (sdLoadSample(0, "/SAMPLES/KICKS/kick01.wav")) {
    // Assigner à la voix 0
    assignSampleToVoice(0, SAMPLE_SOURCE_SD, 0);
    setSound(0);  // Mettre à jour les paramètres
}
```

### 4. Navigation Interactive

```cpp
void loop() {
    // Mise à jour du navigateur SD
    sdBrowserUpdate();
    
    // Gestion des commandes utilisateur
    if (touchDetected) {
        handleSDCommands(touchCommand);
    }
}
```

## Diagnostic et Débogage

### Messages de Debug

Le système affiche des informations détaillées via Serial :

```
=== INITIALISATION SYSTÈME SD ===
Type de carte SD : SDHC
Taille carte SD : 16000MB
Scan des samples dans : /SAMPLES/
Sample trouvé : kick01 (44100Hz, 1ch, 250ms)
Sample trouvé : snare01 (44100Hz, 2ch, 180ms)
Total samples trouvés : 2
Système SD initialisé avec succès
==============================
```

### Statistiques du Cache

```
=== STATISTIQUES CACHE SD ===
Slots utilisés : 3/16 (18%)
Mémoire utilisée : 598400 bytes
Samples disponibles : 12
Samples en cache :
  Slot 0 : /SAMPLES/KICKS/kick01.wav (11025 échantillons)
  Slot 1 : /SAMPLES/SNARES/snare01.wav (7938 échantillons)
  Slot 2 : /SAMPLES/HIHATS/hat01.wav (4410 échantillons)
============================
```

### Codes d'Erreur Communs

- **"SD non initialisée"** - Carte SD absente ou défaillante
- **"Format WAV non supporté"** - Fichier non PCM 16-bit
- **"Sample trop volumineux"** - Fichier > 200KB
- **"Impossible d'allouer la mémoire"** - RAM insuffisante
- **"Erreur de lecture des données"** - Corruption fichier

## Performance

### Benchmarks Typiques

- **Chargement sample 100KB** : ~50-150ms
- **Accès échantillon cache** : <1μs
- **Latence audio** : Identique aux samples intégrés
- **Mémoire par sample** : Taille fichier + overhead minimal

### Recommandations

1. **Pré-charger** les samples fréquemment utilisés
2. **Organiser** les fichiers par catégorie dans des répertoires
3. **Optimiser** la taille des samples (mono vs stéréo)
4. **Surveiller** l'utilisation du cache pour éviter les évictions

## Évolutions Futures

### Fonctionnalités Prévues

- [ ] Support formats audio additionnels (FLAC, MP3)
- [ ] Compression temps réel des samples
- [ ] Sauvegarde des configurations sur SD
- [ ] Interface graphique pour la navigation
- [ ] Chargement par streaming pour gros samples
- [ ] Métadonnées étendues (BPM, clé, tags)

### Optimisations Possibles

- [ ] Cache prédictif basé sur les patterns
- [ ] Compression adaptative selon la RAM disponible
- [ ] Chargement en arrière-plan pendant la lecture
- [ ] Support hotswap de cartes SD

## Résolution de Problèmes

### SD Card non détectée

1. Vérifier les connexions SPI
2. Tester avec une autre carte SD (préférer Class 10)
3. Formater en FAT32
4. Vérifier l'alimentation (certaines cartes consomment plus)

### Samples non chargés

1. Vérifier le format WAV (doit être PCM 16-bit)
2. Contrôler la taille du fichier (<200KB)
3. Vérifier les noms de fichiers (pas de caractères spéciaux)
4. S'assurer de la structure de répertoires

### Performance dégradée

1. Monitorer l'utilisation du cache
2. Éviter les samples trop volumineux
3. Préférer le mode mono si possible
4. Vider le cache périodiquement si nécessaire

---

**Version** : 1.0  
**Date** : 2025-01-23  
**Auteur** : Système SD Card DRUM_2025_VSAMPLER