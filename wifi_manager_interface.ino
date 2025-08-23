////////////////////////////////////////////////////////////////////////////////////////////
//                          WIFI MANAGER - INTERFACE TACTILE                            //
//                              DRUM 2025 VSAMPLER                                       //
////////////////////////////////////////////////////////////////////////////////////////////
// Partie 2 du module WiFi File Manager
// Interface utilisateur tactile et fonctions complémentaires

////////////////////////////////////////////////////////////////////////////////////////////
//                               CONTINUATION CLASSE WiFiFileManager                     //
////////////////////////////////////////////////////////////////////////////////////////////

// Continuation de la classe WiFiFileManager (partie privée)
private:
    /**
     * Mise à jour de l'état scanning
     */
    void updateScanning() {
        if (millis() - lastNetworkScan > 3000) { // Scan terminé après 3 secondes
            int networksFound = WiFi.scanComplete();
            
            if (networksFound >= 0) {
                Serial.printf("📡 %d réseaux détectés\n", networksFound);
                
                detectedNetworks.clear();
                for (int i = 0; i < networksFound; i++) {
                    WiFiNetworkInfo network;
                    network.ssid = WiFi.SSID(i);
                    network.rssi = WiFi.RSSI(i);
                    network.channel = WiFi.channel(i);
                    network.authMode = WiFi.encryptionType(i);
                    network.isOpen = (network.authMode == WIFI_AUTH_OPEN);
                    
                    detectedNetworks.push_back(network);
                }
                
                // Trier par signal décroissant
                std::sort(detectedNetworks.begin(), detectedNetworks.end(),
                         [](const WiFiNetworkInfo& a, const WiFiNetworkInfo& b) {
                             return a.rssi > b.rssi;
                         });
                
                currentState = WIFI_DISABLED;
                WiFi.scanDelete();
            }
        }
    }
    
    /**
     * Mise à jour de l'état connecting
     */
    void updateConnecting(uint32_t currentTime) {
        if (currentTime - connectionStartTime > WIFI_CONNECTION_TIMEOUT) {
            Serial.println("❌ Timeout connexion WiFi");
            currentState = WIFI_ERROR_STATE;
            WiFi.disconnect();
            return;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            localIP = WiFi.localIP();
            gatewayIP = WiFi.gatewayIP();
            subnetMask = WiFi.subnetMask();
            
            Serial.printf("✅ Connecté au réseau '%s'\n", clientSSID);
            Serial.printf("📍 IP: %s\n", localIP.toString().c_str());
            Serial.printf("🚪 Gateway: %s\n", gatewayIP.toString().c_str());
            
            currentState = WIFI_CONNECTED;
            startWebServer();
        }
    }
    
    /**
     * Mise à jour de l'état connected
     */
    void updateConnected(uint32_t currentTime) {
        // Vérifier la connexion
        if (operatingMode == WIFI_CLIENT_MODE && WiFi.status() != WL_CONNECTED) {
            Serial.println("⚠️ Connexion WiFi perdue");
            currentState = WIFI_ERROR_STATE;
            stopWebServer();
            return;
        }
        
        // Mettre à jour les statistiques de transfert
        if (currentTransfer.active) {
            updateTransferProgress();
        }
    }
    
    /**
     * Mise à jour de l'état error
     */
    void updateError(uint32_t currentTime) {
        // Auto-recovery après 30 secondes
        if (currentTime - connectionStartTime > 30000) {
            Serial.println("🔄 Tentative de récupération...");
            currentState = WIFI_DISABLED;
            WiFi.disconnect();
        }
    }
    
    /**
     * Mise à jour du nombre de clients connectés
     */
    void updateClientCount() {
        if (operatingMode == WIFI_AP_MODE) {
            activeClients = WiFi.softAPgetStationNum();
        } else {
            activeClients = webServerActive ? 1 : 0;
        }
    }
    
    /**
     * Mise à jour du progrès de transfert
     */
    void updateTransferProgress() {
        uint32_t elapsed = millis() - currentTransfer.startTime;
        if (elapsed > 30000) { // Timeout 30 secondes
            Serial.println("⏰ Timeout transfert");
            currentTransfer.active = false;
        }
    }
    
    /**
     * Obtenir le nombre total de fichiers
     */
    uint32_t getTotalFileCount() {
        uint32_t count = 0;
        File root = SD.open(WIFI_ALLOWED_PATH);
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (!file.isDirectory() && 
                    String(file.name()).toLowerCase().endsWith(".wav")) {
                    count++;
                }
                file.close();
                file = root.openNextFile();
            }
            root.close();
        }
        return count;
    }
    
    /**
     * Obtenir la taille totale des fichiers
     */
    uint64_t getTotalFileSize() {
        uint64_t totalSize = 0;
        File root = SD.open(WIFI_ALLOWED_PATH);
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (!file.isDirectory() && 
                    String(file.name()).toLowerCase().endsWith(".wav")) {
                    totalSize += file.size();
                }
                file.close();
                file = root.openNextFile();
            }
            root.close();
        }
        return totalSize;
    }
    
    /**
     * Chargement de la configuration depuis EEPROM
     */
    bool loadConfiguration() {
        preferences.begin("wifi_mgr", true);
        
        String savedClientSSID = preferences.getString("client_ssid", "");
        String savedClientPassword = preferences.getString("client_pass", "");
        String savedAPSSID = preferences.getString("ap_ssid", WIFI_DEFAULT_SSID);
        String savedAPPassword = preferences.getString("ap_pass", WIFI_DEFAULT_PASSWORD);
        operatingMode = (WiFiMode)preferences.getUChar("mode", WIFI_CLIENT_MODE);
        
        strncpy(clientSSID, savedClientSSID.c_str(), sizeof(clientSSID) - 1);
        strncpy(clientPassword, savedClientPassword.c_str(), sizeof(clientPassword) - 1);
        strncpy(apSSID, savedAPSSID.c_str(), sizeof(apSSID) - 1);
        strncpy(apPassword, savedAPPassword.c_str(), sizeof(apPassword) - 1);
        
        preferences.end();
        
        Serial.println("Configuration WiFi chargée");
        return savedClientSSID.length() > 0;
    }
    
    /**
     * Sauvegarde de la configuration dans EEPROM
     */
    void saveConfiguration() {
        preferences.begin("wifi_mgr", false);
        
        preferences.putString("client_ssid", clientSSID);
        preferences.putString("client_pass", clientPassword);
        preferences.putString("ap_ssid", apSSID);
        preferences.putString("ap_pass", apPassword);
        preferences.putUChar("mode", operatingMode);
        
        preferences.end();
        
        Serial.println("Configuration WiFi sauvegardée");
    }

