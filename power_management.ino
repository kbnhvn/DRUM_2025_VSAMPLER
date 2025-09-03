void initPowerButton() {
  Serial.println("[POWER] === POWER MANAGEMENT INIT ===");
  
  // Configuration du pin bouton
  pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
  
  // Configuration wake-up sources
  esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_BUTTON_PIN, 0); // Bouton sur LOW
  esp_sleep_enable_timer_wakeup(BATTERY_CHECK_INTERVAL * 1000000ULL); // Timer backup
  
  // Vérifier la cause du réveil
  handleWakeUp();
  
  // Test initial du bouton
  bool buttonTest = digitalRead(POWER_BUTTON_PIN);
  Serial.printf("[POWER] Button pin %d state: %s\n", 
                POWER_BUTTON_PIN, buttonTest ? "HIGH (OK)" : "LOW (pressed)");
  
  if (!buttonTest) {
    Serial.println("[POWER] WARNING: Button appears stuck or wiring issue");
  }
  
  Serial.println("[POWER] Power management ready");
  Serial.println("[POWER] - Short press: Wake/brightness");
  Serial.println("[POWER] - Long press (3s): Deep sleep");
  Serial.println("[POWER] ================================");
}

void handlePowerButton() {
  // Lecture avec debouncing
  bool reading = !digitalRead(POWER_BUTTON_PIN); // Inversion pour pull-up
  
  // Gestion debounce
  if (reading != lastPowerButtonState) {
    lastPowerDebounce = millis();
  }
  
  if ((millis() - lastPowerDebounce) > DEBOUNCE_TIME) {
    if (reading != powerButtonState) {
      powerButtonState = reading;
      
      if (powerButtonState) {
        // Bouton pressé
        powerPressTime = millis();
        longPressHandled = false;
        Serial.println("[POWER] Button pressed");
        
      } else {
        // Bouton relâché
        unsigned long duration = millis() - powerPressTime;
        Serial.printf("[POWER] Button released (%lums)\n", duration);
        
        if (duration < LONG_PRESS_TIME && !longPressHandled) {
          handleShortPress();
        }
        longPressHandled = false;
      }
    }
    
    // Vérification appui long en cours
    if (powerButtonState && !longPressHandled) {
      unsigned long currentDuration = millis() - powerPressTime;
      if (currentDuration >= LONG_PRESS_TIME) {
        handleLongPress();
        longPressHandled = true;
      }
    }
  }
  
  lastPowerButtonState = reading;
}

void handleShortPress() {
  Serial.println("[POWER] Short press action");
  
  // Action 1: Réveil du backlight
  if (getBacklightPercent() < 50) {
    setBacklightPercent(80);
    Serial.println("[POWER] Backlight activated");
    return;
  }
  
  // Action 2: Retour à la vue principale
  if (currentView != VIEW_MAIN) {
    currentView = VIEW_MAIN;
    forceCompleteRedraw();
    Serial.println("[POWER] Returned to main view");
    return;
  }
  
  // Action 3: Feedback si déjà réveillé
  int currentBrightness = getBacklightPercent();
  setBacklightPercent(100);
  delay(100);
  setBacklightPercent(currentBrightness);
  Serial.println("[POWER] Wake feedback");
}

void handleLongPress() {
  Serial.println("[POWER] Long press detected - sleep sequence");
  
  // Vérifier si on peut faire du deep sleep
  if (!batteryPowered) {
    showUSBModeMessage();
    return;
  }
  
  // Démarrer séquence de sleep
  showSleepConfirmation();
}

void showSleepConfirmation() {
  if (!gfx) {
    shutdownRequested = true;
    return;
  }
  
  // Écran de confirmation
  gfx->fillScreen(RGB565(20, 20, 25));
  
  // Titre
  gfx->setTextColor(RGB565(100, 150, 255), RGB565(20, 20, 25));
  gfx->setCursor(180, 80);
  gfx->print("SLEEP MODE");
  
  // Instructions
  gfx->setTextColor(WHITE, RGB565(20, 20, 25));
  gfx->setCursor(120, 110);
  gfx->print("Hold button to confirm");
  
  gfx->setCursor(130, 130);
  gfx->print("Release to cancel");
  
  // Progress bar background
  gfx->drawRect(140, 160, 200, 20, WHITE);
  gfx->fillRect(142, 162, 196, 16, RGB565(40, 40, 45));
  
  // Attendre confirmation (2 secondes)
  unsigned long confirmStart = millis();
  bool confirmed = true;
  
  while (millis() - confirmStart < 2000) {
    // Vérifier si bouton toujours pressé
    if (digitalRead(POWER_BUTTON_PIN)) {
      // Bouton relâché - annuler
      confirmed = false;
      break;
    }
    
    // Mettre à jour la barre de progression
    unsigned long elapsed = millis() - confirmStart;
    int progress = (elapsed * 196) / 2000;
    
    gfx->fillRect(142, 162, progress, 16, RGB565(100, 150, 255));
    
    // Countdown
    int remaining = 2 - (elapsed / 1000);
    gfx->fillRect(160, 190, 160, 15, RGB565(20, 20, 25));
    gfx->setCursor(210, 200);
    gfx->printf("Sleep in %d", remaining + 1);
    
    delay(50);
  }
  
  if (confirmed) {
    Serial.println("[POWER] Sleep confirmed");
    shutdownRequested = true;
  } else {
    Serial.println("[POWER] Sleep cancelled");
    
    // Message d'annulation
    gfx->fillRect(140, 140, 200, 40, RGB565(20, 20, 25));
    gfx->setTextColor(RGB565(100, 255, 100), RGB565(20, 20, 25));
    gfx->setCursor(190, 165);
    gfx->print("CANCELLED");
    delay(1000);
    
    // Retour à l'affichage normal
    forceCompleteRedraw();
  }
}

