////////////////////////////////////////////////////////////////////////////////////////////
//                      DRUM_2025_VSAMPLER - MONITORING DE PERFORMANCE                   //
////////////////////////////////////////////////////////////////////////////////////////////
// Module de monitoring temps réel des performances système
// Surveillance continue de l'audio, mémoire, UI, connectivité et stabilité
// 
// Auteur: ZCARLOS 2025
// Version: 1.0
// 
// Fonctionnalités :
// - Monitoring audio (latence, polyphonie, qualité)
// - Surveillance mémoire (heap, fragmentation, cache)
// - Métriques interface (réactivité tactile, transitions)
// - État connectivité (WiFi, MIDI, SD Card)
// - Alertes et historiques de performance
// - Affichage temps réel sur écran et Serial

#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>

////////////////////////////////////////////////////////////////////////////////////////////
//                                CONFIGURATION MONITORING                               //
////////////////////////////////////////////////////////////////////////////////////////////

// Intervalles de mise à jour (ms)
#define MONITOR_UPDATE_FAST_MS          100    // Métriques audio critiques
#define MONITOR_UPDATE_NORMAL_MS        500    // Métriques système générales
#define MONITOR_UPDATE_SLOW_MS         5000    // Métriques de tendance
#define MONITOR_DISPLAY_UPDATE_MS      1000    // Mise à jour affichage
#define MONITOR_HISTORY_INTERVAL_MS   60000    // Sauvegarde historique

// Seuils d'alerte
#define ALERT_AUDIO_LATENCY_MS           15    // Seuil alerte latence audio
#define ALERT_HEAP_LOW_KB                50    // Seuil heap critique
#define ALERT_FRAGMENTATION_HIGH         30    // Seuil fragmentation critique (%)
#define ALERT_TOUCH_RESPONSE_MS          80    // Seuil réponse tactile lente
#define ALERT_CPU_USAGE_HIGH             85    // Seuil CPU critique (%)
#define ALERT_TEMP_HIGH_C                75    // Seuil température critique

// Taille des historiques
#define HISTORY_BUFFER_SIZE              60    // 60 points pour 1h à 1 min/point
#define ALERT_HISTORY_SIZE               20    // Historique des 20 dernières alertes

// Configuration affichage
#define MONITOR_DISPLAY_X                10
#define MONITOR_DISPLAY_Y               280
#define MONITOR_DISPLAY_WIDTH           460
#define MONITOR_DISPLAY_HEIGHT           40

////////////////////////////////////////////////////////////////////////////////////////////
//                              STRUCTURES DE MONITORING                                 //
////////////////////////////////////////////////////////////////////////////////////////////

// Métriques audio en temps réel
struct AudioMetrics {
    uint32_t latencyUs;             // Latence actuelle en microsecondes
    uint8_t activeVoices;           // Nombre de voix actives
    uint8_t maxVoices;              // Maximum de voix atteint
    float cpuUsageAudio;            // Usage CPU pour l'audio (%)
    uint16_t sampleRate;            // Fréquence d'échantillonnage actuelle
    uint8_t bitDepth;               // Profondeur de bits
    bool audioStable;               // État de stabilité audio
    uint32_t underruns;             // Nombre d'underruns détectés
    uint32_t overruns;              // Nombre d'overruns détectés
    uint32_t lastAudioUpdate;       // Timestamp dernière mise à jour
};

// Métriques mémoire
struct MemoryMetrics {
    uint32_t heapFree;              // Heap libre (bytes)
    uint32_t heapTotal;             // Heap total (bytes)
    uint32_t heapMin;               // Heap minimum observé
    uint8_t fragmentation;          // Fragmentation mémoire (%)
    uint32_t largestBlock;          // Plus grand bloc contigu
    uint32_t psramFree;             // PSRAM libre (si disponible)
    uint8_t cacheUsage;             // Usage cache SD (%)
    uint8_t cacheHitRate;           // Taux de succès cache (%)
    uint32_t lastMemoryUpdate;      // Timestamp dernière mise à jour
};

// Métriques interface utilisateur
struct UIMetrics {
    uint32_t touchResponseTime;     // Temps de réponse tactile (ms)
    uint32_t maxTouchResponse;      // Temps de réponse max observé
    uint8_t touchErrorRate;         // Taux d'erreur tactile (%)
    uint32_t appSwitchTime;         // Temps commutation application (ms)
    uint8_t displayFPS;             // FPS d'affichage estimé
    bool touchCalibrated;           // État calibration tactile
    uint8_t uiResponsiveness;       // Score réactivité UI (0-100)
    uint32_t lastUIUpdate;          // Timestamp dernière mise à jour
};