public:
    /**
     * Fonctions d'interface publique pour l'intégration
     */
    
    // États et contrôles
    bool isWiFiActive() const {
        return (currentState != WIFI_DISABLED && 
                currentState != WIFI_ERROR_STATE && 
                currentState != WIFI_AUDIO_DISABLED);
    }
    
    bool isAudioSafe() const {
        return !audioSystemActive && !isAudioSystemActive();
    }
    
    WiFiState getState() const { return currentState; }
    WiFiMode getMode() const { return operatingMode; }
    
    String getStateString() const {
        switch (currentState) {
            case WIFI_DISABLED: return "DÉSACTIVÉ";
            case WIFI_SCANNING: return "SCAN...";
            case WIFI_CONNECTING: return "CONNEXION...";
            case WIFI_CONNECTED: return "CONNECTÉ";
            case WIFI_AP_MODE: return "ACCESS POINT";
            case WIFI_SERVER_RUNNING: return "SERVEUR ACTIF";
            case WIFI_ERROR_STATE: return "ERREUR";
            case WIFI_AUDIO_DISABLED: return "AUDIO ACTIF";
            default: return "INCONNU";
        }
    }
    
    String getCurrentIP() const {
        return localIP.toString();
    }
    
    uint32_t getActiveClients() const { return activeClients; }
    uint32_t getTotalUploads() const { return totalUploads; }
    uint32_t getTotalDownloads() const { return totalDownloads; }
    uint64_t getTotalBytesTransferred() const { return totalBytesTransferred; }
    
    // Contrôles réseau
    bool scanNetworks() {
        if (audioSystemActive || isWiFiActive()) {
            return false;
        }
        
        Serial.println("📡 Scan des réseaux WiFi...");
        WiFi.mode(WIFI_STA);
        WiFi.scanNetworks(true); // Scan asynchrone
        
        currentState = WIFI_SCANNING;
        lastNetworkScan = millis();
        
        return true;
    }
    
    size_t getDetectedNetworkCount() const {
        return detectedNetworks.size();
    }
    
    const WiFiNetworkInfo* getDetectedNetwork(size_t index) const {
        if (index >= detectedNetworks.size()) return nullptr;
        return &detectedNetworks[index];
    }
    
    void selectNetwork(int index) {
        if (index >= 0 && index < detectedNetworks.size()) {
            selectedNetworkIndex = index;
        }
    }
    
    int getSelectedNetworkIndex() const {
        return selectedNetworkIndex;
    }
    
    // Configuration
    bool setClientCredentials(const char* ssid, const char* password) {
        if (strlen(ssid) == 0 || strlen(ssid) >= sizeof(clientSSID)) {
            return false;
        }
        
        strncpy(clientSSID, ssid, sizeof(clientSSID) - 1);
        strncpy(clientPassword, password, sizeof(clientPassword) - 1);
        operatingMode = WIFI_CLIENT_MODE;
        
        saveConfiguration();
        return true;
    }
    
    bool setAPCredentials(const char* ssid, const char* password) {
        if (strlen(ssid) == 0 || strlen(ssid) >= sizeof(apSSID)) {
            return false;
        }
        
        strncpy(apSSID, ssid, sizeof(apSSID) - 1);
        strncpy(apPassword, password, sizeof(apPassword) - 1);
        operatingMode = WIFI_AP_MODE_ACTIVE;
        
        saveConfiguration();
        return true;
    }
    
    void setMode(WiFiMode mode) {
        operatingMode = mode;
        saveConfiguration();
    }
    
    // Informations configuration
    const char* getClientSSID() const { return clientSSID; }
    const char* getAPSSID() const { return apSSID; }
    
    // Transferts
    bool isTransferActive() const { return currentTransfer.active; }
    const TransferProgress& getTransferProgress() const { return currentTransfer; }
    float getTransferProgressPercent() const {
        if (!currentTransfer.active || currentTransfer.totalBytes == 0) return 0.0f;
        return (float)currentTransfer.transferredBytes / currentTransfer.totalBytes * 100.0f;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////
//                               INTERFACE TACTILE WiFi MANAGER                          //
////////////////////////////////////////////////////////////////////////////////////////////

// Variables d'état de l'interface
bool wifiInterfaceNeedsRedraw = true;
uint32_t wifiLastInterfaceUpdate = 0;
int wifiSelectedMenuItem = 0;
bool wifiShowKeyboard = false;
char wifiKeyboardBuffer[64] = "";
int wifiKeyboardCursor = 0;

/**
 * Zones tactiles WiFi Manager (correspondant aux spécifications)
 */
#define WIFI_ZONE_NETWORK_1     16    // Réseau détecté 1
#define WIFI_ZONE_NETWORK_2     17    // Réseau détecté 2
#define WIFI_ZONE_NETWORK_3     18    // Réseau détecté 3
#define WIFI_ZONE_NETWORK_4     19    // Réseau détecté 4
#define WIFI_ZONE_CLIENT_MODE   20    // Mode client WiFi
#define WIFI_ZONE_AP_MODE       21    // Mode Access Point
#define WIFI_ZONE_START_SERVER  22    // Démarrage serveur web
#define WIFI_ZONE_STOP_WIFI     23    // Arrêt complet WiFi
#define WIFI_ZONE_RETOUR        24    // Retour menu principal
#define WIFI_ZONE_SCAN_NETWORKS 25    // Scan réseaux disponibles
#define WIFI_ZONE_SETTINGS      28    // Configuration avancée

/**
 * Dessiner l'interface WiFi Manager
 */
void drawWifiManager() {
    if (!wifiInterfaceNeedsRedraw && millis() - wifiLastInterfaceUpdate < 1000) {
        return; // Pas besoin de redessiner
    }
    
    gfx->fillScreen(BLACK);
    
    // En-tête
    drawWifiHeader();
    
    // Barre de statut
    drawWifiStatusBar();
    
    // Interface principale selon l'état
    WiFiState state = wifiManager.getState();
    
    switch (state) {
        case WIFI_DISABLED:
        case WIFI_AUDIO_DISABLED:
            drawWifiDisabledInterface();
            break;
        case WIFI_SCANNING:
            drawWifiScanningInterface();
            break;
        case WIFI_CONNECTING:
            drawWifiConnectingInterface();
            break;
        case WIFI_CONNECTED:
        case WIFI_AP_MODE:
        case WIFI_SERVER_RUNNING:
            drawWifiActiveInterface();
            break;
        case WIFI_ERROR_STATE:
            drawWifiErrorInterface();
            break;
    }
    
    // Clavier virtuel si affiché
    if (wifiShowKeyboard) {
        drawWifiKeyboard();
    }
    
    // Boutons de contrôle
    drawWifiControlButtons();
    
    wifiInterfaceNeedsRedraw = false;
    wifiLastInterfaceUpdate = millis();
}

/**
 * En-tête WiFi Manager
 */
void drawWifiHeader() {
    gfx->setTextColor(ZCYAN, BLACK);
    gfx->setCursor(150, 15);
    gfx->print("WiFi FILE MANAGER");
    
    // Indicateur d'état principal
    WiFiState state = wifiManager.getState();
    int stateColor = DARKGREY;
    
    switch (state) {
        case WIFI_CONNECTED:
        case WIFI_SERVER_RUNNING:
            stateColor = ZGREEN;
            break;
        case WIFI_AP_MODE:
            stateColor = ZBLUE;
            break;
        case WIFI_CONNECTING:
        case WIFI_SCANNING:
            stateColor = ZYELLOW;
            break;
        case WIFI_ERROR_STATE:
            stateColor = ZRED;
            break;
        case WIFI_AUDIO_DISABLED:
            stateColor = ZMAGENTA;
            break;
        default:
            stateColor = DARKGREY;
            break;
    }
    
    gfx->setTextColor(stateColor, BLACK);
    gfx->setCursor(350, 15);
    gfx->print(wifiManager.getStateString());
}

/**
 * Barre de statut WiFi
 */
void drawWifiStatusBar() {
    int statusY = 35;
    
    gfx->drawLine(0, statusY - 5, 479, statusY - 5, DARKGREY);
    gfx->fillRect(0, statusY, 480, 25, BLACK);
    
    // Statut réseau
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(10, statusY + 10);
    gfx->print("Réseau: ");
    
    if (wifiManager.isWiFiActive()) {
        gfx->setTextColor(ZGREEN, BLACK);
        gfx->print(wifiManager.getCurrentIP());
    } else {
        gfx->setTextColor(DARKGREY, BLACK);
        gfx->print("INACTIF");
    }
    
    // Statut SD Card
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(180, statusY + 10);
    gfx->print("SD: ");
    gfx->setTextColor(sdIsReady() ? ZGREEN : ZRED, BLACK);
    gfx->print(sdIsReady() ? "OK" : "ERREUR");
    
    // Statut Audio
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(250, statusY + 10);
    gfx->print("Audio: ");
    gfx->setTextColor(wifiManager.isAudioSafe() ? DARKGREY : ZRED, BLACK);
    gfx->print(wifiManager.isAudioSafe() ? "LIBRE" : "ACTIF");
    
    // Clients connectés
    if (wifiManager.isWiFiActive()) {
        gfx->setTextColor(LIGHTGREY, BLACK);
        gfx->setCursor(350, statusY + 10);
        gfx->print("Clients: ");
        gfx->setTextColor(ZCYAN, BLACK);
        gfx->print(wifiManager.getActiveClients());
    }
}

/**
 * Interface WiFi désactivé
 */
void drawWifiDisabledInterface() {
    int yPos = 80;
    
    // Message principal
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, yPos);
    gfx->print("WiFi désactivé - Sélectionnez un mode :");
    
    yPos += 30;
    
    // Options de mode
    gfx->setCursor(20, yPos);
    gfx->print("Mode actuel: ");
    gfx->setTextColor(ZCYAN, BLACK);
    WiFiMode mode = wifiManager.getMode();
    gfx->print(mode == WIFI_CLIENT_MODE ? "CLIENT" : "ACCESS POINT");
    
    yPos += 30;
    
    // Configuration client
    if (mode == WIFI_CLIENT_MODE) {
        gfx->setTextColor(LIGHTGREY, BLACK);
        gfx->setCursor(20, yPos);
        gfx->print("SSID configuré: ");
        gfx->setTextColor(ZGREEN, BLACK);
        const char* ssid = wifiManager.getClientSSID();
        gfx->print(strlen(ssid) > 0 ? ssid : "Aucun");
        
        yPos += 20;
        gfx->setTextColor(LIGHTGREY, BLACK);
        gfx->setCursor(20, yPos);
        gfx->print("Réseaux détectés: ");
        gfx->setTextColor(ZYELLOW, BLACK);
        gfx->print(wifiManager.getDetectedNetworkCount());
    } else {
        gfx->setTextColor(LIGHTGREY, BLACK);
        gfx->setCursor(20, yPos);
        gfx->print("SSID AP: ");
        gfx->setTextColor(ZBLUE, BLACK);
        gfx->print(wifiManager.getAPSSID());
    }
    
    // Liste des réseaux détectés (mode client)
    if (mode == WIFI_CLIENT_MODE && wifiManager.getDetectedNetworkCount() > 0) {
        yPos += 40;
        gfx->setTextColor(ZYELLOW, BLACK);
        gfx->setCursor(20, yPos);
        gfx->print("Réseaux disponibles :");
        
        for (size_t i = 0; i < min(4, wifiManager.getDetectedNetworkCount()); i++) {
            const WiFiNetworkInfo* network = wifiManager.getDetectedNetwork(i);
            if (network) {
                yPos += 20;
                
                bool selected = (wifiManager.getSelectedNetworkIndex() == (int)i);
                gfx->setTextColor(selected ? ZGREEN : LIGHTGREY, BLACK);
                gfx->setCursor(30, yPos);
                
                // Signal strength bars
                int bars = map(network->rssi, -100, -30, 1, 5);
                for (int b = 0; b < 5; b++) {
                    gfx->print(b < bars ? "█" : "░");
                }
                
                gfx->print(" ");
                gfx->print(network->ssid);
                
                if (network->isOpen) {
                    gfx->setTextColor(DARKGREY, BLACK);
                    gfx->print(" (Ouvert)");
                } else {
                    gfx->setTextColor(ZRED, BLACK);
                    gfx->print(" 🔒");
                }
            }
        }
    }
}

