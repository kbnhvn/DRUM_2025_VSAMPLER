# 🚀 DRUM_2025_VSAMPLER - GUIDE DE DÉMARRAGE RAPIDE

## ⚡ Mise en Route Express (15 minutes)

### 🎯 Objectif
Faire fonctionner votre DRUM_2025_VSAMPLER Extended et jouer vos premiers samples en moins de 15 minutes.

---

## 📦 Ce Dont Vous Avez Besoin

### Hardware Obligatoire
- **ESP32-S3 JC4827W543 C** (modèle capacitif)
- **WM8731 Audio Codec** (module I2S/I2C)
- **Carte SD** (FAT32, Class 10, 4-32GB)
- **Alimentation** 5V/2A (USB-C ou externe)
- **Sortie audio** (casque, ampli, ou enceintes)

### Software Requis
- **Arduino IDE 2.3.5+**
- **ESP32 Board Package 3.2.0+**
- **Arduino_GFX_Library**

---

## 🔌 Connexions Rapides

### WM8731 → ESP32-S3
```
VCC  → 3.3V    |    SCL  → GPIO 22
GND  → GND     |    SDA  → GPIO 21
BCK  → GPIO 14 |    DIN  → GPIO 12
WS   → GPIO 15 |    DOUT → GPIO 13
```

### SD Card → ESP32-S3
```
VCC  → 3.3V    |    MOSI → GPIO 23
GND  → GND     |    MISO → GPIO 19
CS   → GPIO 5  |    SCK  → GPIO 18
```

---

## 💾 Préparation SD Card

### 1. Formatage
- **Format** : FAT32 obligatoire
- **Taille** : 16GB recommandée

### 2. Structure Répertoires
```
/SAMPLES/
├── KICKS/
├── SNARES/
├── HIHATS/
└── USER/
```

### 3. Samples Test (Optionnel)
Copier quelques fichiers WAV 44.1kHz/16-bit dans les dossiers

---

## 🛠️ Installation Software

### 1. Arduino IDE
```
1. Télécharger Arduino IDE 2.3.5+
2. Installer ESP32 Board Package :
   - File → Preferences → Additional Board Manager URLs
   - Ajouter : https://espressif.github.io/arduino-esp32/package_esp32_index.json
   - Tools → Board Manager → "ESP32" → Install

3. Installer Arduino_GFX_Library :
   - Tools → Manage Libraries → "GFX Library for Arduino"
```

### 2. Configuration Board
```
Tools → Board → ESP32 Arduino → ESP32S3 Dev Module
Tools → Partition Scheme → Huge APP (3MB)
Tools → Upload Speed → 921600
```

---

## 📁 Installation Projet

### 1. Téléchargement
```bash
# Git (si disponible)
git clone https://github.com/[repo]/DRUM_2025_VSAMPLER.git

# Ou télécharger ZIP et extraire
```

### 2. Ouverture
```
1. Arduino IDE → File → Open
2. Sélectionner : DRUM_2025_VSAMPLER.ino
3. Vérifier : Tous fichiers .ino visibles en onglets
```

---

## 🚀 Premier Démarrage

### 1. Upload Code
```
1. Connecter ESP32 via USB
2. Tools → Port → [Sélectionner port correct]
3. Sketch → Upload (maintenir BOOT si nécessaire)
4. Attendre "Hard resetting via RTS pin..."
```

### 2. Vérification Démarrage
```
Serial Monitor (115200 bauds) doit afficher :
✅ WM8731 Manager prêt
✅ SD Card détectée : Type SDHC
✅ Menu System initialisé - 6 applications
🎵 DRUM_2025_VSAMPLER prêt !
```

---

## 🎵 Premier Test Audio

### 1. Test Samples Intégrés
```
1. Écran tactile → Menu affiché automatiquement
2. Touch "DRUM MACHINE" (première app)
3. Touch pad 1 → Son "SYNTH1" doit jouer
4. Tester autres pads → Différents samples
```

