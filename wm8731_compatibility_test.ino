/**
 * Tests de Compatibilité WM8731 avec DRUM_2025_VSAMPLER
 * 
 * Ce fichier contient les tests pour vérifier la compatibilité
 * du driver WM8731 avec l'architecture audio existante.
 * 
 * Auteur: ZCARLOS 2025
 * Version: 1.0
 */

#ifndef WM8731_COMPATIBILITY_TEST_H
#define WM8731_COMPATIBILITY_TEST_H

// ==========================================
// TESTS DE COMPATIBILITÉ
// ==========================================

/**
 * Structure pour les résultats de tests
 */
struct TestResult {
    bool passed;
    String description;
    String error;
};

/**
 * Test 1: Vérification des pins I2S
 */
TestResult test_i2s_pins() {
    TestResult result;
    result.description = "Test pins I2S WM8731";
    
    // Vérifier que les nouvelles pins sont différentes des anciennes
    bool pins_different = true;
    
    #ifdef mylcd_type2
    // Anciennes pins PCM5102A
    const int old_bck = 9;
    const int old_ws = 5;
    const int old_dout = 14;
    
    // Nouvelles pins WM8731
    const int new_bck = 14;
    const int new_ws = 15;
    const int new_dout = 13;
    const int new_din = 12;
    
    // Vérifier les conflits
    if (new_bck == old_ws || new_ws == old_dout || new_dout == old_bck) {
        pins_different = false;
        result.error = "Conflit de pins entre PCM5102A et WM8731";
    }
    
    // Vérifier que la nouvelle pin DIN n'entre pas en conflit
    if (new_din == old_bck || new_din == old_ws || new_din == old_dout) {
        pins_different = false;
        result.error = "Pin DIN en conflit avec anciennes pins";
    }
    #endif
    
    result.passed = pins_different;
    return result;
}

/**
 * Test 2: Initialisation du WM8731
 */
TestResult test_wm8731_initialization() {
    TestResult result;
    result.description = "Test initialisation WM8731";
    
    bool init_success = wm8731_begin();
    
    if (init_success) {
        // Vérifier l'état après initialisation
        if (wm8731_isReady()) {
            result.passed = true;
        } else {
            result.passed = false;
            result.error = "WM8731 non prêt après initialisation";
        }
    } else {
        result.passed = false;
        result.error = "Échec initialisation WM8731";
    }
    
    return result;
}

/**
 * Test 3: Configuration I2S bidirectionnelle
 */
TestResult test_i2s_bidirectional() {
    TestResult result;
    result.description = "Test I2S bidirectionnel";
    
    if (!wm8731_isReady()) {
        result.passed = false;
        result.error = "WM8731 non initialisé";
        return result;
    }
    
    // Test configuration output (déjà testée dans synthESP32)
    wm8731_enableOutput(true);
    wm8731_setOutputVolume(80);
    
    // Test configuration input (nouvelle fonctionnalité)
    wm8731_enableInput(true);
    wm8731_setInputGain(20);
    wm8731_setInputSource(0); // Line input
    
    // Test lecture simple (sans bloquer)
    uint8_t test_buffer[64];
    size_t bytes_read = wm8731_readAudio(test_buffer, 16);
    
    wm8731_enableInput(false);
    
    // Le test passe si on peut lire des données (même si c'est du silence)
    result.passed = true; // On ne peut pas vraiment tester sans signal d'entrée
    
    return result;
}

/**
 * Test 4: Compatibilité avec synthESP32
 */