/**
 * Interface scan en cours
 */
void drawWifiScanningInterface() {
    int yPos = 100;
    
    gfx->setTextColor(ZYELLOW, BLACK);
    gfx->setCursor(150, yPos);
    gfx->print("📡 Scan en cours...");
    
    // Animation simple
    int dots = (millis() / 500) % 4;
    yPos += 30;
    gfx->setCursor(180, yPos);
    for (int i = 0; i < dots; i++) {
        gfx->print(".");
    }
}

/**
 * Interface connexion en cours
 */
void drawWifiConnectingInterface() {
    int yPos = 100;
    
    gfx->setTextColor(ZYELLOW, BLACK);
    gfx->setCursor(150, yPos);
    gfx->print("🔗 Connexion...");
    
    yPos += 30;
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(100, yPos);
    gfx->print("Réseau: ");
    gfx->setTextColor(ZCYAN, BLACK);
    gfx->print(wifiManager.getClientSSID());
}

/**
 * Interface WiFi actif
 */
void drawWifiActiveInterface() {
    int yPos = 80;
    
    // Statut de connexion
    gfx->setTextColor(ZGREEN, BLACK);
    gfx->setCursor(20, yPos);
    gfx->print("🌐 WiFi Actif");
    
    yPos += 25;
    
    // Informations de connexion
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, yPos);
    gfx->print("IP: ");
    gfx->setTextColor(ZCYAN, BLACK);
    gfx->print(wifiManager.getCurrentIP());
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(200, yPos);
    gfx->print("Port: 80");
    
    yPos += 25;
    
    // URL d'accès
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(20, yPos);
    gfx->print("URL: http://");
    gfx->setTextColor(ZGREEN, BLACK);
    gfx->print(wifiManager.getCurrentIP());
    
    yPos += 35;
    
    // Statistiques de transfert
    gfx->drawLine(0, yPos - 5, 479, yPos - 5, DARKGREY);
    
    gfx->setTextColor(ZCYAN, BLACK);
    gfx->setCursor(20, yPos + 10);
    gfx->print("📊 Statistiques :");
    
    yPos += 30;
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(30, yPos);
    gfx->print("Uploads: ");
    gfx->setTextColor(ZGREEN, BLACK);
    gfx->print(wifiManager.getTotalUploads());
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(150, yPos);
    gfx->print("Downloads: ");
    gfx->setTextColor(ZBLUE, BLACK);
    gfx->print(wifiManager.getTotalDownloads());
    
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(280, yPos);
    gfx->print("Clients: ");
    gfx->setTextColor(ZYELLOW, BLACK);
    gfx->print(wifiManager.getActiveClients());
    
    // Progrès de transfert actif
    if (wifiManager.isTransferActive()) {
        const TransferProgress& progress = wifiManager.getTransferProgress();
        
        yPos += 30;
        gfx->setTextColor(ZYELLOW, BLACK);
        gfx->setCursor(20, yPos);
        gfx->print(progress.isUpload ? "📤 Upload: " : "📥 Download: ");
        gfx->print(progress.filename);
        
        yPos += 20;
        
        // Barre de progression
        int progressWidth = 400;
        int progressHeight = 8;
        int progressX = 40;
        
        gfx->drawRect(progressX - 1, yPos - 1, progressWidth + 2, progressHeight + 2, DARKGREY);
        gfx->fillRect(progressX, yPos, progressWidth, progressHeight, BLACK);
        
        float progressPercent = wifiManager.getTransferProgressPercent();
        int progressFill = (int)(progressWidth * progressPercent / 100.0f);
        
        gfx->fillRect(progressX, yPos, progressFill, progressHeight, ZGREEN);
        
        // Pourcentage
        gfx->setTextColor(LIGHTGREY, BLACK);
        gfx->setCursor(progressX + progressWidth + 10, yPos + 5);
        gfx->printf("%.1f%%", progressPercent);
    }
}

