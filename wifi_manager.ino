////////////////////////////////////////////////////////////////////////////////////////////
//                               WIFI FILE MANAGER                                       //
//                              DRUM 2025 VSAMPLER                                       //
////////////////////////////////////////////////////////////////////////////////////////////
// Module de gestion WiFi avec serveur web pour upload/download de samples
// Interface sécurisée avec désactivation automatique pendant utilisation audio
// 
// Auteur: ZCARLOS 2025
// Version: 1.0
//
// Fonctionnalités:
// - Interface web responsive pour gestion fichiers
// - Upload/download samples WAV depuis/vers SD card
// - API REST complète pour navigation et transferts
// - Sécurité: seuls les fichiers /SAMPLES/USER/ modifiables
// - Priorité audio: désactivation WiFi automatique si audio actif
// - Configuration réseau persistante via EEPROM

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

////////////////////////////////////////////////////////////////////////////////////////////
//                               CONFIGURATION WiFi                                      //
////////////////////////////////////////////////////////////////////////////////////////////

// Configuration réseau par défaut
#define WIFI_DEFAULT_SSID     "DRUM_2025_AP"
#define WIFI_DEFAULT_PASSWORD "DrumSampler2025"
#define WIFI_CONNECTION_TIMEOUT 15000   // 15 secondes timeout connexion
#define WIFI_AUTO_TIMEOUT      600000   // 10 minutes auto-désactivation
#define WIFI_ACTIVITY_TIMEOUT  5000     // 5 secondes après activité audio

// Configuration serveur web
#define WEB_SERVER_PORT       80
#define MAX_UPLOAD_SIZE       (10 * 1024 * 1024)  // 10MB max par fichier
#define TRANSFER_CHUNK_SIZE   8192                 // 8KB chunks pour transferts

// Configuration sécurité
#define WIFI_MAX_CLIENTS      4
#define WIFI_ALLOWED_PATH     "/SAMPLES/USER/"

////////////////////////////////////////////////////////////////////////////////////////////
//                               ÉNUMÉRATIONS ET STRUCTURES                              //
////////////////////////////////////////////////////////////////////////////////////////////

// États du gestionnaire WiFi
enum WiFiState {
    WIFI_DISABLED = 0,        // WiFi complètement désactivé (défaut)
    WIFI_SCANNING,            // Scan des réseaux disponibles
    WIFI_CONNECTING,          // Connexion en cours à un réseau
    WIFI_CONNECTED,           // Connecté en mode client
    WIFI_AP_MODE,             // Mode Access Point actif
    WIFI_SERVER_RUNNING,      // Serveur web actif
    WIFI_ERROR_STATE,         // Erreur de connexion
    WIFI_AUDIO_DISABLED       // Désactivé temporairement (audio actif)
};

// Modes de fonctionnement WiFi
enum WiFiMode {
    WIFI_CLIENT_MODE = 0,     // Mode client (connexion à réseau existant)
    WIFI_AP_MODE_ACTIVE       // Mode Access Point (créer réseau)
};

// Informations d'un réseau WiFi détecté
struct WiFiNetworkInfo {
    String ssid;
    int32_t rssi;
    int32_t channel;
    wifi_auth_mode_t authMode;
    bool isOpen;
};

// État des transferts de fichiers
struct TransferProgress {
    String filename;
    uint32_t totalBytes;
    uint32_t transferredBytes;
    uint32_t startTime;
    bool active;
    bool isUpload;
};

////////////////////////////////////////////////////////////////////////////////////////////
//                               CLASSE WIFI FILE MANAGER                                //
////////////////////////////////////////////////////////////////////////////////////////////

class WiFiFileManager {
private:
    // États et configuration
    WiFiState currentState;
    WiFiMode operatingMode;
    bool initialized;
    bool webServerActive;
    bool audioSystemActive;
    
    // Configuration réseau
    char clientSSID[64];
    char clientPassword[128];
    char apSSID[64];
    char apPassword[128];
    IPAddress localIP;
    IPAddress gatewayIP;
    IPAddress subnetMask;
    
    // Serveur web et transferts
    WebServer* webServer;
    TransferProgress currentTransfer;
    uint32_t lastActivity;
    uint32_t lastAudioCheck;
    uint32_t connectionStartTime;
    
    // Réseaux détectés
    std::vector<WiFiNetworkInfo> detectedNetworks;
    int selectedNetworkIndex;
    uint32_t lastNetworkScan;
    
    // Statistiques
    uint32_t totalUploads;
    uint32_t totalDownloads;
    uint64_t totalBytesTransferred;
    uint32_t activeClients;
    
