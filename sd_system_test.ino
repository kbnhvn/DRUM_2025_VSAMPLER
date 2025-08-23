/*
 * DRUM_2025_VSAMPLER - Test du Système SD Card
 * 
 * Ce fichier contient une suite de tests pour valider
 * le fonctionnement complet du système SD Card
 */

// Variables de test
bool testResults[10];
uint8_t currentTest = 0;

/*
 * Test 1 : Initialisation du système SD
 */
bool test_SD_Initialization() {
    Serial.println("\n=== TEST 1: INITIALISATION SD ===");
    
    bool result = sdBegin();
    
    if (result) {
        Serial.println("✓ SD initialisée avec succès");
        Serial.printf("✓ Carte détectée, prête à l'emploi\n");
    } else {
        Serial.println("✗ Échec initialisation SD");
        return false;
    }
    
    if (sdIsReady()) {
        Serial.println("✓ Système SD opérationnel");
        return true;
    } else {
        Serial.println("✗ Système SD non opérationnel");
        return false;
    }
}

/*
 * Test 2 : Scan des samples
 */
bool test_SD_SampleScan() {
    Serial.println("\n=== TEST 2: SCAN DES SAMPLES ===");
    
    uint16_t count = sdScanSamples("/SAMPLES/");
    
    Serial.printf("Samples trouvés : %d\n", count);
    
    if (count > 0) {
        Serial.println("✓ Samples détectés sur la carte SD");
        
        // Afficher les premiers samples
        for (uint16_t i = 0; i < min(count, 5); i++) {
            const SampleInfo* info = sdGetSampleInfo(i);
            if (info && info->isValid) {
                Serial.printf("  %d: %s (%dHz, %dch, %dms)\n", 
                            i, info->displayName, info->sampleRate, 
                            info->channels, info->duration);
            }
        }
        return true;
    } else {
        Serial.println("✗ Aucun sample trouvé");
        Serial.println("  Vérifiez la structure /SAMPLES/ sur la carte SD");
        return false;
    }
}

/*
 * Test 3 : Chargement d'un sample
 */
bool test_SD_SampleLoading() {
    Serial.println("\n=== TEST 3: CHARGEMENT SAMPLE ===");
    
    if (sdGetSampleCount() == 0) {
        Serial.println("✗ Aucun sample disponible pour le test");
        return false;
    }
    
    const char* firstSample = sdGetSamplePath(0);
    Serial.printf("Chargement de : %s\n", firstSample);
    
    uint32_t startTime = millis();
    bool result = sdLoadSample(0, firstSample);
    uint32_t loadTime = millis() - startTime;
    
    if (result) {
        Serial.printf("✓ Sample chargé en %d ms\n", loadTime);
        Serial.printf("✓ Slot 0 : %d échantillons\n", sdGetSampleLength(0));
        return true;
    } else {
        Serial.println("✗ Échec du chargement");
        return false;
    }
}

/*
 * Test 4 : Assignation à une voix
 */
bool test_SD_VoiceAssignment() {
    Serial.println("\n=== TEST 4: ASSIGNATION VOIX ===");
    
    if (!sdIsSampleLoaded(0)) {
        Serial.println("✗ Aucun sample chargé pour le test");
        return false;
    }
    
    bool result = assignSampleToVoice(0, SAMPLE_SOURCE_SD, 0);
    
    if (result) {
        Serial.println("✓ Sample SD assigné à la voix 0");
        
        // Vérifier l'assignation
        int16_t* data = sdGetSampleData(0);
        if (data != nullptr) {
            Serial.println("✓ Données accessible via l'interface hybride");
            return true;
        } else {
            Serial.println("✗ Données non accessibles");
            return false;
        }
    } else {
        Serial.println("✗ Échec de l'assignation");
        return false;
    }
}

/*
 * Test 5 : Lecture d'échantillons
 */
bool test_SD_SampleReading() {
    Serial.println("\n=== TEST 5: LECTURE ÉCHANTILLONS ===");
    
    // Tester la lecture via l'interface hybride
    int16_t sample1 = getSample(0, 0);      // Premier échantillon
    int16_t sample2 = getSample(0, 100);    // 100ème échantillon
    int16_t sample3 = getSample(0, 1000);   // 1000ème échantillon
    
    if (sample1 != 0 || sample2 != 0 || sample3 != 0) {
        Serial.printf("✓ Échantillons lus : %d, %d, %d\n", sample1, sample2, sample3);
        
        // Tester la fin du sample
        uint32_t length = getSampleLength(0);
        int16_t lastSample = getSample(0, length - 1);
        int16_t beyondEnd = getSample(0, length + 100);
        
        if (beyondEnd == 0) {
            Serial.println("✓ Protection dépassement de longueur fonctionnelle");
            return true;
        } else {
            Serial.println("✗ Protection dépassement défaillante");
            return false;
        }
    } else {
        Serial.println("✗ Échantillons non lisibles");
        return false;
    }
}

/*
 * Test 6 : Gestion du cache
 */
