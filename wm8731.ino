/**
 * WM8731 Audio Codec Driver pour ESP32-S3
 * Remplace le PCM5102A pour ajouter l'audio bidirectionnel
 * 
 * Auteur: ZCARLOS 2025
 * Version: 1.0
 * 
 * Fonctionnalités:
 * - Configuration I2S bidirectionnelle (input + output)
 * - Interface I2C pour contrôle du codec WM8731
 * - Contrôle gain input/output et routage audio
 * - Compatible avec l'architecture existante synthESP32
 */

#include <Wire.h>
#include "driver/i2s.h"

// Configuration pins WM8731 MIKROE-506
#define WM8731_I2C_ADDR     0x1A    // Adresse I2C du WM8731

// I2C pins (shared with touch controller)
#define WM8731_SDA_PIN      8       // SDA → IO8 (shared with touch)
#define WM8731_SCL_PIN      4       // SCL → IO4 (shared with touch)

// I2S pins pour audio bidirectionnel MIKROE-506
#define WM8731_BCK_PIN      37      // BCLK → IO37 (tie to both DAC-SCK and ADC-SCK)
#define WM8731_WS_PIN       15      // LRCLK → IO15 (tie to both DACL and ADCL)
#define WM8731_DOUT_PIN     16      // DACDAT → IO16 (ESP→codec, MIKROE MOSI)
#define WM8731_DIN_PIN      36      // ADCDAT → IO36 (codec→ESP, MIKROE MISO)

// Registres WM8731
#define WM8731_REG_LLINEIN       0x00
#define WM8731_REG_RLINEIN       0x01
#define WM8731_REG_LHEADOUT      0x02
#define WM8731_REG_RHEADOUT      0x03
#define WM8731_REG_ANALOGPATH    0x04
#define WM8731_REG_DIGITALPATH   0x05
#define WM8731_REG_POWERDOWN     0x06
#define WM8731_REG_DIGITALIF     0x07
#define WM8731_REG_SAMPLERATE    0x08
#define WM8731_REG_DIGITALACT    0x09
#define WM8731_REG_RESET         0x0F

// Valeurs par défaut
#define WM8731_INPUT_GAIN_DEFAULT    23    // Gain input par défaut (0-31)
#define WM8731_OUTPUT_VOL_DEFAULT    121   // Volume output par défaut (48-127)
#define WM8731_INPUT_LINE            0     // Source Line Input
#define WM8731_INPUT_MIC             1     // Source Microphone

/**
 * Classe de gestion du codec WM8731
 */
class WM8731_Manager {
private:
    bool _initialized;
    uint8_t _inputGain;
    uint8_t _outputVolume;
    uint8_t _inputSource;
    bool _inputEnabled;
    bool _outputEnabled;
    TwoWire* _i2c;
    
    /**
     * Écriture dans un registre du WM8731 via I2C
     */
    void writeRegister(uint8_t reg, uint16_t value) {
        uint16_t data = (reg << 9) | (value & 0x1FF);
        
        _i2c->beginTransmission(WM8731_I2C_ADDR);
        _i2c->write((data >> 8) & 0xFF);
        _i2c->write(data & 0xFF);
        uint8_t error = _i2c->endTransmission();
        
        if (error != 0) {
            Serial.printf("WM8731 I2C Error: %d\n", error);
        }
        
        delay(1); // Petit délai pour stabilité
    }
    
    /**
     * Initialisation des registres du WM8731
     */
    bool initializeCodec() {
        Serial.println("Initialisation WM8731...");
        
        // Reset du codec
        writeRegister(WM8731_REG_RESET, 0x00);
        delay(10);
        
        // Configuration Power Down - Tout activé sauf OUT et DAC initialement
        writeRegister(WM8731_REG_POWERDOWN, 0x60); // OUTPD=1, DACPD=1
        
        // Configuration interface numérique - I2S, 16-bit, Master mode
        writeRegister(WM8731_REG_DIGITALIF, 0x42); // FORMAT=10 (I2S), IWL=00 (16-bit)
        
        // Configuration fréquence d'échantillonnage - 44.1kHz
        writeRegister(WM8731_REG_SAMPLERATE, 0x20); // SR=1000 (44.1kHz), CLKDIV2=0
        
        // Configuration chemin analogique
        writeRegister(WM8731_REG_ANALOGPATH, 0x12); // DACSEL=1, BYPASS=0, INSEL=0, MUTEMIC=1
        
        // Configuration chemin numérique
        writeRegister(WM8731_REG_DIGITALPATH, 0x00); // Pas de mute, pas de filtre
        
        // Configuration input gain initial
        writeRegister(WM8731_REG_LLINEIN, 0x117); // LRINBOTH=1, LINVOL=23
        writeRegister(WM8731_REG_RLINEIN, 0x117);
        
        // Configuration output volume initial
        writeRegister(WM8731_REG_LHEADOUT, 0x179); // LRHPBOTH=1, LHPVOL=121
        writeRegister(WM8731_REG_RHEADOUT, 0x179);
        
        // Activation du codec
        writeRegister(WM8731_REG_DIGITALACT, 0x01); // ACTIVE=1
        
        delay(10);
        
        Serial.println("WM8731 initialisé");
        return true;
    }
    