/**
 * Interface erreur WiFi
 */
void drawWifiErrorInterface() {
    int yPos = 100;
    
    gfx->setTextColor(ZRED, BLACK);
    gfx->setCursor(150, yPos);
    gfx->print("❌ Erreur WiFi");
    
    yPos += 30;
    gfx->setTextColor(LIGHTGREY, BLACK);
    gfx->setCursor(80, yPos);
    gfx->print("Vérifiez la configuration réseau");
    
    yPos += 20;
    gfx->setCursor(120, yPos);
    gfx->print("Récupération automatique...");
}

/**
 * Boutons de contrôle WiFi
 */
void drawWifiControlButtons() {
    WiFiState state = wifiManager.getState();
    bool audioActive = !wifiManager.isAudioSafe();
    
    // Bouton SCAN (zone 25)
    int scanColor = DARKGREY;
    if (state == WIFI_DISABLED && !audioActive) {
        scanColor = ZYELLOW;
    }
    drawBT(WIFI_ZONE_SCAN_NETWORKS, scanColor, "SCAN");
    
    // Bouton CLIENT MODE (zone 20)
    int clientColor = (wifiManager.getMode() == WIFI_CLIENT_MODE) ? ZGREEN : DARKGREY;
    drawBT(WIFI_ZONE_CLIENT_MODE, clientColor, "CLIENT");
    
    // Bouton AP MODE (zone 21)
    int apColor = (wifiManager.getMode() == WIFI_AP_MODE_ACTIVE) ? ZBLUE : DARKGREY;
    drawBT(WIFI_ZONE_AP_MODE, apColor, "ACCESS POINT");
    
    // Bouton START/STOP (zone 22)
    if (wifiManager.isWiFiActive()) {
        drawBT(WIFI_ZONE_START_SERVER, ZRED, "ARRÊTER");
    } else {
        int startColor = audioActive ? DARKGREY : ZGREEN;
        drawBT(WIFI_ZONE_START_SERVER, startColor, "DÉMARRER");
    }
    
    // Bouton RETOUR (zone 24)
    drawBT(WIFI_ZONE_RETOUR, DARKGREY, "RETOUR");
    
    // Bouton SETTINGS (zone 28)
    drawBT(WIFI_ZONE_SETTINGS, DARKGREY, "CONFIG");
}