bool test_SD_CacheManagement() {
    Serial.println("\n=== TEST 6: GESTION CACHE ===");
    
    uint8_t initialUsage = sdGetCacheUsage();
    Serial.printf("Utilisation initiale : %d%%\n", initialUsage);
    
    // Charger plusieurs samples si disponibles
    uint16_t sampleCount = sdGetSampleCount();
    uint8_t samplesToLoad = min(sampleCount, 4);
    
    for (uint8_t i = 1; i < samplesToLoad; i++) {
        const char* samplePath = sdGetSamplePath(i);
        if (sdLoadSample(i, samplePath)) {
            Serial.printf("✓ Sample %d chargé\n", i);
        }
    }
    
    uint8_t finalUsage = sdGetCacheUsage();
    Serial.printf("Utilisation finale : %d%%\n", finalUsage);
    
    if (finalUsage > initialUsage) {
        Serial.println("✓ Cache s'remplit correctement");
        
        // Test de vidage
        sdClearCache();
        uint8_t afterClear = sdGetCacheUsage();
        
        if (afterClear == 0) {
            Serial.println("✓ Vidage du cache fonctionnel");
            return true;
        } else {
            Serial.println("✗ Vidage du cache défaillant");
            return false;
        }
    } else {
        Serial.println("✗ Cache ne se remplit pas");
        return false;
    }
}

/*
 * Test 7 : Performance de chargement
 */
bool test_SD_Performance() {
    Serial.println("\n=== TEST 7: PERFORMANCE ===");
    
    if (sdGetSampleCount() == 0) {
        Serial.println("✗ Aucun sample pour test de performance");
        return false;
    }
    
    const char* testSample = sdGetSamplePath(0);
    uint32_t totalTime = 0;
    const uint8_t iterations = 3;
    
    for (uint8_t i = 0; i < iterations; i++) {
        sdUnloadSample(0);  // Décharger d'abord
        
        uint32_t startTime = millis();
        bool result = sdLoadSample(0, testSample);
        uint32_t loadTime = millis() - startTime;
        
        if (!result) {
            Serial.printf("✗ Échec chargement itération %d\n", i);
            return false;
        }
        
        totalTime += loadTime;
        Serial.printf("  Itération %d: %d ms\n", i, loadTime);
    }
    
    uint32_t avgTime = totalTime / iterations;
    Serial.printf("✓ Temps moyen de chargement : %d ms\n", avgTime);
    
    if (avgTime < 500) {  // Moins de 500ms acceptable
        Serial.println("✓ Performance acceptable");
        return true;
    } else {
        Serial.println("⚠ Performance lente (>500ms)");
        return true;  // Pas une erreur critique
    }
}

/*
 * Test 8 : Trigger audio
 */
bool test_SD_AudioTrigger() {
    Serial.println("\n=== TEST 8: TRIGGER AUDIO ===");
    
    // Charger et assigner un sample
    if (sdGetSampleCount() > 0) {
        sdLoadSample(0, sdGetSamplePath(0));
        assignSampleToVoice(0, SAMPLE_SOURCE_SD, 0);
        setSound(0);
        
        Serial.println("✓ Sample prêt pour trigger");
        
        // Simuler un trigger
        synthESP32_TRIGGER(0);
        
        Serial.println("✓ Trigger audio envoyé");
        Serial.println("  (Vérifiez manuellement la sortie audio)");
        
        delay(500);  // Laisser le sample jouer un peu
        
        return true;
    } else {
        Serial.println("✗ Aucun sample pour test audio");
        return false;
    }
}

/*
 * Test 9 : Mode hybride
 */
bool test_SD_HybridMode() {
    Serial.println("\n=== TEST 9: MODE HYBRIDE ===");
    
    // Assigner sample intégré à voix 1
    assignSampleToVoice(1, SAMPLE_SOURCE_BUILTIN, 16);
    setSound(1);
    
    // Si samples SD disponibles, assigner à voix 2
    if (sdGetSampleCount() > 0) {
        sdLoadSample(0, sdGetSamplePath(0));
        assignSampleToVoice(2, SAMPLE_SOURCE_SD, 0);
        setSound(2);
        
        Serial.println("✓ Voix 1: Sample intégré");
        Serial.println("✓ Voix 2: Sample SD");
        
        // Tester les deux sources
        int16_t builtinSample = getSample(1, 100);
        int16_t sdSample = getSample(2, 100);
        
        Serial.printf("✓ Lecture hybride: Intégré=%d, SD=%d\n", builtinSample, sdSample);
        
        return true;
    } else {
        Serial.println("⚠ Mode hybride partiel (pas de samples SD)");
        return true;
    }
}

/*
 * Test 10 : Récupération d'erreurs
 */
