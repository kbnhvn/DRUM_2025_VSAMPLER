/*
 * DRUM_2025_VSAMPLER - SD Card Module
 * Module de gestion de carte SD pour le chargement dynamique de samples WAV
 * 
 * Fonctionnalités :
 * - Chargement samples WAV depuis SD Card
 * - Cache intelligent LRU pour 8-16 samples
 * - Navigation fichiers et métadonnées
 * - Intégration avec synthESP32.ino
 * - Support mono/stéréo 44.1kHz/16-bit
 */

#include <SD.h>
#include <SPI.h>

// Configuration SPI SD Card (pins configurables)
#define SD_CS_PIN   5
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_SCK_PIN  18

// Configuration cache
#define MAX_CACHED_SAMPLES 16
#define MAX_SAMPLE_SIZE (200 * 1024)  // 200KB max par sample
#define TOTAL_CACHE_SIZE (MAX_CACHED_SAMPLES * MAX_SAMPLE_SIZE)

// Configuration fichiers
#define MAX_FILENAME_LENGTH 64
#define MAX_DISPLAYNAME_LENGTH 32
#define MAX_FILES_LIST 256

// Structure pour les informations d'un sample
struct SampleInfo {
    char filename[MAX_FILENAME_LENGTH];
    char displayName[MAX_DISPLAYNAME_LENGTH];
    uint32_t sampleRate;
    uint16_t duration;      // en millisecondes
    uint8_t channels;       // 1=mono, 2=stéréo
    uint32_t dataSize;      // taille des données audio
    uint32_t fileSize;      // taille totale du fichier
    bool isValid;
};

// Structure pour un sample chargé en cache
struct LoadedSample {
    int16_t* data;
    uint32_t length;        // nombre d'échantillons
    char filename[MAX_FILENAME_LENGTH];
    uint32_t lastUsed;      // timestamp pour LRU
    uint8_t channels;
    uint32_t sampleRate;
    bool isLoaded;
};

// Variables globales
LoadedSample sdCache[MAX_CACHED_SAMPLES];
SampleInfo sdFilesList[MAX_FILES_LIST];
uint16_t sdFilesCount = 0;
uint32_t sdCacheMemoryUsed = 0;
bool sdInitialized = false;
uint32_t sdLastAccessTime = 0;

// Répertoires de samples
const char* sdSampleDirectories[] = {
    "/SAMPLES/KICKS/",
    "/SAMPLES/SNARES/",
    "/SAMPLES/HIHATS/",
    "/SAMPLES/PERCUSSION/",
    "/SAMPLES/SYNTHS/",
    "/SAMPLES/USER/",
    "/SAMPLES/"
};
const uint8_t sdDirectoryCount = sizeof(sdSampleDirectories) / sizeof(sdSampleDirectories[0]);

/*
 * Initialisation du système SD Card
 */
bool sdBegin() {
    Serial.println("Initialisation SD Card...");
    
    // Configuration SPI personnalisée si nécessaire
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Erreur : Impossible d'initialiser la carte SD");
        return false;
    }
    
    // Vérifier le type de carte
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("Erreur : Aucune carte SD détectée");
        return false;
    }
    
    Serial.print("Type de carte SD : ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    
    // Afficher la taille de la carte
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("Taille carte SD : %lluMB\n", cardSize);
    
    // Initialiser le cache
    for (uint8_t i = 0; i < MAX_CACHED_SAMPLES; i++) {
        sdCache[i].data = nullptr;
        sdCache[i].length = 0;
        sdCache[i].lastUsed = 0;
        sdCache[i].isLoaded = false;
        memset(sdCache[i].filename, 0, MAX_FILENAME_LENGTH);
    }
    
    // Scanner les samples disponibles
    sdScanSamples("/SAMPLES/");
    
    sdInitialized = true;
    Serial.println("SD Card initialisée avec succès");
    return true;
}

