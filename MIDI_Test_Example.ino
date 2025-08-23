////////////////////////////////////////////////////////////////////////////////////////////
//                        MIDI UART Hardware Test Example
////////////////////////////////////////////////////////////////////////////////////////////
// Programme de test simple pour valider le module MIDI du DRUM_2025_VSAMPLER
// Ce test peut être utilisé pour vérifier les fonctionnalités MIDI de base

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== MIDI UART Hardware Test ===");
  Serial.println("Initializing MIDI...");
  
  // Initialiser le système MIDI
  midiSetup();
  
  // Configuration de test
  midiSetEnabled(true);
  midiSetChannel(1);
  midiSetClockMode(false);  // Mode master pour test
  midiSetReceiving(true);
  midiSetTransmitting(true);
  
  Serial.println("MIDI initialized successfully!");
  Serial.println(midiGetStatus());
  
  // Test 1: Envoyer quelques notes MIDI
  Serial.println("\nTest 1: Sending MIDI Notes...");
  testMidiNotes();
  
  // Test 2: Envoyer des CC
  Serial.println("\nTest 2: Sending MIDI CC...");
  testMidiCC();
  
  // Test 3: Test du clock MIDI
  Serial.println("\nTest 3: MIDI Clock test...");
  testMidiClock();
  
  Serial.println("\n=== Tests terminés ===");
  Serial.println("Connectez un équipement MIDI pour tester la réception");
}