    /**
     * Configuration I2S bidirectionnelle
     */
    void configureI2S() {
        Serial.println("Configuration I2S bidirectionnelle...");
        
        // Configuration I2S pour RX et TX simultanés
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
            .sample_rate = SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
            .dma_buf_count = DMA_NUM_BUF,
            .dma_buf_len = DMA_BUF_LEN,
            .use_apll = true,
            .tx_desc_auto_clear = true,
            .fixed_mclk = 0
        };
        
        // Configuration des pins I2S
        i2s_pin_config_t i2s_pin_config = {
            .bck_io_num = WM8731_BCK_PIN,
            .ws_io_num = WM8731_WS_PIN,
            .data_out_num = WM8731_DOUT_PIN,
            .data_in_num = WM8731_DIN_PIN
        };
        
        // Installation du driver I2S
        esp_err_t ret = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
        if (ret != ESP_OK) {
            Serial.printf("Erreur installation I2S: %d\n", ret);
            return;
        }
        
        // Configuration des pins
        ret = i2s_set_pin(I2S_NUM_0, &i2s_pin_config);
        if (ret != ESP_OK) {
            Serial.printf("Erreur configuration pins I2S: %d\n", ret);
            return;
        }
        
        Serial.println("I2S configuré en mode bidirectionnel");
    }

public:
    /**
     * Constructeur
     */
    WM8731_Manager() : _initialized(false), 
                       _inputGain(WM8731_INPUT_GAIN_DEFAULT),
                       _outputVolume(WM8731_OUTPUT_VOL_DEFAULT),
                       _inputSource(WM8731_INPUT_LINE),
                       _inputEnabled(false),
                       _outputEnabled(true),
                       _i2c(nullptr) {}
    
    /**
     * Initialisation du driver WM8731
     * @return true si succès, false sinon
     */
    bool begin() {
        if (_initialized) {
            Serial.println("WM8731 déjà initialisé");
            return true;
        }
        
        // Initialisation I2C pour contrôle du codec
        _i2c = &Wire;
        _i2c->begin(WM8731_SDA_PIN, WM8731_SCL_PIN, 400000);
        
        Serial.println("Démarrage WM8731 Manager...");
        
        // Test de communication I2C
        _i2c->beginTransmission(WM8731_I2C_ADDR);
        uint8_t error = _i2c->endTransmission();
        
        if (error != 0) {
            Serial.printf("WM8731 non détecté à l'adresse 0x%02X (erreur: %d)\n", WM8731_I2C_ADDR, error);
            return false;
        }
        
        // Arrêter l'ancien I2S s'il existe
        i2s_driver_uninstall(I2S_NUM_0);
        delay(10);
        
        // Initialiser le codec
        if (!initializeCodec()) {
            Serial.println("Échec initialisation codec WM8731");
            return false;
        }
        
        // Configurer I2S
        configureI2S();
        
        _initialized = true;
        Serial.println("WM8731 Manager prêt");
        
        return true;
    }
    
    /**
     * Configuration du gain d'entrée
     * @param gain Gain 0-31 (0=-34.5dB, 31=+12dB)
     */
    void setInputGain(uint8_t gain) {
        if (!_initialized) return;
        
        _inputGain = constrain(gain, 0, 31);
        
        // LRINBOTH=1 pour mettre à jour les deux canaux simultanément
        uint16_t regValue = 0x100 | _inputGain;
        writeRegister(WM8731_REG_LLINEIN, regValue);
        
        Serial.printf("Gain input: %d\n", _inputGain);
    }
    
    /**
     * Configuration du volume de sortie
     * @param volume Volume 48-127 (48=-73dB, 127=+6dB)
     */
    void setOutputVolume(uint8_t volume) {
        if (!_initialized) return;
        
        _outputVolume = constrain(volume, 48, 127);
        
        // LRHPBOTH=1 pour mettre à jour les deux canaux simultanément
        uint16_t regValue = 0x100 | _outputVolume;
        writeRegister(WM8731_REG_LHEADOUT, regValue);
        
        Serial.printf("Volume output: %d\n", _outputVolume);
    }
    
    /**
     * Activation/désactivation de l'entrée audio
     * @param enable true pour activer, false pour désactiver
     */
    void enableInput(bool enable) {
        if (!_initialized) return;
        
        _inputEnabled = enable;
        
        // Lecture de l'état actuel du registre power
        uint16_t powerReg = 0x60; // État par défaut
        
        if (_inputEnabled) {
            powerReg &= ~0x20; // ADCPD=0 (activer ADC)
        } else {
            powerReg |= 0x20;  // ADCPD=1 (désactiver ADC)
        }
        
        if (_outputEnabled) {
            powerReg &= ~0x40; // DACPD=0 (activer DAC)
            powerReg &= ~0x80; // OUTPD=0 (activer sortie)
        } else {
            powerReg |= 0x40;  // DACPD=1 (désactiver DAC)
            powerReg |= 0x80;  // OUTPD=1 (désactiver sortie)
        }
        
        writeRegister(WM8731_REG_POWERDOWN, powerReg);
        
        Serial.printf("Input %s\n", enable ? "activé" : "désactivé");
    }
    
    /**
     * Activation/désactivation de la sortie audio
     * @param enable true pour activer, false pour désactiver
     */
    void enableOutput(bool enable) {
        if (!_initialized) return;
        
        _outputEnabled = enable;
        
        // Même logique que enableInput mais pour la sortie
        uint16_t powerReg = 0x60;
        
        if (_inputEnabled) {
            powerReg &= ~0x20;
        } else {
            powerReg |= 0x20;
        }
        
        if (_outputEnabled) {
            powerReg &= ~0x40;
            powerReg &= ~0x80;
        } else {
            powerReg |= 0x40;
            powerReg |= 0x80;
        }
        
        writeRegister(WM8731_REG_POWERDOWN, powerReg);
        
        Serial.printf("Output %s\n", enable ? "activé" : "désactivé");
    }
    
    /**
     * Sélection de la source d'entrée
     * @param source 0=Line Input, 1=Microphone
     */
    void setInputSource(uint8_t source) {
        if (!_initialized) return;
        
        _inputSource = (source == WM8731_INPUT_MIC) ? WM8731_INPUT_MIC : WM8731_INPUT_LINE;
        
        // Configuration du chemin analogique
        uint16_t analogPath = 0x12; // Valeur de base
        
        if (_inputSource == WM8731_INPUT_MIC) {
            analogPath |= 0x04;  // INSEL=1 (sélection microphone)
            analogPath &= ~0x02; // MUTEMIC=0 (activer micro)
        } else {
            analogPath &= ~0x04; // INSEL=0 (sélection line)
            analogPath |= 0x02;  // MUTEMIC=1 (mute micro)
        }
        
        writeRegister(WM8731_REG_ANALOGPATH, analogPath);
        
        Serial.printf("Source input: %s\n", (_inputSource == WM8731_INPUT_MIC) ? "Microphone" : "Line");
    }
    
    /**
     * Vérification de l'état du driver
     * @return true si prêt, false sinon
     */
    bool isReady() {
        return _initialized;
    }
    
    /**
     * Lecture d'un échantillon audio depuis l'entrée
     * @param buffer Buffer pour stocker les données
     * @param samples Nombre d'échantillons à lire
     * @return Nombre d'octets lus
     */
    size_t readAudio(uint8_t* buffer, size_t samples) {
        if (!_initialized || !_inputEnabled) return 0;
        
        size_t bytes_read = 0;
        esp_err_t ret = i2s_read(I2S_NUM_0, buffer, samples * 4, &bytes_read, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            Serial.printf("Erreur lecture I2S: %d\n", ret);
            return 0;
        }
        
        return bytes_read;
    }
    
    /**
     * Obtention des paramètres actuels
     */
    uint8_t getInputGain() { return _inputGain; }
    uint8_t getOutputVolume() { return _outputVolume; }
    uint8_t getInputSource() { return _inputSource; }
    bool isInputEnabled() { return _inputEnabled; }
    bool isOutputEnabled() { return _outputEnabled; }
    
    /**
     * Mode de test - génération d'un tone de test
     */
    void testTone() {
        if (!_initialized) return;
        
        Serial.println("Test tone WM8731...");
        
        // Activer sortie si pas déjà fait
        if (!_outputEnabled) {
            enableOutput(true);
        }
        
        // Le tone sera généré par l'architecture existante
        Serial.println("Utilisez synthESP32_TRIGGER() pour tester la sortie audio");
    }
};