/*
 * Vérifier si le système SD est prêt
 */
bool sdIsReady() {
    return sdInitialized && (SD.cardType() != CARD_NONE);
}

/*
 * Parser l'en-tête WAV d'un fichier
 */
bool sdParseWavHeader(File &file, SampleInfo &info) {
    if (!file) return false;
    
    file.seek(0);
    
    // Lire l'en-tête WAV (44 bytes minimum)
    uint8_t header[44];
    if (file.read(header, 44) != 44) {
        return false;
    }
    
    // Vérifier la signature RIFF
    if (memcmp(header, "RIFF", 4) != 0) {
        return false;
    }
    
    // Vérifier la signature WAVE
    if (memcmp(header + 8, "WAVE", 4) != 0) {
        return false;
    }
    
    // Vérifier la signature fmt
    if (memcmp(header + 12, "fmt ", 4) != 0) {
        return false;
    }
    
    // Lire les informations du format
    uint16_t audioFormat = *(uint16_t*)(header + 20);
    uint16_t numChannels = *(uint16_t*)(header + 22);
    uint32_t sampleRate = *(uint32_t*)(header + 24);
    uint16_t bitsPerSample = *(uint16_t*)(header + 34);
    
    // Vérifier le format (PCM 16-bit uniquement)
    if (audioFormat != 1 || bitsPerSample != 16) {
        Serial.println("Format WAV non supporté (seul PCM 16-bit est supporté)");
        return false;
    }
    
    // Chercher le chunk "data"
    file.seek(36);
    uint8_t dataChunk[8];
    while (file.available() > 8) {
        if (file.read(dataChunk, 8) != 8) break;
        
        if (memcmp(dataChunk, "data", 4) == 0) {
            uint32_t dataSize = *(uint32_t*)(dataChunk + 4);
            
            // Remplir les informations
            info.sampleRate = sampleRate;
            info.channels = numChannels;
            info.dataSize = dataSize;
            info.fileSize = file.size();
            info.duration = (dataSize / (numChannels * 2)) * 1000 / sampleRate;
            info.isValid = true;
            
            return true;
        } else {
            // Continuer la recherche du chunk data
            uint32_t chunkSize = *(uint32_t*)(dataChunk + 4);
            file.seek(file.position() + chunkSize);
        }
    }
    
    return false;
}

/*
 * Scanner les samples dans un répertoire
 */
uint16_t sdScanSamples(const char* directory) {
    if (!sdIsReady()) return 0;
    
    Serial.printf("Scan des samples dans : %s\n", directory);
    
    File root = SD.open(directory);
    if (!root) {
        Serial.printf("Impossible d'ouvrir le répertoire : %s\n", directory);
        return 0;
    }
    
    if (!root.isDirectory()) {
        root.close();
        return 0;
    }
    
    sdFilesCount = 0;
    
    File file = root.openNextFile();
    while (file && sdFilesCount < MAX_FILES_LIST) {
        if (!file.isDirectory()) {
            String fileName = file.name();
            fileName.toLowerCase();
            
            // Vérifier l'extension .wav
            if (fileName.endsWith(".wav")) {
                SampleInfo &info = sdFilesList[sdFilesCount];
                
                // Copier le nom du fichier
                snprintf(info.filename, MAX_FILENAME_LENGTH, "%s%s", directory, file.name());
                
                // Créer le nom d'affichage (sans extension)
                String displayName = file.name();
                int dotIndex = displayName.lastIndexOf('.');
                if (dotIndex > 0) {
                    displayName = displayName.substring(0, dotIndex);
                }
                strncpy(info.displayName, displayName.c_str(), MAX_DISPLAYNAME_LENGTH - 1);
                info.displayName[MAX_DISPLAYNAME_LENGTH - 1] = '\0';
                
                // Parser l'en-tête WAV
                if (sdParseWavHeader(file, info)) {
                    Serial.printf("Sample trouvé : %s (%dHz, %dch, %dms)\n", 
                                info.displayName, info.sampleRate, info.channels, info.duration);
                    sdFilesCount++;
                } else {
                    Serial.printf("Fichier WAV invalide : %s\n", file.name());
                }
            }
        } else {
            // Récursion dans les sous-répertoires
            String subDir = directory;
            subDir += file.name();
            subDir += "/";
            sdScanSamples(subDir.c_str());
        }
        
        file.close();
        file = root.openNextFile();
    }
    
    root.close();
    Serial.printf("Total samples trouvés : %d\n", sdFilesCount);
    return sdFilesCount;
}