// Métriques connectivité
struct ConnectivityMetrics {
    bool wifiConnected;             // État connexion WiFi
    int8_t wifiRSSI;                // Signal WiFi (dBm)
    uint32_t wifiUptime;            // Durée connexion WiFi (ms)
    uint8_t wifiReconnects;         // Nombre de reconnexions
    bool sdCardPresent;             // Présence carte SD
    uint32_t sdReadSpeed;           // Vitesse lecture SD (KB/s)
    uint32_t sdWriteSpeed;          // Vitesse écriture SD (KB/s)
    bool midiConnected;             // État connexion MIDI
    uint16_t midiMessages;          // Messages MIDI reçus
    uint32_t lastConnUpdate;        // Timestamp dernière mise à jour
};

// Métriques système générales
struct SystemMetrics {
    float cpuUsage;                 // Usage CPU global (%)
    float temperature;              // Température CPU (°C)
    uint32_t uptime;                // Durée de fonctionnement (ms)
    uint32_t freeStack;             // Stack libre (bytes)
    uint8_t systemStability;        // Score stabilité (0-100)
    uint32_t errorCount;            // Nombre d'erreurs détectées
    uint32_t warningCount;          // Nombre d'avertissements
    bool systemHealthy;             // État général du système
    uint32_t lastSystemUpdate;      // Timestamp dernière mise à jour
};

// Point d'historique
struct HistoryPoint {
    uint32_t timestamp;
    uint8_t cpuUsage;
    uint16_t heapFree;              // en KB
    uint8_t activeVoices;
    uint8_t systemStability;
    uint16_t latencyUs;
};

// Alerte système
struct SystemAlert {
    uint32_t timestamp;
    uint8_t severity;               // 0=Info, 1=Warning, 2=Error, 3=Critical
    uint8_t category;               // 0=Audio, 1=Memory, 2=UI, 3=Connectivity, 4=System
    char message[64];
    bool acknowledged;
};

////////////////////////////////////////////////////////////////////////////////////////////
//                              VARIABLES GLOBALES                                       //
////////////////////////////////////////////////////////////////////////////////////////////

// Métriques temps réel
AudioMetrics audioMetrics;
MemoryMetrics memoryMetrics;
UIMetrics uiMetrics;
ConnectivityMetrics connectivityMetrics;
SystemMetrics systemMetrics;

// Historiques de performance
HistoryPoint performanceHistory[HISTORY_BUFFER_SIZE];
uint8_t historyIndex = 0;
bool historyFull = false;

// Système d'alertes
SystemAlert alertHistory[ALERT_HISTORY_SIZE];
uint8_t alertIndex = 0;
uint8_t activeAlerts = 0;
bool alertsEnabled = true;

// État du monitoring
bool monitoringActive = false;
bool displayEnabled = true;
bool loggingEnabled = true;
uint32_t lastDisplayUpdate = 0;
uint32_t lastHistorySave = 0;

// Timers de mise à jour
uint32_t lastFastUpdate = 0;
uint32_t lastNormalUpdate = 0;
uint32_t lastSlowUpdate = 0;

// Configuration d'affichage
enum DisplayMode {
    DISPLAY_OVERVIEW = 0,
    DISPLAY_AUDIO_DETAIL,
    DISPLAY_MEMORY_DETAIL,
    DISPLAY_CONNECTIVITY_DETAIL,
    DISPLAY_ALERTS,
    DISPLAY_HISTORY
};

DisplayMode currentDisplayMode = DISPLAY_OVERVIEW;
uint8_t displayRotation = 0;

// Callbacks pour intégration
void (*onAlertTriggered)(SystemAlert* alert) = nullptr;
void (*onMetricsUpdated)(void) = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////
//                              INITIALISATION MONITORING                                //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Initialisation du système de monitoring
 */