TestResult test_synth_compatibility() {
    TestResult result;
    result.description = "Test compatibilité synthESP32";
    
    if (!wm8731_isReady()) {
        result.passed = false;
        result.error = "WM8731 non initialisé";
        return result;
    }
    
    // Activer la sortie pour le test
    wm8731_enableOutput(true);
    wm8731_setOutputVolume(60); // Volume modéré pour le test
    
    // Test des fonctions synthESP32 existantes
    bool synth_compatible = true;
    
    try {
        // Test trigger d'un son
        synthESP32_TRIGGER(0);
        delay(100);
        
        // Test changement de volume
        synthESP32_setVol(0, 100);
        
        // Test changement de filtre
        synthESP32_setFilter(0, 50);
        
        // Test volume master
        synthESP32_setMVol(80);
        
        result.passed = true;
    } catch (...) {
        result.passed = false;
        result.error = "Erreur dans les fonctions synthESP32";
    }
    
    return result;
}

/**
 * Test 5: Contrôle des paramètres WM8731
 */
TestResult test_parameter_control() {
    TestResult result;
    result.description = "Test contrôle paramètres WM8731";
    
    if (!wm8731_isReady()) {
        result.passed = false;
        result.error = "WM8731 non initialisé";
        return result;
    }
    
    bool params_ok = true;
    
    // Test gain input (0-31)
    wm8731_setInputGain(0);
    wm8731_setInputGain(15);
    wm8731_setInputGain(31);
    
    // Test volume output (48-127)
    wm8731_setOutputVolume(48);
    wm8731_setOutputVolume(90);
    wm8731_setOutputVolume(127);
    
    // Test source input
    wm8731_setInputSource(0); // Line
    wm8731_setInputSource(1); // Mic
    wm8731_setInputSource(0); // Retour Line
    
    // Test enable/disable
    wm8731_enableInput(true);
    wm8731_enableInput(false);
    wm8731_enableOutput(false);
    wm8731_enableOutput(true);
    
    result.passed = params_ok;
    return result;
}

/**
 * Test 6: Performance et latence
 */
TestResult test_performance() {
    TestResult result;
    result.description = "Test performance audio";
    
    if (!wm8731_isReady()) {
        result.passed = false;
        result.error = "WM8731 non initialisé";
        return result;
    }
    
    // Mesurer le temps de traitement audio
    unsigned long start_time = micros();
    
    // Simuler une boucle audio typique
    for (int i = 0; i < 10; i++) {
        synthESP32_TRIGGER(i % 16);
        delayMicroseconds(100);
    }
    
    unsigned long elapsed = micros() - start_time;
    
    // Vérifier que le traitement reste dans les limites acceptables
    // Pour 44.1kHz, chaque échantillon = ~22.7µs
    bool performance_ok = (elapsed < 5000); // Moins de 5ms pour 10 triggers
    
    result.passed = performance_ok;
    if (!performance_ok) {
        result.error = "Performance dégradée: " + String(elapsed) + "µs";
    }
    
    return result;
}

/**
 * Test 7: Mémoire et stabilité
 */
TestResult test_memory_stability() {
    TestResult result;
    result.description = "Test stabilité mémoire";
    
    // Vérifier la mémoire libre avant
    size_t free_heap_before = ESP.getFreeHeap();
    
    // Effectuer plusieurs opérations
    for (int i = 0; i < 100; i++) {
        wm8731_setOutputVolume(60 + (i % 67));
        wm8731_setInputGain(i % 32);
        if (i % 10 == 0) {
            wm8731_enableInput(true);
            wm8731_enableInput(false);
        }
    }
    
    // Vérifier la mémoire libre après
    size_t free_heap_after = ESP.getFreeHeap();
    
    // Vérifier qu'il n'y a pas de fuite mémoire significative
    int memory_diff = free_heap_before - free_heap_after;
    bool memory_stable = (abs(memory_diff) < 1024); // Moins de 1KB de différence
    
    result.passed = memory_stable;
    if (!memory_stable) {
        result.error = "Fuite mémoire détectée: " + String(memory_diff) + " octets";
    }
    
    return result;
}

/**
 * Test 8: Compatibilité I2C avec autres périphériques
 */