/*
 * Trouver un slot libre dans le cache (LRU)
 */
uint8_t sdFindFreeSlot() {
    // Chercher un slot vide
    for (uint8_t i = 0; i < MAX_CACHED_SAMPLES; i++) {
        if (!sdCache[i].isLoaded) {
            return i;
        }
    }
    
    // Tous les slots sont occupés, trouver le moins récemment utilisé
    uint8_t oldestSlot = 0;
    uint32_t oldestTime = sdCache[0].lastUsed;
    
    for (uint8_t i = 1; i < MAX_CACHED_SAMPLES; i++) {
        if (sdCache[i].lastUsed < oldestTime) {
            oldestTime = sdCache[i].lastUsed;
            oldestSlot = i;
        }
    }
    
    // Libérer le slot le plus ancien
    sdUnloadSample(oldestSlot);
    return oldestSlot;
}

/*
 * Charger un sample depuis la SD vers le cache
 */
bool sdLoadSample(uint8_t slot, const char* filename) {
    if (!sdIsReady() || slot >= MAX_CACHED_SAMPLES) {
        return false;
    }
    
    // Vérifier si le sample est déjà chargé
    for (uint8_t i = 0; i < MAX_CACHED_SAMPLES; i++) {
        if (sdCache[i].isLoaded && strcmp(sdCache[i].filename, filename) == 0) {
            // Sample déjà en cache, mettre à jour le timestamp
            sdCache[i].lastUsed = millis();
            Serial.printf("Sample %s déjà en cache (slot %d)\n", filename, i);
            return true;
        }
    }
    
    File file = SD.open(filename);
    if (!file) {
        Serial.printf("Impossible d'ouvrir le fichier : %s\n", filename);
        return false;
    }
    
    // Parser l'en-tête pour obtenir les informations
    SampleInfo info;
    if (!sdParseWavHeader(file, info)) {
        file.close();
        Serial.printf("Erreur lors du parsing WAV : %s\n", filename);
        return false;
    }
    
    // Vérifier si on a assez de mémoire
    uint32_t sampleMemorySize = info.dataSize;
    if (sampleMemorySize > MAX_SAMPLE_SIZE) {
        file.close();
        Serial.printf("Sample trop volumineux : %s (%d bytes)\n", filename, sampleMemorySize);
        return false;
    }
    
    // Trouver un slot libre
    if (slot == 255) {  // Si slot == 255, auto-assignment
        slot = sdFindFreeSlot();
    } else {
        // Libérer le slot demandé s'il est occupé
        if (sdCache[slot].isLoaded) {
            sdUnloadSample(slot);
        }
    }
    
    // Allouer la mémoire
    sdCache[slot].data = (int16_t*)malloc(sampleMemorySize);
    if (!sdCache[slot].data) {
        file.close();
        Serial.printf("Impossible d'allouer la mémoire pour : %s\n", filename);
        return false;
    }
    
    // Positionner le curseur au début des données audio
    file.seek(44);  // Position standard après l'en-tête WAV
    
    // Chercher le chunk "data" précisément
    uint8_t dataChunk[8];
    while (file.available() > 8) {
        if (file.read(dataChunk, 8) != 8) break;
        
        if (memcmp(dataChunk, "data", 4) == 0) {
            // Trouvé le chunk data, lire les échantillons
            uint32_t bytesRead = file.read((uint8_t*)sdCache[slot].data, info.dataSize);
            
            if (bytesRead != info.dataSize) {
                free(sdCache[slot].data);
                sdCache[slot].data = nullptr;
                file.close();
                Serial.printf("Erreur de lecture des données : %s\n", filename);
                return false;
            }
            break;
        } else {
            // Continuer la recherche
            uint32_t chunkSize = *(uint32_t*)(dataChunk + 4);
            file.seek(file.position() + chunkSize);
        }
    }
    
    file.close();
    
    // Remplir les informations du cache
    sdCache[slot].length = info.dataSize / (info.channels * 2);  // Nombre d'échantillons
    sdCache[slot].channels = info.channels;
    sdCache[slot].sampleRate = info.sampleRate;
    sdCache[slot].lastUsed = millis();
    sdCache[slot].isLoaded = true;
    strncpy(sdCache[slot].filename, filename, MAX_FILENAME_LENGTH - 1);
    sdCache[slot].filename[MAX_FILENAME_LENGTH - 1] = '\0';
    
    sdCacheMemoryUsed += sampleMemorySize;
    
    Serial.printf("Sample chargé : %s -> slot %d (%d échantillons, %d bytes)\n", 
                filename, slot, sdCache[slot].length, sampleMemorySize);
    
    return true;
}