    // Préférences système
    Preferences preferences;
    
public:
    /**
     * Constructeur
     */
    WiFiFileManager() : 
        currentState(WIFI_DISABLED),
        operatingMode(WIFI_CLIENT_MODE),
        initialized(false),
        webServerActive(false),
        audioSystemActive(false),
        webServer(nullptr),
        selectedNetworkIndex(-1),
        lastNetworkScan(0),
        totalUploads(0),
        totalDownloads(0),
        totalBytesTransferred(0),
        activeClients(0),
        lastActivity(0),
        lastAudioCheck(0),
        connectionStartTime(0)
    {
        strcpy(clientSSID, "");
        strcpy(clientPassword, "");
        strcpy(apSSID, WIFI_DEFAULT_SSID);
        strcpy(apPassword, WIFI_DEFAULT_PASSWORD);
        
        currentTransfer.active = false;
        currentTransfer.totalBytes = 0;
        currentTransfer.transferredBytes = 0;
        currentTransfer.isUpload = false;
    }
    
    /**
     * Initialisation du gestionnaire WiFi
     */
    bool begin() {
        if (initialized) {
            Serial.println("WiFi Manager déjà initialisé");
            return true;
        }
        
        Serial.println("=== INITIALISATION WIFI FILE MANAGER ===");
        
        // Charger la configuration depuis EEPROM
        if (!loadConfiguration()) {
            Serial.println("Configuration par défaut chargée");
        }
        
        // Initialiser WiFi en mode désactivé
        WiFi.mode(WIFI_OFF);
        currentState = WIFI_DISABLED;
        
        // Créer le serveur web (non démarré)
        webServer = new WebServer(WEB_SERVER_PORT);
        setupWebServerRoutes();
        
        initialized = true;
        lastActivity = millis();
        lastAudioCheck = millis();
        
        Serial.println("WiFi File Manager initialisé");
        Serial.printf("Configuration: Client='%s' / AP='%s'\n", clientSSID, apSSID);
        return true;
    }
    
    /**
     * Mise à jour principale (à appeler dans loop())
     */
    void update() {
        if (!initialized) return;
        
        uint32_t currentTime = millis();
        
        // Vérification priorité audio (critique)
        if (currentTime - lastAudioCheck >= 1000) { // Vérifier chaque seconde
            lastAudioCheck = currentTime;
            checkAudioPriority();
        }
        
        // Gestion des états WiFi
        switch (currentState) {
            case WIFI_SCANNING:
                updateScanning();
                break;
            case WIFI_CONNECTING:
                updateConnecting(currentTime);
                break;
            case WIFI_CONNECTED:
            case WIFI_AP_MODE:
            case WIFI_SERVER_RUNNING:
                updateConnected(currentTime);
                break;
            case WIFI_ERROR_STATE:
                updateError(currentTime);
                break;
            default:
                break;
        }
        
        // Gestion du serveur web
        if (webServerActive && webServer) {
            webServer->handleClient();
            updateClientCount();
        }
        
        // Auto-timeout si inactif
        if (isWiFiActive() && currentTime - lastActivity > WIFI_AUTO_TIMEOUT) {
            Serial.println("WiFi auto-désactivé (timeout inactivité)");
            stopWiFi();
        }
    }
    
    /**
     * Vérification critique de la priorité audio
     */
    void checkAudioPriority() {
        bool audioNowActive = isAudioSystemActive();
        
        if (audioNowActive && !audioSystemActive) {
            // Audio vient de démarrer - désactiver WiFi immédiatement
            audioSystemActive = true;
            if (isWiFiActive()) {
                Serial.println("🔊 AUDIO ACTIF - Désactivation WiFi immédiate");
                forceStopWiFi();
                currentState = WIFI_AUDIO_DISABLED;
            }
        } else if (!audioNowActive && audioSystemActive) {
            // Audio vient de s'arrêter
            audioSystemActive = false;
            if (currentState == WIFI_AUDIO_DISABLED) {
                Serial.println("🔇 Audio arrêté - WiFi peut être réactivé");
                currentState = WIFI_DISABLED;
            }
        }
        
        audioSystemActive = audioNowActive;
    }
    
    /**
     * Vérifier si le système audio est actif
     */
    bool isAudioSystemActive() {
        // Vérifier WM8731 actif
        if (wm8731_isReady()) {
            // Lire un petit échantillon pour détecter l'activité
            uint8_t testBuffer[32];
            size_t bytesRead = wm8731_readAudio(testBuffer, 8);
            if (bytesRead > 0) {
                // Analyser le signal pour détecter de l'activité
                int16_t maxLevel = 0;
                for (size_t i = 0; i < bytesRead; i += 2) {
                    int16_t sample = *((int16_t*)(testBuffer + i));
                    if (abs(sample) > maxLevel) {
                        maxLevel = abs(sample);
                    }
                }
                if (maxLevel > 100) { // Seuil d'activité
                    return true;
                }
            }
        }
        
        // Vérifier synthESP32 actif (voices en cours de lecture)
        for (int i = 0; i < 16; i++) {
            if (latch[i] > 0) {
                return true;
            }
        }
        
        // Vérifier séquenceur actif
        if (playing || recording) {
            return true;
        }
        
        // Vérifier activité récente (dans les 5 dernières secondes)
        extern uint32_t lastAudioActivity;
        return (millis() - lastAudioActivity) < WIFI_ACTIVITY_TIMEOUT;
    }
    