// Instance globale du manager WM8731
WM8731_Manager wm8731;

/**
 * Fonction d'initialisation compatible avec l'architecture existante
 * Remplace la configuration I2S dans synthESP32_begin()
 */
bool wm8731_begin() {
    return wm8731.begin();
}

/**
 * Configuration du gain d'entrée (compatible API simple)
 */
void wm8731_setInputGain(uint8_t gain) {
    wm8731.setInputGain(gain);
}

/**
 * Configuration du volume de sortie (compatible API simple)
 */
void wm8731_setOutputVolume(uint8_t volume) {
    wm8731.setOutputVolume(volume);
}

/**
 * Activation/désactivation entrée
 */
void wm8731_enableInput(bool enable) {
    wm8731.enableInput(enable);
}

/**
 * Activation/désactivation sortie
 */
void wm8731_enableOutput(bool enable) {
    wm8731.enableOutput(enable);
}

/**
 * Sélection source d'entrée
 */
void wm8731_setInputSource(uint8_t source) {
    wm8731.setInputSource(source);
}

/**
 * Vérification état
 */
bool wm8731_isReady() {
    return wm8731.isReady();
}

/**
 * Lecture audio depuis l'entrée
 */
size_t wm8731_readAudio(uint8_t* buffer, size_t samples) {
    return wm8731.readAudio(buffer, samples);
}

/**
 * Test de fonctionnement
 */
void wm8731_testTone() {
    wm8731.testTone();
}