/*
 * Décharger un sample du cache
 */
void sdUnloadSample(uint8_t slot) {
    if (slot >= MAX_CACHED_SAMPLES || !sdCache[slot].isLoaded) {
        return;
    }
    
    if (sdCache[slot].data) {
        free(sdCache[slot].data);
        sdCache[slot].data = nullptr;
    }
    
    Serial.printf("Sample déchargé du slot %d : %s\n", slot, sdCache[slot].filename);
    
    sdCache[slot].length = 0;
    sdCache[slot].lastUsed = 0;
    sdCache[slot].isLoaded = false;
    memset(sdCache[slot].filename, 0, MAX_FILENAME_LENGTH);
}

/*
 * Vérifier si un sample est chargé
 */
bool sdIsSampleLoaded(uint8_t slot) {
    if (slot >= MAX_CACHED_SAMPLES) return false;
    return sdCache[slot].isLoaded;
}

/*
 * Obtenir les données d'un sample chargé
 */
int16_t* sdGetSampleData(uint8_t slot) {
    if (slot >= MAX_CACHED_SAMPLES || !sdCache[slot].isLoaded) {
        return nullptr;
    }
    
    sdCache[slot].lastUsed = millis();  // Mettre à jour LRU
    return sdCache[slot].data;
}

/*
 * Obtenir la longueur d'un sample chargé
 */
uint32_t sdGetSampleLength(uint8_t slot) {
    if (slot >= MAX_CACHED_SAMPLES || !sdCache[slot].isLoaded) {
        return 0;
    }
    
    sdCache[slot].lastUsed = millis();  // Mettre à jour LRU
    return sdCache[slot].length;
}

/*
 * Obtenir le nombre de canaux d'un sample
 */
uint8_t sdGetSampleChannels(uint8_t slot) {
    if (slot >= MAX_CACHED_SAMPLES || !sdCache[slot].isLoaded) {
        return 1;  // Défaut mono
    }
    
    return sdCache[slot].channels;
}

/*
 * Obtenir le sample rate d'un sample
 */
uint32_t sdGetSampleRate(uint8_t slot) {
    if (slot >= MAX_CACHED_SAMPLES || !sdCache[slot].isLoaded) {
        return 44100;  // Défaut 44.1kHz
    }
    
    return sdCache[slot].sampleRate;
}

/*
 * Obtenir le nom d'un sample dans la liste
 */
const char* sdGetSampleName(uint16_t index) {
    if (index >= sdFilesCount) return nullptr;
    return sdFilesList[index].displayName;
}