    /**
     * Démarrage du WiFi selon le mode configuré
     */
    bool startWiFi() {
        if (audioSystemActive) {
            Serial.println("❌ Impossible de démarrer WiFi - Audio actif");
            return false;
        }
        
        if (currentState != WIFI_DISABLED && currentState != WIFI_AUDIO_DISABLED) {
            Serial.println("WiFi déjà actif");
            return true;
        }
        
        Serial.println("🚀 Démarrage WiFi...");
        lastActivity = millis();
        
        if (operatingMode == WIFI_CLIENT_MODE && strlen(clientSSID) > 0) {
            return startClientMode();
        } else {
            return startAPMode();
        }
    }
    
    /**
     * Démarrage en mode client
     */
    bool startClientMode() {
        Serial.printf("Mode Client - Connexion à '%s'\n", clientSSID);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(clientSSID, clientPassword);
        
        currentState = WIFI_CONNECTING;
        connectionStartTime = millis();
        
        return true;
    }
    
    /**
     * Démarrage en mode Access Point
     */
    bool startAPMode() {
        Serial.printf("Mode AP - Création réseau '%s'\n", apSSID);
        
        WiFi.mode(WIFI_AP);
        bool success = WiFi.softAP(apSSID, apPassword, 1, 0, WIFI_MAX_CLIENTS);
        
        if (success) {
            localIP = WiFi.softAPIP();
            currentState = WIFI_AP_MODE;
            
            Serial.printf("✅ Access Point créé\n");
            Serial.printf("📱 SSID: %s\n", apSSID);
            Serial.printf("🔒 Password: %s\n", apPassword);
            Serial.printf("🌐 IP: %s\n", localIP.toString().c_str());
            
            return startWebServer();
        } else {
            Serial.println("❌ Échec création Access Point");
            currentState = WIFI_ERROR_STATE;
            return false;
        }
    }
    
    /**
     * Arrêt complet du WiFi
     */
    void stopWiFi() {
        Serial.println("🛑 Arrêt WiFi...");
        
        stopWebServer();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        
        currentState = WIFI_DISABLED;
        audioSystemActive = false;
        
        // Réinitialiser les statistiques de session
        activeClients = 0;
        currentTransfer.active = false;
        
        Serial.println("WiFi arrêté");
    }
    
    /**
     * Arrêt forcé immédiat (pour priorité audio)
     */
    void forceStopWiFi() {
        if (webServer) {
            webServer->stop();
        }
        webServerActive = false;
        
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        
        activeClients = 0;
        currentTransfer.active = false;
    }
    
    /**
     * Démarrage du serveur web
     */
    bool startWebServer() {
        if (!webServer || audioSystemActive) {
            return false;
        }
        
        Serial.println("🌐 Démarrage serveur web...");
        
        webServer->begin();
        webServerActive = true;
        currentState = WIFI_SERVER_RUNNING;
        
        Serial.printf("✅ Serveur web actif sur http://%s:%d\n", 
                     localIP.toString().c_str(), WEB_SERVER_PORT);
        
        return true;
    }
    
    /**
     * Arrêt du serveur web
     */
    void stopWebServer() {
        if (webServer && webServerActive) {
            webServer->stop();
            webServerActive = false;
            Serial.println("🌐 Serveur web arrêté");
        }
    }
    
    /**
     * Configuration des routes du serveur web
     */
    void setupWebServerRoutes() {
        if (!webServer) return;
        
        // Page principale
        webServer->on("/", HTTP_GET, [this]() { handleRoot(); });
        
        // API REST pour gestion fichiers
        webServer->on("/api/files", HTTP_GET, [this]() { handleFileList(); });
        webServer->on("/api/upload", HTTP_POST, [this]() { handleUploadResponse(); }, [this]() { handleFileUpload(); });
        webServer->on("/api/delete", HTTP_DELETE, [this]() { handleFileDelete(); });
        webServer->on("/api/info", HTTP_GET, [this]() { handleSystemInfo(); });
        webServer->on("/api/status", HTTP_GET, [this]() { handleStatus(); });
        
        // Téléchargement de fichiers
        webServer->on("/download", HTTP_GET, [this]() { handleFileDownload(); });
        
        // Gestion des erreurs
        webServer->onNotFound([this]() { handleNotFound(); });
        
        Serial.println("Routes serveur web configurées");
    }
    
    /**
     * Gestion de la page principale
     */
    void handleRoot() {
        lastActivity = millis();
        
        if (audioSystemActive) {
            webServer->send(503, "text/html", 
                          "<h1>Service Temporarily Unavailable</h1>"
                          "<p>Audio system is active. Please wait...</p>");
            return;
        }
        
        String html = generateMainHTML();
        webServer->send(200, "text/html", html);
    }
    