bool performanceMonitorBegin() {
    if (monitoringActive) {
        Serial.println("Monitoring déjà actif");
        return true;
    }
    
    Serial.println("\n📊 DRUM_2025_VSAMPLER - MONITORING PERFORMANCE");
    Serial.println("===============================================");
    
    // Initialiser les structures de métriques
    memset(&audioMetrics, 0, sizeof(AudioMetrics));
    memset(&memoryMetrics, 0, sizeof(MemoryMetrics));
    memset(&uiMetrics, 0, sizeof(UIMetrics));
    memset(&connectivityMetrics, 0, sizeof(ConnectivityMetrics));
    memset(&systemMetrics, 0, sizeof(SystemMetrics));
    
    // Initialiser les historiques
    memset(performanceHistory, 0, sizeof(performanceHistory));
    memset(alertHistory, 0, sizeof(alertHistory));
    historyIndex = 0;
    historyFull = false;
    alertIndex = 0;
    activeAlerts = 0;
    
    // Configuration initiale
    audioMetrics.sampleRate = SAMPLE_RATE;
    audioMetrics.bitDepth = 16;
    memoryMetrics.heapTotal = ESP.getHeapSize();
    memoryMetrics.heapMin = ESP.getFreeHeap();
    systemMetrics.uptime = millis();
    
    // Première mesure de référence
    updateAudioMetrics();
    updateMemoryMetrics();
    updateConnectivityMetrics();
    updateSystemMetrics();
    
    monitoringActive = true;
    Serial.println("✅ Monitoring de performance initialisé");
    
    return true;
}

/**
 * Arrêt du monitoring
 */