/*
 * Obtenir le chemin d'un sample dans la liste
 */
const char* sdGetSamplePath(uint16_t index) {
    if (index >= sdFilesCount) return nullptr;
    return sdFilesList[index].filename;
}

/*
 * Obtenir les informations d'un sample dans la liste
 */
const SampleInfo* sdGetSampleInfo(uint16_t index) {
    if (index >= sdFilesCount) return nullptr;
    return &sdFilesList[index];
}

/*
 * Vider le cache
 */
void sdClearCache() {
    for (uint8_t i = 0; i < MAX_CACHED_SAMPLES; i++) {
        sdUnloadSample(i);
    }
    sdCacheMemoryUsed = 0;
    Serial.println("Cache SD vidé");
}

/*
 * Obtenir l'utilisation du cache (0-100%)
 */
uint8_t sdGetCacheUsage() {
    uint8_t slotsUsed = 0;
    for (uint8_t i = 0; i < MAX_CACHED_SAMPLES; i++) {
        if (sdCache[i].isLoaded) {
            slotsUsed++;
        }
    }
    return (slotsUsed * 100) / MAX_CACHED_SAMPLES;
}

/*
 * Obtenir l'utilisation mémoire du cache
 */
uint32_t sdGetMemoryUsage() {
    return sdCacheMemoryUsed;
}

/*
 * Trouver un sample par nom de fichier
 */
int16_t sdFindSampleByName(const char* filename) {
    for (uint16_t i = 0; i < sdFilesCount; i++) {
        if (strcmp(sdFilesList[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}

/*
 * Charger un sample par son index dans la liste
 */
bool sdLoadSampleByIndex(uint8_t slot, uint16_t index) {
    if (index >= sdFilesCount) return false;
    return sdLoadSample(slot, sdFilesList[index].filename);
}

/*
 * Obtenir le nombre total de samples disponibles
 */
uint16_t sdGetSampleCount() {
    return sdFilesCount;
}

/*
 * Afficher les statistiques du cache
 */
void sdPrintCacheStats() {
    Serial.println("=== STATISTIQUES CACHE SD ===");
    Serial.printf("Slots utilisés : %d/%d (%d%%)\n", 
                  sdGetCacheUsage() * MAX_CACHED_SAMPLES / 100, 
                  MAX_CACHED_SAMPLES, 
                  sdGetCacheUsage());
    Serial.printf("Mémoire utilisée : %d bytes\n", sdCacheMemoryUsed);
    Serial.printf("Samples disponibles : %d\n", sdFilesCount);
    
    Serial.println("Samples en cache :");
    for (uint8_t i = 0; i < MAX_CACHED_SAMPLES; i++) {
        if (sdCache[i].isLoaded) {
            Serial.printf("  Slot %d : %s (%d échantillons)\n", 
                        i, sdCache[i].filename, sdCache[i].length);
        }
    }
    Serial.println("============================");
}

/*
 * Test de performance du système SD
 */
void sdPerformanceTest() {
    if (!sdIsReady()) {
        Serial.println("SD non initialisée pour le test de performance");
        return;
    }
    
    Serial.println("=== TEST DE PERFORMANCE SD ===");
    
    if (sdFilesCount == 0) {
        Serial.println("Aucun sample disponible pour le test");
        return;
    }
    
    // Test de chargement
    uint32_t startTime = millis();
    bool loadResult = sdLoadSampleByIndex(0, 0);  // Charger le premier sample
    uint32_t loadTime = millis() - startTime;
    
    if (loadResult) {
        Serial.printf("Chargement du sample : %d ms\n", loadTime);
        Serial.printf("Sample chargé : %s (%d échantillons)\n", 
                    sdCache[0].filename, sdCache[0].length);
    } else {
        Serial.println("Échec du chargement du sample");
    }
    
    Serial.println("=============================");
}