/**
 * Clavier virtuel WiFi (pour saisie SSID/mot de passe)
 */
void drawWifiKeyboard() {
    // Overlay semi-transparent
    gfx->fillRect(0, 150, 480, 170, OSCURO);
    gfx->drawRect(10, 160, 460, 150, ZYELLOW);
    
    gfx->setTextColor(ZYELLOW, OSCURO);
    gfx->setCursor(20, 175);
    gfx->print("CONFIGURATION RÉSEAU");
    
    // Zone de saisie
    gfx->drawRect(20, 190, 400, 25, DARKGREY);
    gfx->fillRect(21, 191, 398, 23, BLACK);
    
    gfx->setTextColor(ZGREEN, BLACK);
    gfx->setCursor(25, 200);
    gfx->print(wifiKeyboardBuffer);
    
    // Curseur clignotant
    if ((millis() / 500) % 2) {
        int cursorX = 25 + (strlen(wifiKeyboardBuffer) * 6);
        gfx->drawLine(cursorX, 195, cursorX, 210, ZGREEN);
    }
    
    // Instructions
    gfx->setTextColor(LIGHTGREY, OSCURO);
    gfx->setCursor(20, 225);
    gfx->print("Utilisez les pads 0-15 pour saisir");
    gfx->setCursor(20, 240);
    gfx->print("SCAN=Retour  START=Valider  STOP=Effacer");
}