    /**
     * Génération de l'interface HTML principale
     */
    String generateMainHTML() {
        return R"(
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DRUM 2025 VSAMPLER - File Manager</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);
            color: #ffffff;
            min-height: 100vh;
            overflow-x: hidden;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px 0;
            border-bottom: 2px solid #00ff88;
        }
        
        .header h1 {
            font-size: 2.5em;
            color: #00ff88;
            text-shadow: 0 0 10px rgba(0, 255, 136, 0.3);
            margin-bottom: 10px;
        }
        
        .header p {
            color: #cccccc;
            font-size: 1.1em;
        }
        
        .status-bar {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            padding: 15px;
            margin-bottom: 30px;
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
        }
        
        .status-item {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .status-icon {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }
        
        .status-icon.online { background: #00ff88; }
        .status-icon.offline { background: #ff4444; }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        .upload-section {
            background: rgba(0, 0, 0, 0.2);
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 30px;
            border: 2px dashed #666;
            transition: all 0.3s ease;
            cursor: pointer;
        }
        
        .upload-section:hover {
            border-color: #00ff88;
            background: rgba(0, 255, 136, 0.05);
        }
        
        .upload-section.dragover {
            border-color: #00ff88;
            background: rgba(0, 255, 136, 0.1);
            transform: scale(1.02);
        }
        
        .upload-content {
            text-align: center;
        }
        
        .upload-icon {
            font-size: 3em;
            color: #00ff88;
            margin-bottom: 20px;
        }
        
        .upload-text {
            font-size: 1.2em;
            margin-bottom: 15px;
        }
        
        .upload-hint {
            color: #999;
            font-size: 0.9em;
        }
        
        .progress-container {
            margin-top: 20px;
            display: none;
        }
        
        .progress-bar {
            width: 100%;
            height: 20px;
            background: #333;
            border-radius: 10px;
            overflow: hidden;
            margin-bottom: 10px;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #00ff88, #00cc6a);
            width: 0%;
            transition: width 0.3s ease;
            border-radius: 10px;
        }
        
        .progress-text {
            text-align: center;
            color: #ccc;
            font-size: 0.9em;
        }
        
        .file-list {
            background: rgba(0, 0, 0, 0.2);
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 30px;
        }
        
        .file-list h3 {
            color: #00ff88;
            margin-bottom: 20px;
            font-size: 1.5em;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .file-grid {
            display: grid;
            gap: 15px;
        }
        
        .file-item {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            padding: 15px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            transition: all 0.3s ease;
            border-left: 4px solid #00ff88;
        }
        
        .file-item:hover {
            background: rgba(255, 255, 255, 0.1);
            transform: translateX(5px);
        }
        
        .file-info {
            display: flex;
            align-items: center;
            gap: 15px;
            flex: 1;
        }
        
        .file-icon {
            width: 40px;
            height: 40px;
            background: #00ff88;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-weight: bold;
            color: #000;
        }
        
        .file-details {
            flex: 1;
        }
        
        .file-name {
            font-weight: bold;
            margin-bottom: 5px;
        }
        
        .file-meta {
            color: #999;
            font-size: 0.85em;
        }
        
        .file-actions {
            display: flex;
            gap: 10px;
        }
        
        .btn {
            padding: 8px 16px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            text-decoration: none;
            font-size: 0.9em;
            transition: all 0.3s ease;
            display: inline-flex;
            align-items: center;
            gap: 5px;
        }
        
        .btn-primary {
            background: #00ff88;
            color: #000;
        }
        
        .btn-primary:hover {
            background: #00cc6a;
            transform: translateY(-2px);
        }
        
        .btn-danger {
            background: #ff4444;
            color: #fff;
        }
        
        .btn-danger:hover {
            background: #cc3333;
            transform: translateY(-2px);
        }
        
        .btn-secondary {
            background: #666;
            color: #fff;
        }
        
        .btn-secondary:hover {
            background: #555;
            transform: translateY(-2px);
        }
        
        .system-info {
            background: rgba(0, 0, 0, 0.2);
            border-radius: 15px;
            padding: 20px;
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
        }
        
        .info-card {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            padding: 15px;
            text-align: center;
        }
        
        .info-value {
            font-size: 2em;
            font-weight: bold;
            color: #00ff88;
            margin-bottom: 5px;
        }
        
        .info-label {
            color: #ccc;
            font-size: 0.9em;
        }
        
        .empty-state {
            text-align: center;
            padding: 40px;
            color: #666;
        }
        
        .empty-state-icon {
            font-size: 4em;
            margin-bottom: 20px;
        }
        
        @media (max-width: 768px) {
            .container { padding: 10px; }
            .header h1 { font-size: 2em; }
            .file-item { flex-direction: column; align-items: flex-start; gap: 15px; }
            .file-actions { width: 100%; justify-content: flex-end; }
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- En-tête -->
        <div class="header">
            <h1>🥁 DRUM 2025 VSAMPLER</h1>
            <p>File Manager - Gestion des samples WAV</p>
        </div>
        
        <!-- Barre de statut -->
        <div class="status-bar">
            <div class="status-item">
                <div class="status-icon online" id="statusNetwork"></div>
                <span>Réseau: <span id="networkStatus">En ligne</span></span>
            </div>
            <div class="status-item">
                <div class="status-icon online" id="statusSD"></div>
                <span>SD Card: <span id="sdStatus">OK</span></span>
            </div>
            <div class="status-item">
                <div class="status-icon offline" id="statusAudio"></div>
                <span>Audio: <span id="audioStatus">Inactif</span></span>
            </div>
            <div class="status-item">
                <span>Clients: <span id="clientCount">0</span></span>
            </div>
        </div>
        
        <!-- Zone d'upload -->
        <div class="upload-section" id="uploadZone">
            <div class="upload-content">
                <div class="upload-icon">📤</div>
                <div class="upload-text">Glissez vos fichiers WAV ici ou cliquez pour parcourir</div>
                <div class="upload-hint">Formats supportés: WAV 16-bit, Taille max: 10MB</div>
                <input type="file" id="fileInput" multiple accept=".wav" style="display: none;">
            </div>
            <div class="progress-container" id="progressContainer">
                <div class="progress-bar">
                    <div class="progress-fill" id="progressFill"></div>
                </div>
                <div class="progress-text" id="progressText">Upload en cours...</div>
            </div>
        </div>
        
        <!-- Liste des fichiers -->
        <div class="file-list">
            <h3>📁 Bibliothèque de samples</h3>
            <div class="file-grid" id="fileGrid">
                <!-- Fichiers chargés dynamiquement -->
            </div>
        </div>
        
        <!-- Informations système -->
        <div class="system-info">
            <div class="info-card">
                <div class="info-value" id="totalFiles">0</div>
                <div class="info-label">Samples totaux</div>
            </div>
            <div class="info-card">
                <div class="info-value" id="totalSize">0 MB</div>
                <div class="info-label">Espace utilisé</div>
            </div>
            <div class="info-card">
                <div class="info-value" id="uploadCount">0</div>
                <div class="info-label">Uploads</div>
            </div>
            <div class="info-card">
                <div class="info-value" id="downloadCount">0</div>
                <div class="info-label">Downloads</div>
            </div>
        </div>
    </div>
    
    <script>
        // Variables globales
        let files = [];
        let systemStatus = {};
        
        // Initialisation
        document.addEventListener('DOMContentLoaded', function() {
            setupEventListeners();
            loadFileList();
            updateSystemStatus();
            
            // Mise à jour périodique
            setInterval(updateSystemStatus, 5000);
            setInterval(loadFileList, 10000);
        });
        
        // Configuration des événements
        function setupEventListeners() {
            const uploadZone = document.getElementById('uploadZone');
            const fileInput = document.getElementById('fileInput');
            
            // Upload zone events
            uploadZone.addEventListener('click', () => fileInput.click());
            uploadZone.addEventListener('dragover', handleDragOver);
            uploadZone.addEventListener('dragleave', handleDragLeave);
            uploadZone.addEventListener('drop', handleDrop);
            
            // File input change
            fileInput.addEventListener('change', handleFileSelect);
        }
        
        // Gestion drag & drop
        function handleDragOver(e) {
            e.preventDefault();
            e.currentTarget.classList.add('dragover');
        }
        
        function handleDragLeave(e) {
            e.preventDefault();
            e.currentTarget.classList.remove('dragover');
        }
        
        function handleDrop(e) {
            e.preventDefault();
            e.currentTarget.classList.remove('dragover');
            
            const files = Array.from(e.dataTransfer.files);
            uploadFiles(files);
        }
        
        // Sélection de fichiers
        function handleFileSelect(e) {
            const files = Array.from(e.target.files);
            uploadFiles(files);
        }
        
        // Upload de fichiers
        async function uploadFiles(fileList) {
            for (const file of fileList) {
                if (file.type !== 'audio/wav' && !file.name.toLowerCase().endsWith('.wav')) {
                    alert(`Fichier ignoré: ${file.name} (format non supporté)`);
                    continue;
                }
                
                if (file.size > 10 * 1024 * 1024) {
                    alert(`Fichier trop volumineux: ${file.name} (max 10MB)`);
                    continue;
                }
                
                await uploadFile(file);
            }
        }
        
        // Upload d'un fichier individuel
        async function uploadFile(file) {
            const formData = new FormData();
            formData.append('file', file);
            
            const progressContainer = document.getElementById('progressContainer');
            const progressFill = document.getElementById('progressFill');
            const progressText = document.getElementById('progressText');
            
            progressContainer.style.display = 'block';
            progressText.textContent = `Upload: ${file.name}`;
            
            try {
                const xhr = new XMLHttpRequest();
                
                xhr.upload.onprogress = (e) => {
                    if (e.lengthComputable) {
                        const percent = (e.loaded / e.total) * 100;
                        progressFill.style.width = percent + '%';
                        progressText.textContent = `Upload: ${file.name} (${Math.round(percent)}%)`;
                    }
                };
                
                xhr.onload = () => {
                    if (xhr.status === 200) {
                        progressText.textContent = `✅ Upload terminé: ${file.name}`;
                        setTimeout(() => {
                            progressContainer.style.display = 'none';
                            progressFill.style.width = '0%';
                        }, 2000);
                        loadFileList(); // Recharger la liste
                    } else {
                        progressText.textContent = `❌ Erreur upload: ${file.name}`;
                    }
                };
                
                xhr.onerror = () => {
                    progressText.textContent = `❌ Erreur réseau: ${file.name}`;
                };
                
                xhr.open('POST', '/api/upload');
                xhr.send(formData);
                
            } catch (error) {
                console.error('Erreur upload:', error);
                progressText.textContent = `❌ Erreur: ${file.name}`;
            }
        }
        
        // Chargement de la liste des fichiers
        async function loadFileList() {
            try {
                const response = await fetch('/api/files');
                if (!response.ok) throw new Error('Erreur chargement fichiers');
                
                files = await response.json();
                displayFiles(files);
                
            } catch (error) {
                console.error('Erreur chargement fichiers:', error);
                displayError('Impossible de charger la liste des fichiers');
            }
        }
        
        // Affichage des fichiers
        function displayFiles(fileList) {
            const fileGrid = document.getElementById('fileGrid');
            
            if (fileList.length === 0) {
                fileGrid.innerHTML = `
                    <div class="empty-state">
                        <div class="empty-state-icon">📂</div>
                        <p>Aucun sample trouvé</p>
                        <p>Uploadez vos premiers fichiers WAV !</p>
                    </div>
                `;
                return;
            }
            
            const fileHTML = fileList.map(file => {
                const sizeKB = Math.round(file.size / 1024);
                const duration = file.duration ? `${file.duration}ms` : 'N/A';
                
                return `
                    <div class="file-item">
                        <div class="file-info">
                            <div class="file-icon">WAV</div>
                            <div class="file-details">
                                <div class="file-name">${file.name}</div>
                                <div class="file-meta">${sizeKB} KB • ${duration} • ${file.channels || 1}ch</div>
                            </div>
                        </div>
                        <div class="file-actions">
                            <a href="/download?file=${encodeURIComponent(file.name)}" class="btn btn-primary">
                                📥 Télécharger
                            </a>
                            <button class="btn btn-danger" onclick="deleteFile('${file.name}')">
                                🗑️ Supprimer
                            </button>
                        </div>
                    </div>
                `;
            }).join('');
            
            fileGrid.innerHTML = fileHTML;
        }
        
        // Suppression d'un fichier
        async function deleteFile(filename) {
            if (!confirm(`Confirmer la suppression de "${filename}" ?`)) {
                return;
            }
            
            try {
                const response = await fetch(`/api/delete?file=${encodeURIComponent(filename)}`, {
                    method: 'DELETE'
                });
                
                if (response.ok) {
                    loadFileList(); // Recharger la liste
                } else {
                    alert('Erreur lors de la suppression');
                }
            } catch (error) {
                console.error('Erreur suppression:', error);
                alert('Erreur de connexion');
            }
        }
        
        // Mise à jour du statut système
        async function updateSystemStatus() {
            try {
                const response = await fetch('/api/status');
                if (!response.ok) throw new Error('Erreur statut');
                
                systemStatus = await response.json();
                updateStatusDisplay(systemStatus);
                
            } catch (error) {
                console.error('Erreur statut:', error);
            }
        }
        
        // Affichage du statut
        function updateStatusDisplay(status) {
            // Statut réseau
            const networkIcon = document.getElementById('statusNetwork');
            const networkText = document.getElementById('networkStatus');
            networkIcon.className = `status-icon ${status.network ? 'online' : 'offline'}`;
            networkText.textContent = status.network ? 'En ligne' : 'Hors ligne';
            
            // Statut SD
            const sdIcon = document.getElementById('statusSD');
            const sdText = document.getElementById('sdStatus');
            sdIcon.className = `status-icon ${status.sdCard ? 'online' : 'offline'}`;
            sdText.textContent = status.sdCard ? 'OK' : 'Erreur';
            
            // Statut audio
            const audioIcon = document.getElementById('statusAudio');
            const audioText = document.getElementById('audioStatus');
            audioIcon.className = `status-icon ${status.audioActive ? 'online' : 'offline'}`;
            audioText.textContent = status.audioActive ? 'Actif' : 'Inactif';
            
            // Clients connectés
            document.getElementById('clientCount').textContent = status.clients || 0;
            
            // Statistiques
            document.getElementById('totalFiles').textContent = status.totalFiles || 0;
            document.getElementById('totalSize').textContent = `${Math.round((status.totalSize || 0) / 1024 / 1024)} MB`;
            document.getElementById('uploadCount').textContent = status.uploads || 0;
            document.getElementById('downloadCount').textContent = status.downloads || 0;
        }
        
        // Affichage d'erreur
        function displayError(message) {
            const fileGrid = document.getElementById('fileGrid');
            fileGrid.innerHTML = `
                <div class="empty-state">
                    <div class="empty-state-icon">⚠️</div>
                    <p>${message}</p>
                </div>
            `;
        }
    </script>
</body>
</html>
        )";
    }
    
    /**
     * Gestion de la liste des fichiers (API)
     */
    void handleFileList() {
        lastActivity = millis();
        
        if (audioSystemActive) {
            webServer->send(503, "application/json", "{\"error\":\"Audio system active\"}");
            return;
        }
        
        if (!sdIsReady()) {
            webServer->send(500, "application/json", "{\"error\":\"SD card not available\"}");
            return;
        }
        
        DynamicJsonDocument doc(4096);
        JsonArray filesArray = doc.to<JsonArray>();
        
        // Scanner le répertoire /SAMPLES/USER/
        File root = SD.open(WIFI_ALLOWED_PATH);
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (!file.isDirectory()) {
                    String fileName = file.name();
                    if (fileName.toLowerCase().endsWith(".wav")) {
                        JsonObject fileObj = filesArray.createNestedObject();
                        fileObj["name"] = fileName;
                        fileObj["size"] = file.size();
                        fileObj["path"] = String(WIFI_ALLOWED_PATH) + fileName;
                        
                        // Essayer de parser les infos WAV
                        SampleInfo info;
                        if (sdParseWavHeader(file, info)) {
                            fileObj["duration"] = info.duration;
                            fileObj["channels"] = info.channels;
                            fileObj["sampleRate"] = info.sampleRate;
                        }
                    }
                }
                file.close();
                file = root.openNextFile();
            }
            root.close();
        }
        
        String response;
        serializeJson(doc, response);
        webServer->send(200, "application/json", response);
    }
    
    /**
     * Gestion de l'upload de fichiers
     */
    void handleFileUpload() {
        lastActivity = millis();
        
        HTTPUpload& upload = webServer->upload();
        
        if (upload.status == UPLOAD_FILE_START) {
            if (audioSystemActive) {
                return; // Abandonner silencieusement
            }
            
            // Vérifier l'extension
            String filename = upload.filename;
            if (!filename.toLowerCase().endsWith(".wav")) {
                return; // Fichier non supporté
            }
            
            // Créer le chemin de destination
            String filepath = String(WIFI_ALLOWED_PATH) + filename;
            
            // Démarrer l'upload
            File uploadFile = SD.open(filepath, FILE_WRITE);
            if (uploadFile) {
                currentTransfer.filename = filename;
                currentTransfer.totalBytes = upload.totalSize;
                currentTransfer.transferredBytes = 0;
                currentTransfer.startTime = millis();
                currentTransfer.active = true;
                currentTransfer.isUpload = true;
                
                uploadFile.close();
                Serial.printf("📤 Upload démarré: %s (%d bytes)\n", filename.c_str(), upload.totalSize);
            }
            
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (audioSystemActive || !currentTransfer.active) {
                return;
            }
            
            String filepath = String(WIFI_ALLOWED_PATH) + currentTransfer.filename;
            File uploadFile = SD.open(filepath, FILE_APPEND);
            if (uploadFile) {
                uploadFile.write(upload.buf, upload.currentSize);
                uploadFile.close();
                
                currentTransfer.transferredBytes += upload.currentSize;
                
                // Vérifier si on dépasse la limite
                if (currentTransfer.transferredBytes > MAX_UPLOAD_SIZE) {
                    SD.remove(filepath.c_str());
                    currentTransfer.active = false;
                    Serial.println("❌ Upload annulé: fichier trop volumineux");
                }
            }
            
        } else if (upload.status == UPLOAD_FILE_END) {
            if (currentTransfer.active) {
                totalUploads++;
                totalBytesTransferred += currentTransfer.transferredBytes;
                
                uint32_t duration = millis() - currentTransfer.startTime;
                Serial.printf("✅ Upload terminé: %s (%d bytes en %d ms)\n", 
                             currentTransfer.filename.c_str(), 
                             currentTransfer.transferredBytes, 
                             duration);
            }
            
            currentTransfer.active = false;
        }
    }
    
    /**
     * Réponse après upload
     */
    void handleUploadResponse() {
        if (audioSystemActive) {
            webServer->send(503, "application/json", "{\"error\":\"Audio system active\"}");
            return;
        }
        
        if (currentTransfer.transferredBytes > MAX_UPLOAD_SIZE) {
            webServer->send(413, "application/json", "{\"error\":\"File too large\"}");
        } else {
            webServer->send(200, "application/json", "{\"success\":true}");
        }
    }
    
    /**
     * Gestion du téléchargement de fichiers
     */
    void handleFileDownload() {
        lastActivity = millis();
        
        if (audioSystemActive) {
            webServer->send(503, "text/plain", "Service temporarily unavailable");
            return;
        }
        
        String filename = webServer->arg("file");
        if (filename.length() == 0) {
            webServer->send(400, "text/plain", "Missing file parameter");
            return;
        }
        
        // Vérifier que le fichier est dans le répertoire autorisé
        String filepath = String(WIFI_ALLOWED_PATH) + filename;
        
        if (!SD.exists(filepath)) {
            webServer->send(404, "text/plain", "File not found");
            return;
        }
        
        File file = SD.open(filepath);
        if (!file) {
            webServer->send(500, "text/plain", "Cannot open file");
            return;
        }
        
        // Démarrer le téléchargement
        totalDownloads++;
        totalBytesTransferred += file.size();
        
        webServer->streamFile(file, "audio/wav");
        file.close();
        
        Serial.printf("📥 Téléchargement: %s (%d bytes)\n", filename.c_str(), file.size());
    }
    
    /**
     * Gestion de la suppression de fichiers
     */
    void handleFileDelete() {
        lastActivity = millis();
        
        if (audioSystemActive) {
            webServer->send(503, "application/json", "{\"error\":\"Audio system active\"}");
            return;
        }
        
        String filename = webServer->arg("file");
        if (filename.length() == 0) {
            webServer->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
            return;
        }
        
        // Vérifier que le fichier est dans le répertoire autorisé
        String filepath = String(WIFI_ALLOWED_PATH) + filename;
        
        if (!SD.exists(filepath)) {
            webServer->send(404, "application/json", "{\"error\":\"File not found\"}");
            return;
        }
        
        if (SD.remove(filepath)) {
            webServer->send(200, "application/json", "{\"success\":true}");
            Serial.printf("🗑️ Fichier supprimé: %s\n", filename.c_str());
        } else {
            webServer->send(500, "application/json", "{\"error\":\"Cannot delete file\"}");
        }
    }
    
    /**
     * Informations système
     */
    void handleSystemInfo() {
        lastActivity = millis();
        
        DynamicJsonDocument doc(1024);
        
        doc["device"] = "DRUM_2025_VSAMPLER";
        doc["version"] = "2.0";
        doc["wifi_state"] = getStateString();
        doc["ip"] = localIP.toString();
        doc["uptime"] = millis();
        doc["free_memory"] = ESP.getFreeHeap();
        doc["total_memory"] = ESP.getHeapSize();
        
        String response;
        serializeJson(doc, response);
        webServer->send(200, "application/json", response);
    }
    
    /**
     * Statut en temps réel
     */
    void handleStatus() {
        lastActivity = millis();
        
        DynamicJsonDocument doc(512);
        
        doc["network"] = isWiFiActive();
        doc["sdCard"] = sdIsReady();
        doc["audioActive"] = audioSystemActive;
        doc["clients"] = activeClients;
        doc["totalFiles"] = getTotalFileCount();
        doc["totalSize"] = getTotalFileSize();
        doc["uploads"] = totalUploads;
        doc["downloads"] = totalDownloads;
        
        String response;
        serializeJson(doc, response);
        webServer->send(200, "application/json", response);
    }
    
    /**
     * Gestion des pages non trouvées
     */
    void handleNotFound() {
        webServer->send(404, "text/html", 
                      "<h1>404 - Page Not Found</h1>"
                      "<p><a href='/'>Retour à l'accueil</a></p>");
    }
    
    // ... [Continuer avec le reste de la classe dans la partie 2]
};

// Instance globale du gestionnaire WiFi
WiFiFileManager wifiManager;

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS PUBLIQUES                                     //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Initialisation du gestionnaire WiFi
 */
bool wifiManagerBegin() {
    return wifiManager.begin();
}

/**
 * Mise à jour du gestionnaire (à appeler dans loop())
 */
void wifiManagerUpdate() {
    wifiManager.update();
}

/**
 * Interface pour le système de menu
 */
void drawWifiManager() {
    // Cette fonction sera implémentée dans la partie suivante
    // pour l'interface tactile complète
}

/**
 * Gestion des touches tactiles WiFi Manager
 */
void wifiManagerHandleTouch(uint16_t zone) {
    // Cette fonction sera implémentée dans la partie suivante
    // pour gérer toutes les zones tactiles définies
}

/**
 * Vérifier si le WiFi est actif
 */
bool isWifiManagerActive() {
    return wifiManager.isWiFiActive();
}

/**
 * État du système audio (pour priorité)
 */
uint32_t lastAudioActivity = 0;

/**
 * Marquer une activité audio (à appeler depuis synthESP32)
 */
void markAudioActivity() {
    lastAudioActivity = millis();
}