### 2. Test Volume
```
Si pas de son :
1. Vérifier connexions audio WM8731
2. Augmenter volume sortie
3. Vérifier alimentation stable
```

---

## 📱 Navigation Interface

### Menu Principal
```
┌─────────────────────────────────────┐
│        DRUM 2025 VSAMPLER          │
├─────────────────────────────────────┤
│  [D] DRUM MACHINE                  │ ← Touch pour drum pads
│  [S] SAMPLING STUDIO               │ ← Touch pour enregistrer  
│  [F] FILE BROWSER                  │ ← Touch pour samples SD
├─────────────────────────────────────┤
│ [M] MIDI  [W] WIFI  [Y] SYSTEM     │ ← Configurations
└─────────────────────────────────────┘
```

### Applications Principales
- **DRUM MACHINE** : 16 pads, polyphonie, paramètres voix
- **FILE BROWSER** : Navigation samples SD, preview, chargement
- **SAMPLING STUDIO** : Enregistrement temps réel, sauvegarde
- **WIFI MANAGER** : Upload/download samples sans fil

---

## 🔧 Résolution Problèmes Express

### ❌ Écran noir
```
Solutions :
✅ Vérifier alimentation 5V/2A
✅ Vérifier connexions display
✅ Redémarrer système
```

### ❌ Pas de son
```
Solutions :
✅ Vérifier connexions WM8731
✅ Vérifier sortie audio connectée
✅ Augmenter volume système
✅ Vérifier messages Serial "WM8731 prêt"
```

### ❌ SD Card non détectée
```
Solutions :
✅ Format FAT32 obligatoire
✅ Vérifier connexions SPI
✅ Tester autre carte SD
✅ Vérifier message Serial "SD détectée"
```

### ❌ Touch non réactif
```
Solutions :
✅ Vérifier modèle ESP32 capacitif (pas résistif)
✅ Calibration automatique au démarrage
✅ Nettoyer écran
✅ Redémarrer pour re-calibration
```

---

## ✅ Check-list Validation

### Hardware ✅
- [ ] ESP32-S3 JC4827W543 C connecté
- [ ] WM8731 câblé et alimenté
- [ ] SD Card insérée et formatée FAT32
- [ ] Sortie audio connectée
- [ ] Alimentation 5V/2A stable

### Software ✅
- [ ] Arduino IDE 2.3.5+ installé
- [ ] ESP32 Board Package 3.2.0+ installé
- [ ] Arduino_GFX_Library installée
- [ ] Code uploadé avec succès
- [ ] Messages démarrage OK dans Serial

### Fonctionnel ✅
- [ ] Menu principal affiché
- [ ] Touch zones réactives
- [ ] Pads drum machine jouent
- [ ] SD Card détectée (si samples)
- [ ] Audio sorti claire sans distorsion

---

## 🎉 Félicitations !

Votre DRUM_2025_VSAMPLER Extended est maintenant **opérationnel** !

### 🎯 Prochaines Étapes

1. **Explorez les Applications**
   - Testez toutes les 6 applications disponibles
   - Naviguez avec le bouton RETOUR (zone 23)

2. **Ajoutez vos Samples**
   - Copiez fichiers WAV dans /SAMPLES/ sur SD
   - Utilisez File Browser pour charger
   - Testez Sampling Studio pour enregistrer

3. **Configurez selon Usage**
   - MIDI Settings pour équipements externes
   - WiFi Manager pour transferts sans fil
   - System Settings pour optimisation

4. **Approfondissez**
   - Lisez la documentation complète
   - Rejoignez la communauté
   - Partagez vos créations

### 📚 Documentation Complète

Pour aller plus loin : [`DRUM_2025_VSAMPLER_COMPLETE_DOCUMENTATION.md`](DRUM_2025_VSAMPLER_COMPLETE_DOCUMENTATION.md)

---

**🎵 Bonne création musicale ! 🎵**

*DRUM_2025_VSAMPLER Extended - Quick Start Guide v1.0*