void performanceMonitorEnd() {
    if (!monitoringActive) return;
    
    monitoringActive = false;
    Serial.println("📊 Monitoring de performance arrêté");
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              MISE À JOUR DES MÉTRIQUES                               //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Mise à jour principale du monitoring
 */
void performanceMonitorUpdate() {
    if (!monitoringActive) return;
    
    uint32_t currentTime = millis();
    
    // Mise à jour rapide (métriques audio critiques)
    if ((currentTime - lastFastUpdate) >= MONITOR_UPDATE_FAST_MS) {
        updateAudioMetrics();
        lastFastUpdate = currentTime;
    }
    
    // Mise à jour normale (métriques système)
    if ((currentTime - lastNormalUpdate) >= MONITOR_UPDATE_NORMAL_MS) {
        updateMemoryMetrics();
        updateUIMetrics();
        updateSystemMetrics();
        checkAlerts();
        lastNormalUpdate = currentTime;
    }
    
    // Mise à jour lente (tendances)
    if ((currentTime - lastSlowUpdate) >= MONITOR_UPDATE_SLOW_MS) {
        updateConnectivityMetrics();
        calculateTrends();
        lastSlowUpdate = currentTime;
    }
    
    // Sauvegarde historique
    if ((currentTime - lastHistorySave) >= MONITOR_HISTORY_INTERVAL_MS) {
        saveHistoryPoint();
        lastHistorySave = currentTime;
    }
    
    // Mise à jour affichage
    if (displayEnabled && (currentTime - lastDisplayUpdate) >= MONITOR_DISPLAY_UPDATE_MS) {
        updateDisplay();
        lastDisplayUpdate = currentTime;
    }
    
    // Callback de mise à jour
    if (onMetricsUpdated) {
        onMetricsUpdated();
    }
}

/**
 * Mise à jour des métriques audio
 */
void updateAudioMetrics() {
    uint32_t currentTime = millis();
    
    // Compter les voix actives
    audioMetrics.activeVoices = 0;
    for (uint8_t i = 0; i < 16; i++) {
        if (latch[i]) audioMetrics.activeVoices++;
    }
    
    // Mettre à jour le maximum
    if (audioMetrics.activeVoices > audioMetrics.maxVoices) {
        audioMetrics.maxVoices = audioMetrics.activeVoices;
    }
    
    // Test de latence (approximatif)
    if (audioMetrics.activeVoices > 0) {
        uint32_t startTime = esp_timer_get_time();
        // Simulation de mesure latence
        audioMetrics.latencyUs = esp_timer_get_time() - startTime + 2000; // ~2ms base
    }
    
    // Estimation usage CPU audio (basé sur polyphonie)
    audioMetrics.cpuUsageAudio = (audioMetrics.activeVoices * 100.0) / 16.0;
    
    // Stabilité audio
    audioMetrics.audioStable = (audioMetrics.activeVoices <= 16) && 
                               (audioMetrics.latencyUs < (ALERT_AUDIO_LATENCY_MS * 1000));
    
    audioMetrics.lastAudioUpdate = currentTime;
}

/**
 * Mise à jour des métriques mémoire
 */
void updateMemoryMetrics() {
    uint32_t currentTime = millis();
    
    // Métriques heap
    memoryMetrics.heapFree = ESP.getFreeHeap();
    memoryMetrics.largestBlock = ESP.getMaxAllocHeap();
    
    // Minimum observé
    if (memoryMetrics.heapFree < memoryMetrics.heapMin) {
        memoryMetrics.heapMin = memoryMetrics.heapFree;
    }
    
    // Calcul fragmentation
    if (memoryMetrics.heapFree > 0) {
        memoryMetrics.fragmentation = 100 - ((memoryMetrics.largestBlock * 100) / memoryMetrics.heapFree);
    }
    
    // PSRAM si disponible
    memoryMetrics.psramFree = ESP.getFreePsram();
    
    // Métriques cache SD
    if (sdIsReady()) {
        memoryMetrics.cacheUsage = sdGetCacheUsage();
        memoryMetrics.cacheHitRate = 95; // Valeur estimée, devrait venir du module SD
    }
    
    memoryMetrics.lastMemoryUpdate = currentTime;
}

/**
 * Mise à jour des métriques UI
 */
void updateUIMetrics() {
    uint32_t currentTime = millis();
    
    // Simulation de mesure réactivité tactile
    // Dans une implémentation réelle, ceci serait intégré au système tactile
    uiMetrics.touchResponseTime = 25; // ms simulé
    
    if (uiMetrics.touchResponseTime > uiMetrics.maxTouchResponse) {
        uiMetrics.maxTouchResponse = uiMetrics.touchResponseTime;
    }
    
    // Score de réactivité UI
    if (uiMetrics.touchResponseTime <= 30) {
        uiMetrics.uiResponsiveness = 100;
    } else if (uiMetrics.touchResponseTime <= 50) {
        uiMetrics.uiResponsiveness = 80;
    } else if (uiMetrics.touchResponseTime <= 80) {
        uiMetrics.uiResponsiveness = 60;
    } else {
        uiMetrics.uiResponsiveness = 40;
    }
    
    // Estimation FPS
    uiMetrics.displayFPS = 30; // Valeur nominale
    
    // Calibration tactile
    uiMetrics.touchCalibrated = true;
    
    uiMetrics.lastUIUpdate = currentTime;
}

/**
 * Mise à jour des métriques de connectivité
 */
void updateConnectivityMetrics() {
    uint32_t currentTime = millis();
    
    // WiFi
    connectivityMetrics.wifiConnected = isWifiManagerActive();
    if (connectivityMetrics.wifiConnected) {
        connectivityMetrics.wifiRSSI = -45; // dBm simulé
        connectivityMetrics.wifiUptime = currentTime - systemMetrics.uptime;
    }
    
    // SD Card
    connectivityMetrics.sdCardPresent = sdIsReady();
    if (connectivityMetrics.sdCardPresent) {
        connectivityMetrics.sdReadSpeed = 1500; // KB/s simulé
        connectivityMetrics.sdWriteSpeed = 800;  // KB/s simulé
    }
    
    // MIDI
    connectivityMetrics.midiConnected = true; // MIDI hardware toujours présent
    
    connectivityMetrics.lastConnUpdate = currentTime;
}

/**
 * Mise à jour des métriques système
 */
void updateSystemMetrics() {
    uint32_t currentTime = millis();
    
    // Uptime
    systemMetrics.uptime = currentTime;
    
    // Estimation usage CPU global
    systemMetrics.cpuUsage = audioMetrics.cpuUsageAudio + 
                             (connectivityMetrics.wifiConnected ? 15.0 : 0.0) +
                             (memoryMetrics.fragmentation > 20 ? 10.0 : 5.0);
    
    // Température simulée (basée sur usage CPU)
    systemMetrics.temperature = 40.0 + (systemMetrics.cpuUsage * 0.3);
    
    // Stack libre (approximation)
    systemMetrics.freeStack = 8192; // Valeur typique
    
    // Score de stabilité système
    uint8_t stabilityScore = 100;
    if (!audioMetrics.audioStable) stabilityScore -= 20;
    if (memoryMetrics.heapFree < (100 * 1024)) stabilityScore -= 15;
    if (systemMetrics.cpuUsage > 80) stabilityScore -= 15;
    if (systemMetrics.temperature > 70) stabilityScore -= 10;
    if (activeAlerts > 0) stabilityScore -= (activeAlerts * 5);
    
    systemMetrics.systemStability = max(0, stabilityScore);
    
    // État de santé général
    systemMetrics.systemHealthy = (systemMetrics.systemStability >= 70) &&
                                   (memoryMetrics.heapFree > (50 * 1024)) &&
                                   audioMetrics.audioStable;
    
    systemMetrics.lastSystemUpdate = currentTime;
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              SYSTÈME D'ALERTES                                        //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Vérification et déclenchement des alertes
 */
void checkAlerts() {
    if (!alertsEnabled) return;
    
    // Alertes audio
    if (audioMetrics.latencyUs > (ALERT_AUDIO_LATENCY_MS * 1000)) {
        triggerAlert(2, 0, "Latence audio élevée");
    }
    
    if (audioMetrics.activeVoices > 14) {
        triggerAlert(1, 0, "Polyphonie proche du maximum");
    }
    
    // Alertes mémoire
    if (memoryMetrics.heapFree < (ALERT_HEAP_LOW_KB * 1024)) {
        triggerAlert(3, 1, "Heap critique");
    }
    
    if (memoryMetrics.fragmentation > ALERT_FRAGMENTATION_HIGH) {
        triggerAlert(2, 1, "Fragmentation mémoire élevée");
    }
    
    // Alertes UI
    if (uiMetrics.touchResponseTime > ALERT_TOUCH_RESPONSE_MS) {
        triggerAlert(1, 2, "Réponse tactile lente");
    }
    
    // Alertes système
    if (systemMetrics.cpuUsage > ALERT_CPU_USAGE_HIGH) {
        triggerAlert(2, 4, "Usage CPU élevé");
    }
    
    if (systemMetrics.temperature > ALERT_TEMP_HIGH_C) {
        triggerAlert(3, 4, "Température critique");
    }
    
    // Alertes connectivité
    if (!connectivityMetrics.sdCardPresent) {
        triggerAlert(1, 3, "SD Card déconnectée");
    }
}

/**
 * Déclencher une alerte
 */
void triggerAlert(uint8_t severity, uint8_t category, const char* message) {
    SystemAlert* alert = &alertHistory[alertIndex];
    
    alert->timestamp = millis();
    alert->severity = severity;
    alert->category = category;
    strncpy(alert->message, message, sizeof(alert->message) - 1);
    alert->message[sizeof(alert->message) - 1] = '\0';
    alert->acknowledged = false;
    
    alertIndex = (alertIndex + 1) % ALERT_HISTORY_SIZE;
    if (activeAlerts < ALERT_HISTORY_SIZE) {
        activeAlerts++;
    }
    
    // Log de l'alerte
    const char* severityStr[] = {"INFO", "WARN", "ERROR", "CRIT"};
    const char* categoryStr[] = {"AUDIO", "MEMORY", "UI", "CONN", "SYSTEM"};
    
    Serial.printf("🚨 [%s-%s] %s\n", severityStr[severity], categoryStr[category], message);
    
    // Callback d'alerte
    if (onAlertTriggered) {
        onAlertTriggered(alert);
    }
}

/**
 * Acquitter toutes les alertes
 */
void acknowledgeAllAlerts() {
    for (uint8_t i = 0; i < ALERT_HISTORY_SIZE; i++) {
        alertHistory[i].acknowledged = true;
    }
    activeAlerts = 0;
    Serial.println("📋 Toutes les alertes acquittées");
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              HISTORIQUES ET TENDANCES                                 //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Sauvegarder un point d'historique
 */
void saveHistoryPoint() {
    HistoryPoint* point = &performanceHistory[historyIndex];
    
    point->timestamp = millis();
    point->cpuUsage = (uint8_t)systemMetrics.cpuUsage;
    point->heapFree = memoryMetrics.heapFree / 1024; // en KB
    point->activeVoices = audioMetrics.activeVoices;
    point->systemStability = systemMetrics.systemStability;
    point->latencyUs = audioMetrics.latencyUs;
    
    historyIndex = (historyIndex + 1) % HISTORY_BUFFER_SIZE;
    if (historyIndex == 0) {
        historyFull = true;
    }
    
    if (loggingEnabled) {
        Serial.printf("📈 Historique: CPU:%d%% Heap:%dKB Voix:%d Stab:%d%%\n",
                      point->cpuUsage, point->heapFree, 
                      point->activeVoices, point->systemStability);
    }
}

/**
 * Calculer les tendances
 */
void calculateTrends() {
    if (!historyFull && historyIndex < 10) return; // Besoin d'au moins 10 points
    
    // Calculs de tendances simplifiés
    uint8_t pointsToAnalyze = min(historyIndex, 10);
    
    uint16_t cpuSum = 0;
    uint32_t heapSum = 0;
    
    for (uint8_t i = 0; i < pointsToAnalyze; i++) {
        uint8_t idx = (historyIndex - 1 - i + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
        cpuSum += performanceHistory[idx].cpuUsage;
        heapSum += performanceHistory[idx].heapFree;
    }
    
    uint8_t avgCPU = cpuSum / pointsToAnalyze;
    uint16_t avgHeap = heapSum / pointsToAnalyze;
    
    // Détection de tendances préoccupantes
    if (avgCPU > 80) {
        triggerAlert(1, 4, "Tendance CPU élevée détectée");
    }
    
    if (avgHeap < 100) {
        triggerAlert(2, 1, "Tendance mémoire faible détectée");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              AFFICHAGE ET INTERFACE                                   //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Mise à jour de l'affichage monitoring
 */
void updateDisplay() {
    if (!displayEnabled) return;
    
    switch (currentDisplayMode) {
        case DISPLAY_OVERVIEW:
            drawOverviewDisplay();
            break;
        case DISPLAY_AUDIO_DETAIL:
            drawAudioDetailDisplay();
            break;
        case DISPLAY_MEMORY_DETAIL:
            drawMemoryDetailDisplay();
            break;
        case DISPLAY_CONNECTIVITY_DETAIL:
            drawConnectivityDisplay();
            break;
        case DISPLAY_ALERTS:
            drawAlertsDisplay();
            break;
        case DISPLAY_HISTORY:
            drawHistoryDisplay();
            break;
    }
}

/**
 * Affichage vue d'ensemble
 */
void drawOverviewDisplay() {
    int x = MONITOR_DISPLAY_X;
    int y = MONITOR_DISPLAY_Y;
    
    // Effacer la zone
    gfx->fillRect(x, y, MONITOR_DISPLAY_WIDTH, MONITOR_DISPLAY_HEIGHT, BLACK);
    
    // Ligne de statut principale
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(x, y + 5);
    gfx->printf("PERF: CPU:%d%% MEM:%dKB VOIX:%d/%d LAT:%dus",
                (int)systemMetrics.cpuUsage,
                memoryMetrics.heapFree / 1024,
                audioMetrics.activeVoices,
                audioMetrics.maxVoices,
                audioMetrics.latencyUs);
    
    // Indicateurs de status
    gfx->setCursor(x, y + 20);
    
    // Audio
    gfx->setTextColor(audioMetrics.audioStable ? ZGREEN : ZRED, BLACK);
    gfx->print("AUD");
    
    // Mémoire
    gfx->setTextColor(memoryMetrics.heapFree > (100*1024) ? ZGREEN : ZRED, BLACK);
    gfx->print(" MEM");
    
    // SD
    gfx->setTextColor(connectivityMetrics.sdCardPresent ? ZGREEN : DARKGREY, BLACK);
    gfx->print(" SD");
    
    // WiFi
    gfx->setTextColor(connectivityMetrics.wifiConnected ? ZGREEN : DARKGREY, BLACK);
    gfx->print(" WiFi");
    
    // Alertes
    if (activeAlerts > 0) {
        gfx->setTextColor(ZRED, BLACK);
        gfx->printf(" [%d ALERTES]", activeAlerts);
    }
    
    // Score de stabilité global
    gfx->setCursor(x + 350, y + 5);
    int stabilityColor = ZGREEN;
    if (systemMetrics.systemStability < 70) stabilityColor = ZRED;
    else if (systemMetrics.systemStability < 85) stabilityColor = ZYELLOW;
    
    gfx->setTextColor(stabilityColor, BLACK);
    gfx->printf("STAB:%d%%", systemMetrics.systemStability);
}

/**
 * Affichage détail audio
 */
void drawAudioDetailDisplay() {
    int x = MONITOR_DISPLAY_X;
    int y = MONITOR_DISPLAY_Y;
    
    gfx->fillRect(x, y, MONITOR_DISPLAY_WIDTH, MONITOR_DISPLAY_HEIGHT, BLACK);
    
    gfx->setTextColor(ZCYAN, BLACK);
    gfx->setCursor(x, y + 5);
    gfx->print("AUDIO DÉTAIL");
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(x, y + 20);
    gfx->printf("Latence:%dus Voix:%d/%d CPU:%d%% %dHz/%dbit",
                audioMetrics.latencyUs,
                audioMetrics.activeVoices,
                audioMetrics.maxVoices,
                (int)audioMetrics.cpuUsageAudio,
                audioMetrics.sampleRate,
                audioMetrics.bitDepth);
}

/**
 * Affichage détail mémoire
 */
void drawMemoryDetailDisplay() {
    int x = MONITOR_DISPLAY_X;
    int y = MONITOR_DISPLAY_Y;
    
    gfx->fillRect(x, y, MONITOR_DISPLAY_WIDTH, MONITOR_DISPLAY_HEIGHT, BLACK);
    
    gfx->setTextColor(ZMAGENTA, BLACK);
    gfx->setCursor(x, y + 5);
    gfx->print("MÉMOIRE DÉTAIL");
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(x, y + 20);
    gfx->printf("Heap:%dKB/%dKB Frag:%d%% Cache:%d%% Min:%dKB",
                memoryMetrics.heapFree / 1024,
                memoryMetrics.heapTotal / 1024,
                memoryMetrics.fragmentation,
                memoryMetrics.cacheUsage,
                memoryMetrics.heapMin / 1024);
}

/**
 * Affichage connectivité
 */
void drawConnectivityDisplay() {
    int x = MONITOR_DISPLAY_X;
    int y = MONITOR_DISPLAY_Y;
    
    gfx->fillRect(x, y, MONITOR_DISPLAY_WIDTH, MONITOR_DISPLAY_HEIGHT, BLACK);
    
    gfx->setTextColor(ZYELLOW, BLACK);
    gfx->setCursor(x, y + 5);
    gfx->print("CONNECTIVITÉ");
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(x, y + 20);
    gfx->printf("WiFi:%s RSSI:%ddBm SD:%s R/W:%d/%dKB/s MIDI:%s",
                connectivityMetrics.wifiConnected ? "OK" : "--",
                connectivityMetrics.wifiRSSI,
                connectivityMetrics.sdCardPresent ? "OK" : "--",
                connectivityMetrics.sdReadSpeed,
                connectivityMetrics.sdWriteSpeed,
                connectivityMetrics.midiConnected ? "OK" : "--");
}

/**
 * Affichage alertes
 */
void drawAlertsDisplay() {
    int x = MONITOR_DISPLAY_X;
    int y = MONITOR_DISPLAY_Y;
    
    gfx->fillRect(x, y, MONITOR_DISPLAY_WIDTH, MONITOR_DISPLAY_HEIGHT, BLACK);
    
    gfx->setTextColor(ZRED, BLACK);
    gfx->setCursor(x, y + 5);
    gfx->printf("ALERTES ACTIVES: %d", activeAlerts);
    
    if (activeAlerts > 0) {
        // Afficher la dernière alerte
        uint8_t lastAlert = (alertIndex - 1 + ALERT_HISTORY_SIZE) % ALERT_HISTORY_SIZE;
        
        gfx->setTextColor(LIGHTGREY, BLACK);
        gfx->setCursor(x, y + 20);
        gfx->printf("Dernière: %s", alertHistory[lastAlert].message);
    } else {
        gfx->setTextColor(ZGREEN, BLACK);
        gfx->setCursor(x, y + 20);
        gfx->print("Aucune alerte active - Système stable");
    }
}

/**
 * Affichage historique
 */
void drawHistoryDisplay() {
    int x = MONITOR_DISPLAY_X;
    int y = MONITOR_DISPLAY_Y;
    
    gfx->fillRect(x, y, MONITOR_DISPLAY_WIDTH, MONITOR_DISPLAY_HEIGHT, BLACK);
    
    gfx->setTextColor(ZBLUE, BLACK);
    gfx->setCursor(x, y + 5);
    gfx->print("HISTORIQUE PERFORMANCE");
    
    // Graphique simple des derniers points
    if (historyIndex > 0) {
        int graphWidth = 200;
        int graphHeight = 20;
        int graphX = x + 200;
        int graphY = y + 15;
        
        gfx->drawRect(graphX, graphY, graphWidth, graphHeight, DARKGREY);
        
        // Dessiner les points CPU
        uint8_t pointsToShow = min(historyIndex, graphWidth / 4);
        for (uint8_t i = 0; i < pointsToShow; i++) {
            uint8_t idx = (historyIndex - 1 - i + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
            uint8_t cpuUsage = performanceHistory[idx].cpuUsage;
            
            int pointX = graphX + graphWidth - (i * 4);
            int pointY = graphY + graphHeight - ((cpuUsage * graphHeight) / 100);
            
            gfx->drawPixel(pointX, pointY, ZGREEN);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              COMMANDES ET CONTRÔLES                                   //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Changer le mode d'affichage
 */
void setDisplayMode(DisplayMode mode) {
    currentDisplayMode = mode;
    Serial.printf("Mode d'affichage: %d\n", mode);
}

/**
 * Rotation des modes d'affichage
 */
void cycleDisplayMode() {
    currentDisplayMode = (DisplayMode)((currentDisplayMode + 1) % 6);
    setDisplayMode(currentDisplayMode);
}

/**
 * Activer/désactiver les alertes
 */
void setAlertsEnabled(bool enabled) {
    alertsEnabled = enabled;
    Serial.printf("Alertes %s\n", enabled ? "activées" : "désactivées");
}

/**
 * Activer/désactiver l'affichage
 */
void setDisplayEnabled(bool enabled) {
    displayEnabled = enabled;
    
    if (!enabled) {
        // Effacer la zone de monitoring
        gfx->fillRect(MONITOR_DISPLAY_X, MONITOR_DISPLAY_Y, 
                     MONITOR_DISPLAY_WIDTH, MONITOR_DISPLAY_HEIGHT, BLACK);
    }
    
    Serial.printf("Affichage monitoring %s\n", enabled ? "activé" : "désactivé");
}

/**
 * Activer/désactiver le logging
 */
void setLoggingEnabled(bool enabled) {
    loggingEnabled = enabled;
    Serial.printf("Logging %s\n", enabled ? "activé" : "désactivé");
}

/**
 * Reset des statistiques
 */
void resetMonitoringStats() {
    audioMetrics.maxVoices = 0;
    audioMetrics.underruns = 0;
    audioMetrics.overruns = 0;
    
    memoryMetrics.heapMin = ESP.getFreeHeap();
    
    uiMetrics.maxTouchResponse = 0;
    
    systemMetrics.errorCount = 0;
    systemMetrics.warningCount = 0;
    
    connectivityMetrics.wifiReconnects = 0;
    
    Serial.println("📊 Statistiques de monitoring réinitialisées");
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              API PUBLIQUE                                             //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Configuration des callbacks
 */
void setAlertCallback(void (*callback)(SystemAlert*)) {
    onAlertTriggered = callback;
}

void setMetricsCallback(void (*callback)(void)) {
    onMetricsUpdated = callback;
}

/**
 * Obtenir les métriques actuelles
 */
AudioMetrics* getAudioMetrics() {
    return &audioMetrics;
}

MemoryMetrics* getMemoryMetrics() {
    return &memoryMetrics;
}

UIMetrics* getUIMetrics() {
    return &uiMetrics;
}

ConnectivityMetrics* getConnectivityMetrics() {
    return &connectivityMetrics;
}

SystemMetrics* getSystemMetrics() {
    return &systemMetrics;
}

/**
 * Obtenir l'historique de performance
 */
HistoryPoint* getPerformanceHistory() {
    return performanceHistory;
}

uint8_t getHistorySize() {
    return historyFull ? HISTORY_BUFFER_SIZE : historyIndex;
}

/**
 * Obtenir les alertes
 */
SystemAlert* getAlertHistory() {
    return alertHistory;
}

uint8_t getActiveAlertsCount() {
    return activeAlerts;
}

/**
 * Commandes de test rapides
 */
void runPerformanceQuickCheck() {
    Serial.println("🔍 Vérification rapide des performances...");
    
    updateAudioMetrics();
    updateMemoryMetrics();
    updateSystemMetrics();
    
    Serial.printf("Audio: %s (lat:%dus, voix:%d)\n",
                  audioMetrics.audioStable ? "OK" : "PROBLÈME",
                  audioMetrics.latencyUs,
                  audioMetrics.activeVoices);
                  
    Serial.printf("Mémoire: %dKB libre (frag:%d%%)\n",
                  memoryMetrics.heapFree / 1024,
                  memoryMetrics.fragmentation);
                  
    Serial.printf("Système: %d%% stabilité (CPU:%d%%)\n",
                  systemMetrics.systemStability,
                  (int)systemMetrics.cpuUsage);
}

void printSystemSummary() {
    Serial.println("\n📋 RÉSUMÉ SYSTÈME");
    Serial.println("=================");
    Serial.printf("Uptime: %lu ms\n", systemMetrics.uptime);
    Serial.printf("Stabilité: %d%%\n", systemMetrics.systemStability);
    Serial.printf("Alertes actives: %d\n", activeAlerts);
    Serial.printf("Heap libre: %d KB\n", memoryMetrics.heapFree / 1024);
    Serial.printf("Voix audio: %d/%d\n", audioMetrics.activeVoices, audioMetrics.maxVoices);
    Serial.printf("Latence audio: %d us\n", audioMetrics.latencyUs);
    Serial.println("=================");
}

////////////////////////////////////////////////////////////////////////////////////////////
//                                      FIN DU MODULE                                    //
////////////////////////////////////////////////////////////////////////////////////////////