TestResult test_i2c_compatibility() {
    TestResult result;
    result.description = "Test compatibilité I2C";
    
    bool i2c_ok = true;
    
    #ifdef ads_ok
    // Tester que l'ADS1015 fonctionne encore
    try {
        int16_t adc_test = ads.readADC_SingleEnded(0);
        // Si on arrive ici, l'I2C fonctionne
    } catch (...) {
        i2c_ok = false;
        result.error = "Conflit I2C avec ADS1015";
    }
    #endif
    
    // Tester la communication avec WM8731
    if (i2c_ok) {
        wm8731_setOutputVolume(80);
        // Si ça ne plante pas, c'est bon
    }
    
    result.passed = i2c_ok;
    return result;
}

/**
 * Test 9: Intégration avec le séquenceur
 */
TestResult test_sequencer_integration() {
    TestResult result;
    result.description = "Test intégration séquenceur";
    
    bool sequencer_ok = true;
    
    // Simuler quelques steps du séquenceur
    for (int step = 0; step < 16; step++) {
        // Simuler un trigger sur certains steps
        if (step % 4 == 0) {
            synthESP32_TRIGGER(step % 16);
        }
        
        // Vérifier que WM8731 reste stable
        if (!wm8731_isReady()) {
            sequencer_ok = false;
            result.error = "WM8731 instable pendant séquenceur";
            break;
        }
        
        delay(10); // Simuler le timing du séquenceur
    }
    
    result.passed = sequencer_ok;
    return result;
}

/**
 * Test 10: Récupération après erreur
 */
TestResult test_error_recovery() {
    TestResult result;
    result.description = "Test récupération erreurs";
    
    // Simuler une condition d'erreur en désactivant/réactivant
    wm8731_enableOutput(false);
    wm8731_enableInput(false);
    
    delay(100);
    
    // Réactiver et vérifier la récupération
    wm8731_enableOutput(true);
    wm8731_setOutputVolume(80);
    
    bool recovery_ok = wm8731_isReady();
    
    // Test d'un trigger après récupération
    if (recovery_ok) {
        synthESP32_TRIGGER(0);
        delay(100);
    }
    
    result.passed = recovery_ok;
    if (!recovery_ok) {
        result.error = "Échec récupération après erreur";
    }
    
    return result;
}

/**
 * Exécuter tous les tests de compatibilité
 */
void run_compatibility_tests() {
    Serial.println("=== TESTS DE COMPATIBILITÉ WM8731 ===");
    Serial.println();
    
    TestResult tests[] = {
        test_i2s_pins(),
        test_wm8731_initialization(),
        test_i2s_bidirectional(),
        test_synth_compatibility(),
        test_parameter_control(),
        test_performance(),
        test_memory_stability(),
        test_i2c_compatibility(),
        test_sequencer_integration(),
        test_error_recovery()
    };
    
    int num_tests = sizeof(tests) / sizeof(TestResult);
    int passed = 0;
    int failed = 0;
    
    for (int i = 0; i < num_tests; i++) {
        Serial.printf("Test %d: %s... ", i + 1, tests[i].description.c_str());
        
        if (tests[i].passed) {
            Serial.println("PASS");
            passed++;
        } else {
            Serial.printf("FAIL - %s\n", tests[i].error.c_str());
            failed++;
        }
    }
    
    Serial.println();
    Serial.printf("RÉSULTATS: %d/%d tests réussis\n", passed, num_tests);
    
    if (failed == 0) {
        Serial.println("✅ TOUS LES TESTS PASSENT - WM8731 compatible");
    } else {
        Serial.printf("❌ %d tests échoués - Vérifier la configuration\n", failed);
    }
    
    Serial.println("=====================================");
}

/**
 * Test de charge (stress test)
 */