void showUSBModeMessage() {
  if (!gfx) return;
  
  // Message mode USB
  gfx->fillRect(100, 100, 280, 80, RGB565(40, 40, 20));
  gfx->drawRect(100, 100, 280, 80, RGB565(255, 200, 0));
  
  gfx->setTextColor(RGB565(255, 200, 0), RGB565(40, 40, 20));
  gfx->setCursor(170, 125);
  gfx->print("USB MODE");
  
  gfx->setTextColor(WHITE, RGB565(40, 40, 20));
  gfx->setCursor(130, 145);
  gfx->print("Sleep disabled");
  
  gfx->setCursor(140, 160);
  gfx->print("Disconnect USB");
  
  delay(2500);
  forceCompleteRedraw();
}

void performDeepSleep() {
  Serial.println("[POWER] ===== ENTERING DEEP SLEEP =====");
  
  if (gfx) {
    // Écran final
    gfx->fillScreen(BLACK);
    gfx->setTextColor(RGB565(100, 150, 255), BLACK);
    gfx->setCursor(160, 110);
    gfx->print("SLEEPING...");
    
    gfx->setTextColor(RGB565(150, 150, 150), BLACK);
    gfx->setCursor(130, 140);
    gfx->print("Press button to wake");
    
    gfx->setCursor(170, 160);
    gfx->print("Saving...");
  }
  
  // Arrêter l'audio proprement
  for (int i = 0; i < 16; i++) {
    latch[i] = 0;
  }
  
  // Sauvegarder l'état actuel
  save_pattern(0);   // Pattern de récupération
  save_sound(0);     // Sound set de récupération
  Serial.println("[POWER] State saved");
  
  // Arrêter les services réseau
  if (isFileServerOn()) {
    stopFileServer();
    Serial.println("[POWER] File server stopped");
  }
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("[POWER] WiFi disabled");
  
  // Réduire la luminosité progressivement
  if (gfx) {
    for (int brightness = getBacklightPercent(); brightness > 0; brightness -= 10) {
      setBacklightPercent(brightness);
      delay(50);
    }
    gfx->fillScreen(BLACK);
  }
  
  // Message final
  Serial.println("[POWER] Going to deep sleep...");
  Serial.println("[POWER] Wake up: press button or wait 30min");
  Serial.flush(); // S'assurer que tous les messages sont envoyés
  
  delay(500); // Laisser le temps aux dernières opérations
  
  // DEEP SLEEP
  esp_deep_sleep_start();
  
  // Cette ligne ne sera jamais atteinte
}

void handleWakeUp() {
  esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();
  
  switch (wakeupCause) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("[POWER] === WAKE UP FROM BUTTON ===");
      performWakeUpAnimation();
      break;
      
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("[POWER] === WAKE UP FROM TIMER ===");
      // Vérification batterie silencieuse
      checkBatteryAndReturn();
      break;
      
    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
      Serial.println("[POWER] === NORMAL BOOT ===");
      // Boot normal, pas de wake-up
      break;
  }
}

void performWakeUpAnimation() {
  if (!gfx) return;
  
  // Animation de réveil
  gfx->fillScreen(BLACK);
  gfx->setTextColor(RGB565(100, 255, 100), BLACK);
  gfx->setCursor(190, 130);
  gfx->print("WAKE UP");
  
  // Réveil progressif du backlight
  for (int brightness = 0; brightness <= 80; brightness += 10) {
    setBacklightPercent(brightness);
    delay(80);
  }
  
  delay(800);
  Serial.println("[POWER] Wake up animation complete");
}

void checkBatteryAndReturn() {
  // Vérification rapide de la batterie sans allumer l'écran
  // (Implémentez selon votre hardware de mesure batterie)
  
  /*
  int batteryLevel = readBatteryLevel(); // Votre fonction
  
  if (batteryLevel < 5) {
    Serial.println("[POWER] Critical battery - staying asleep");
    esp_deep_sleep_start(); // Retour immédiat en sleep
  }
  
  if (batteryLevel < 15) {
    Serial.printf("[POWER] Low battery: %d%%\n", batteryLevel);
  }
  */
  
  Serial.println("[POWER] Battery check complete - returning to sleep");
  delay(100);
  
  // Retour en deep sleep sans réveil utilisateur
  esp_deep_sleep_start();
}

// Fonctions utilitaires
bool isPowerButtonPressed() {
  return !digitalRead(POWER_BUTTON_PIN);
}

void setBatteryMode(bool isBattery) {
  batteryPowered = isBattery;
  Serial.printf("[POWER] Battery mode: %s\n", isBattery ? "ENABLED" : "DISABLED");
}