/**
 * Gestion des touches tactiles WiFi Manager
 */
void wifiManagerHandleTouch(uint16_t zone) {
    WiFiState state = wifiManager.getState();
    bool audioActive = !wifiManager.isAudioSafe();
    
    Serial.printf("WiFi Manager - Touch zone: %d (state: %d)\n", zone, state);
    
    // Si le clavier est affiché, gérer la saisie
    if (wifiShowKeyboard) {
        handleWifiKeyboardTouch(zone);
        return;
    }
    
    // Gestion selon les zones
    switch (zone) {
        case WIFI_ZONE_NETWORK_1:
        case WIFI_ZONE_NETWORK_2:
        case WIFI_ZONE_NETWORK_3:
        case WIFI_ZONE_NETWORK_4:
            handleNetworkSelection(zone - WIFI_ZONE_NETWORK_1);
            break;
            
        case WIFI_ZONE_CLIENT_MODE:
            if (state == WIFI_DISABLED && !audioActive) {
                wifiManager.setMode(WIFI_CLIENT_MODE);
                wifiInterfaceNeedsRedraw = true;
            }
            break;
            
        case WIFI_ZONE_AP_MODE:
            if (state == WIFI_DISABLED && !audioActive) {
                wifiManager.setMode(WIFI_AP_MODE_ACTIVE);
                wifiInterfaceNeedsRedraw = true;
            }
            break;
            
        case WIFI_ZONE_START_SERVER:
            if (audioActive) {
                Serial.println("❌ Impossible - Audio actif");
                break;
            }
            
            if (wifiManager.isWiFiActive()) {
                wifiManager.stopWiFi();
            } else {
                wifiManager.startWiFi();
            }
            wifiInterfaceNeedsRedraw = true;
            break;
            
        case WIFI_ZONE_STOP_WIFI:
            if (wifiManager.isWiFiActive()) {
                wifiManager.stopWiFi();
                wifiInterfaceNeedsRedraw = true;
            }
            break;
            
        case WIFI_ZONE_RETOUR:
            // Arrêter le WiFi si actif avant de retourner au menu
            if (wifiManager.isWiFiActive()) {
                wifiManager.stopWiFi();
            }
            returnToMainMenu();
            break;
            
        case WIFI_ZONE_SCAN_NETWORKS:
            if (state == WIFI_DISABLED && !audioActive) {
                wifiManager.scanNetworks();
                wifiInterfaceNeedsRedraw = true;
            }
            break;
            
        case WIFI_ZONE_SETTINGS:
            if (state == WIFI_DISABLED && !audioActive) {
                showWifiKeyboard("Entrez SSID:");
            }
            break;
    }
}