void loop() {
  // Traitement MIDI continu
  midiProcess();
  
  // Test périodique - envoyer une note toutes les 2 secondes
  static unsigned long lastTest = 0;
  if (millis() - lastTest > 2000) {
    lastTest = millis();
    
    // Note aléatoire pour test
    uint8_t note = random(36, 84);  // Do2 à Do5
    midiSendNoteOn(1, note, 100);
    Serial.printf("Test note envoyée: %d\n", note);
    
    delay(100);
    midiSendNoteOff(1, note);
  }
  
  delay(10);
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              FONCTIONS DE TEST
////////////////////////////////////////////////////////////////////////////////////////////

void testMidiNotes() {
  // Envoyer une gamme de Do majeur
  uint8_t scale[] = {60, 62, 64, 65, 67, 69, 71, 72}; // Do majeur
  
  for (int i = 0; i < 8; i++) {
    Serial.printf("Sending Note On: %d\n", scale[i]);
    midiSendNoteOn(1, scale[i], 100);
    delay(200);
    
    midiSendNoteOff(1, scale[i]);
    delay(50);
  }
}

void testMidiCC() {
  // Test des principaux CC
  Serial.println("Testing Volume CC...");
  for (int vol = 0; vol <= 127; vol += 32) {
    midiSendCC(1, 7, vol);  // Volume
    Serial.printf("Volume CC sent: %d\n", vol);
    delay(100);
  }
  
  Serial.println("Testing Filter CC...");
  for (int filt = 0; filt <= 127; filt += 32) {
    midiSendCC(1, 74, filt);  // Filter
    Serial.printf("Filter CC sent: %d\n", filt);
    delay(100);
  }
  
  Serial.println("Testing Pan CC...");
  for (int pan = 0; pan <= 127; pan += 64) {
    midiSendCC(1, 10, pan);  // Pan
    Serial.printf("Pan CC sent: %d\n", pan);
    delay(100);
  }
}

void testMidiClock() {
  Serial.println("Sending MIDI Start...");
  midiSendStart();
  delay(100);
  
  Serial.println("Sending 24 MIDI Clocks (1 beat)...");
  for (int i = 0; i < 24; i++) {
    midiSendClock();
    delay(20);  // ~120 BPM
  }
  
  Serial.println("Sending MIDI Stop...");
  midiSendStop();
  delay(100);
}

////////////////////////////////////////////////////////////////////////////////////////////
//                         FONCTIONS DE DEMONSTRATION
////////////////////////////////////////////////////////////////////////////////////////////

void demonstrateFullMidiIntegration() {
  Serial.println("\n=== Démonstration intégration complète ===");
  
  // Simulation d'un pattern de batterie
  uint8_t drumPattern[16] = {1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0};
  uint8_t kickNote = 36;    // Kick
  uint8_t snareNote = 38;   // Snare
  uint8_t hihatNote = 42;   // Hi-hat
  
  midiSendStart();
  
  for (int step = 0; step < 16; step++) {
    Serial.printf("Step %d: ", step);
    
    // Kick sur temps forts
    if (step % 4 == 0) {
      midiSendNoteOn(1, kickNote, 120);
      Serial.print("KICK ");
      delay(10);
      midiSendNoteOff(1, kickNote);
    }
    
    // Snare sur temps 2 et 4
    if (step == 4 || step == 12) {
      midiSendNoteOn(2, snareNote, 100);
      Serial.print("SNARE ");
      delay(10);
      midiSendNoteOff(2, snareNote);
    }
    
    // Hi-hat selon pattern
    if (drumPattern[step]) {
      midiSendNoteOn(3, hihatNote, 80);
      Serial.print("HIHAT ");
      delay(10);
      midiSendNoteOff(3, hihatNote);
    }
    
    Serial.println();
    
    // Envoyer 6 clocks pour chaque 16ème note
    for (int clock = 0; clock < 6; clock++) {
      midiSendClock();
      delay(5);
    }
    
    delay(100);  // Pause entre steps
  }
  
  midiSendStop();
  Serial.println("Pattern terminé!");
}

void testMidiReception() {
  Serial.println("\n=== Test réception MIDI ===");
  Serial.println("Envoyez des notes MIDI sur le canal 1...");
  Serial.println("Les messages reçus seront affichés automatiquement");
  
  // Le traitement de réception se fait automatiquement dans midiProcess()
  // Les messages sont affichés via Serial.printf dans les handlers
  
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {  // Test pendant 10 secondes
    midiProcess();
    delay(10);
  }
  
  Serial.println("Test de réception terminé");
}

void configureMidiChannels() {
  Serial.println("\n=== Configuration des canaux MIDI ===");
  
  // Exemple de configuration multi-canal
  for (int ch = 1; ch <= 16; ch++) {
    // Envoyer Program Change pour chaque canal
    midiSendProgramChange(ch, ch - 1);  // Programme 0-15
    Serial.printf("Canal %d configuré avec programme %d\n", ch, ch - 1);
    delay(50);
  }
  
  // Test notes sur différents canaux
  for (int ch = 1; ch <= 4; ch++) {
    uint8_t note = 60 + (ch - 1) * 3;  // Notes différentes par canal
    midiSendNoteOn(ch, note, 100);
    Serial.printf("Note %d envoyée sur canal %d\n", note, ch);
    delay(200);
    midiSendNoteOff(ch, note);
    delay(100);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
//                              UTILITIES DE TEST
////////////////////////////////////////////////////////////////////////////////////////////

void printMidiConfiguration() {
  Serial.println("\n=== Configuration MIDI actuelle ===");
  Serial.println(midiGetStatus());
  Serial.printf("RX Pin: %d\n", MIDI_RX_PIN);
  Serial.printf("TX Pin: %d\n", MIDI_TX_PIN);
  Serial.printf("Baud Rate: %d\n", MIDI_BAUD_RATE);
  Serial.printf("Buffer Size: %d bytes\n", MIDI_BUFFER_SIZE);
  Serial.println("=====================================");
}

void runInteractiveTest() {
  Serial.println("\n=== Mode test interactif ===");
  Serial.println("Commandes disponibles:");
  Serial.println("'n' + note (0-127) : Envoyer note");
  Serial.println("'c' + cc + val : Envoyer CC");
  Serial.println("'s' : Envoyer Start");
  Serial.println("'t' : Envoyer Stop");
  Serial.println("'q' : Quitter");
  
  while (true) {
    if (Serial.available()) {
      char cmd = Serial.read();
      
      switch (cmd) {
        case 'n': {
          int note = Serial.parseInt();
          if (note >= 0 && note <= 127) {
            midiSendNoteOn(1, note, 100);
            Serial.printf("Note %d envoyée\n", note);
            delay(100);
            midiSendNoteOff(1, note);
          }
          break;
        }
        
        case 'c': {
          int cc = Serial.parseInt();
          int val = Serial.parseInt();
          if (cc >= 0 && cc <= 127 && val >= 0 && val <= 127) {
            midiSendCC(1, cc, val);
            Serial.printf("CC %d = %d envoyé\n", cc, val);
          }
          break;
        }
        
        case 's':
          midiSendStart();
          Serial.println("MIDI Start envoyé");
          break;
          
        case 't':
          midiSendStop();
          Serial.println("MIDI Stop envoyé");
          break;
          
        case 'q':
          return;
      }
    }
    
    midiProcess();
    delay(10);
  }
}