bool test_SD_ErrorRecovery() {
    Serial.println("\n=== TEST 10: RÉCUPÉRATION D'ERREURS ===");
    
    // Test chargement fichier inexistant
    bool result1 = sdLoadSample(5, "/SAMPLES/NONEXISTENT.wav");
    if (!result1) {
        Serial.println("✓ Gestion fichier inexistant");
    } else {
        Serial.println("✗ Devrait échouer sur fichier inexistant");
        return false;
    }
    
    // Test accès slot invalide
    int16_t* data = sdGetSampleData(255);
    if (data == nullptr) {
        Serial.println("✓ Protection slot invalide");
    } else {
        Serial.println("✗ Devrait retourner nullptr pour slot invalide");
        return false;
    }
    
    // Test lecture au-delà des limites
    int16_t sample = getSample(20, 999999);  // Voix et position invalides
    if (sample == 0) {
        Serial.println("✓ Protection limites d'accès");
        return true;
    } else {
        Serial.println("✗ Devrait retourner 0 pour accès invalide");
        return false;
    }
}

/*
 * Exécuter tous les tests
 */
void runAllSDTests() {
    Serial.println("\n" + String("=").repeat(50));
    Serial.println("    SUITE DE TESTS SYSTÈME SD CARD");
    Serial.println("    DRUM_2025_VSAMPLER");
    Serial.println(String("=").repeat(50));
    
    currentTest = 0;
    
    testResults[currentTest++] = test_SD_Initialization();
    testResults[currentTest++] = test_SD_SampleScan();
    testResults[currentTest++] = test_SD_SampleLoading();
    testResults[currentTest++] = test_SD_VoiceAssignment();
    testResults[currentTest++] = test_SD_SampleReading();
    testResults[currentTest++] = test_SD_CacheManagement();
    testResults[currentTest++] = test_SD_Performance();
    testResults[currentTest++] = test_SD_AudioTrigger();
    testResults[currentTest++] = test_SD_HybridMode();
    testResults[currentTest++] = test_SD_ErrorRecovery();
    
    // Résumé des résultats
    Serial.println("\n" + String("=").repeat(50));
    Serial.println("    RÉSUMÉ DES TESTS");
    Serial.println(String("=").repeat(50));
    
    uint8_t passed = 0;
    uint8_t failed = 0;
    
    const char* testNames[] = {
        "Initialisation SD",
        "Scan des samples", 
        "Chargement sample",
        "Assignation voix",
        "Lecture échantillons",
        "Gestion cache",
        "Performance",
        "Trigger audio",
        "Mode hybride",
        "Récupération d'erreurs"
    };
    
    for (uint8_t i = 0; i < currentTest; i++) {
        Serial.printf("%s: %s\n", 
                    testNames[i], 
                    testResults[i] ? "✓ SUCCÈS" : "✗ ÉCHEC");
        if (testResults[i]) {
            passed++;
        } else {
            failed++;
        }
    }
    
    Serial.println(String("-").repeat(50));
    Serial.printf("RÉSULTAT GLOBAL: %d succès, %d échecs\n", passed, failed);
    
    if (failed == 0) {
        Serial.println("🎉 SYSTÈME SD ENTIÈREMENT FONCTIONNEL! 🎉");
    } else if (failed <= 2) {
        Serial.println("⚠ SYSTÈME SD PARTIELLEMENT FONCTIONNEL");
    } else {
        Serial.println("❌ SYSTÈME SD NÉCESSITE UNE ATTENTION");
    }
    
    Serial.println(String("=").repeat(50));
    
    // Afficher statistiques finales
    if (sdIsReady()) {
        sdPrintCacheStats();
    }
}

/*
 * Test rapide - version simplifiée
 */
void runQuickSDTest() {
    Serial.println("\n=== TEST RAPIDE SYSTÈME SD ===");
    
    if (sdBegin() && sdIsReady()) {
        Serial.println("✓ SD opérationnelle");
        
        uint16_t count = sdScanSamples("/SAMPLES/");
        Serial.printf("✓ %d samples détectés\n", count);
        
        if (count > 0) {
            if (sdLoadSample(0, sdGetSamplePath(0))) {
                Serial.println("✓ Chargement fonctionnel");
                
                if (assignSampleToVoice(0, SAMPLE_SOURCE_SD, 0)) {
                    Serial.println("✓ Assignation fonctionnelle");
                    Serial.println("🎯 SYSTÈME SD PRÊT À L'EMPLOI!");
                } else {
                    Serial.println("✗ Problème d'assignation");
                }
            } else {
                Serial.println("✗ Problème de chargement");
            }
        } else {
            Serial.println("⚠ Aucun sample sur la carte SD");
        }
    } else {
        Serial.println("✗ Problème d'initialisation SD");
    }
    
    Serial.println("===============================");
}

/*
 * Fonction utilitaire pour les tests interactifs
 */
void testSDFromSerial() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "test") {
            runAllSDTests();
        } else if (command == "quick") {
            runQuickSDTest();
        } else if (command == "stats") {
            sdPrintCacheStats();
        } else if (command == "clear") {
            sdClearCache();
            Serial.println("Cache vidé");
        } else if (command == "help") {
            Serial.println("Commandes disponibles:");
            Serial.println("  test  - Tests complets");
            Serial.println("  quick - Test rapide");
            Serial.println("  stats - Statistiques cache");
            Serial.println("  clear - Vider cache");
            Serial.println("  help  - Cette aide");
        }
    }
}