/**
 * Gestion de la sélection d'un réseau
 */
void handleNetworkSelection(int networkIndex) {
    if (wifiManager.getDetectedNetworkCount() == 0) return;
    
    const WiFiNetworkInfo* network = wifiManager.getDetectedNetwork(networkIndex);
    if (!network) return;
    
    wifiManager.selectNetwork(networkIndex);
    
    if (network->isOpen) {
        // Réseau ouvert - connexion directe
        wifiManager.setClientCredentials(network->ssid.c_str(), "");
        wifiInterfaceNeedsRedraw = true;
    } else {
        // Réseau sécurisé - demander le mot de passe
        strncpy(wifiKeyboardBuffer, network->ssid.c_str(), sizeof(wifiKeyboardBuffer) - 1);
        showWifiKeyboard("Mot de passe:");
    }
}

/**
 * Afficher le clavier virtuel
 */
void showWifiKeyboard(const char* prompt) {
    wifiShowKeyboard = true;
    wifiKeyboardCursor = strlen(wifiKeyboardBuffer);
    wifiInterfaceNeedsRedraw = true;
}

/**
 * Masquer le clavier virtuel
 */
void hideWifiKeyboard() {
    wifiShowKeyboard = false;
    memset(wifiKeyboardBuffer, 0, sizeof(wifiKeyboardBuffer));
    wifiKeyboardCursor = 0;
    wifiInterfaceNeedsRedraw = true;
}