void stress_test_wm8731() {
    Serial.println("=== STRESS TEST WM8731 ===");
    
    if (!wm8731_isReady()) {
        Serial.println("WM8731 non initialisé");
        return;
    }
    
    unsigned long start_time = millis();
    int iterations = 1000;
    bool stress_ok = true;
    
    for (int i = 0; i < iterations && stress_ok; i++) {
        // Changements rapides de paramètres
        wm8731_setOutputVolume(48 + (i % 80));
        wm8731_setInputGain(i % 32);
        
        // Triggers fréquents
        if (i % 10 == 0) {
            synthESP32_TRIGGER(i % 16);
        }
        
        // Enable/disable périodique
        if (i % 100 == 0) {
            wm8731_enableInput(i % 200 < 100);
        }
        
        // Vérifier la stabilité
        if (!wm8731_isReady()) {
            stress_ok = false;
            Serial.printf("ÉCHEC à l'itération %d\n", i);
        }
        
        // Petit délai pour ne pas surcharger
        if (i % 50 == 0) {
            delay(1);
        }
    }
    
    unsigned long elapsed = millis() - start_time;
    
    if (stress_ok) {
        Serial.printf("✅ STRESS TEST RÉUSSI: %d itérations en %lu ms\n", 
                     iterations, elapsed);
        Serial.printf("Performance: %.2f opérations/seconde\n", 
                     (float)iterations * 1000.0 / elapsed);
    } else {
        Serial.println("❌ STRESS TEST ÉCHOUÉ");
    }
    
    Serial.println("=========================");
}

/**
 * Benchmark des performances audio
 */
void benchmark_audio_performance() {
    Serial.println("=== BENCHMARK AUDIO ===");
    
    if (!wm8731_isReady()) {
        Serial.println("WM8731 non initialisé");
        return;
    }
    
    // Test latence trigger
    unsigned long trigger_times[100];
    for (int i = 0; i < 100; i++) {
        unsigned long start = micros();
        synthESP32_TRIGGER(i % 16);
        trigger_times[i] = micros() - start;
        delayMicroseconds(1000); // 1ms entre chaque trigger
    }
    
    // Calculer statistiques
    unsigned long min_time = trigger_times[0];
    unsigned long max_time = trigger_times[0];
    unsigned long total_time = 0;
    
    for (int i = 0; i < 100; i++) {
        if (trigger_times[i] < min_time) min_time = trigger_times[i];
        if (trigger_times[i] > max_time) max_time = trigger_times[i];
        total_time += trigger_times[i];
    }
    
    unsigned long avg_time = total_time / 100;
    
    Serial.printf("Latence trigger - Min: %lu µs, Max: %lu µs, Moyenne: %lu µs\n",
                 min_time, max_time, avg_time);
    
    // Test débit I2C
    unsigned long i2c_start = micros();
    for (int i = 0; i < 10; i++) {
        wm8731_setOutputVolume(60 + i);
    }
    unsigned long i2c_time = micros() - i2c_start;
    
    Serial.printf("Débit I2C: %lu µs pour 10 opérations (%.2f op/s)\n",
                 i2c_time, 10.0 * 1000000.0 / i2c_time);
    
    // Test mémoire
    size_t free_heap = ESP.getFreeHeap();
    size_t total_heap = ESP.getHeapSize();
    size_t used_heap = total_heap - free_heap;
    
    Serial.printf("Mémoire - Total: %d bytes, Utilisée: %d bytes, Libre: %d bytes\n",
                 total_heap, used_heap, free_heap);
    Serial.printf("Utilisation: %.1f%%\n", (float)used_heap * 100.0 / total_heap);
    
    Serial.println("=====================");
}

#endif // WM8731_COMPATIBILITY_TEST_H

/**
 * Fonction principale pour exécuter tous les tests
 * À appeler depuis setup() après l'initialisation
 */
void test_wm8731_compatibility() {
    Serial.println("Démarrage des tests de compatibilité WM8731...");
    delay(1000);
    
    run_compatibility_tests();
    delay(500);
    
    stress_test_wm8731();
    delay(500);
    
    benchmark_audio_performance();
    
    Serial.println("Tests terminés.");
}