/**
 * Gestion des touches du clavier virtuel
 */
void handleWifiKeyboardTouch(uint16_t zone) {
    // Clavier simple utilisant les pads 0-15 pour caractères de base
    const char keyMap[] = "0123456789ABCDEF";
    
    if (zone >= 0 && zone <= 15) {
        // Caractères
        if (strlen(wifiKeyboardBuffer) < sizeof(wifiKeyboardBuffer) - 1) {
            char newChar = keyMap[zone];
            strncat(wifiKeyboardBuffer, &newChar, 1);
            wifiKeyboardCursor++;
        }
    } else if (zone == WIFI_ZONE_SCAN_NETWORKS) {
        // Retour/Annuler
        hideWifiKeyboard();
    } else if (zone == WIFI_ZONE_START_SERVER) {
        // Valider
        validateWifiKeyboardInput();
        hideWifiKeyboard();
    } else if (zone == WIFI_ZONE_STOP_WIFI) {
        // Effacer
        if (strlen(wifiKeyboardBuffer) > 0) {
            wifiKeyboardBuffer[strlen(wifiKeyboardBuffer) - 1] = '\0';
            wifiKeyboardCursor--;
        }
    }
    
    wifiInterfaceNeedsRedraw = true;
}

/**
 * Valider la saisie du clavier
 */
void validateWifiKeyboardInput() {
    if (strlen(wifiKeyboardBuffer) == 0) return;
    
    int selectedNetwork = wifiManager.getSelectedNetworkIndex();
    if (selectedNetwork >= 0) {
        const WiFiNetworkInfo* network = wifiManager.getDetectedNetwork(selectedNetwork);
        if (network) {
            // Configurer les identifiants client
            wifiManager.setClientCredentials(network->ssid.c_str(), wifiKeyboardBuffer);
            Serial.printf("✅ Réseau configuré: %s\n", network->ssid.c_str());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                               FONCTIONS D'INTÉGRATION                                 //
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Fonction appelée depuis keys.ino pour gérer les touches WiFi
 */
void wifiManagerHandleTouch(uint16_t zone);

/**
 * Fonction appelée depuis menu_system.ino pour dessiner l'interface
 */
void drawWifiSettings() {
    drawWifiManager();
}

/**
 * Mise à jour périodique (appelée depuis loop principal)
 */
void wifiManagerUpdate() {
    wifiManager.update();
}

/**
 * Initialisation (appelée depuis setup)
 */
bool wifiManagerBegin() {
    return wifiManager.begin();
}

/**
 * Vérifier si WiFi est actif (pour autres modules)
 */
bool isWifiManagerActive() {
    return wifiManager.isWiFiActive();
}

/**
 * Marquer activité audio (appelée depuis synthESP32 et autres)
 */
void markAudioActivity() {
    lastAudioActivity = millis();
}

/**
 * Forcer arrêt WiFi (urgence audio)
 */
void forceStopWifiManager() {
    wifiManager.forceStopWiFi();
    wifiInterfaceNeedsRedraw = true;
}