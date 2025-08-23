# Guide de Démarrage Rapide - Système SD Card DRUM_2025_VSAMPLER

## 🚀 Installation et Configuration

### 1. Préparation Hardware

```
ESP32-S3 JC4827W543
    ├── SD Card (Class 10 recommandée, FAT32)
    └── Connexions SPI :
        ├── CS   → GPIO 5
        ├── MOSI → GPIO 23  
        ├── MISO → GPIO 19
        └── SCK  → GPIO 18
```

### 2. Préparation de la Carte SD

#### Formatage
- **Format** : FAT32
- **Taille** : 4GB à 32GB recommandée
- **Classe** : Class 10 pour de meilleures performances

#### Structure des Répertoires
```
📁 SAMPLES/
├── 📁 KICKS/
│   ├── 🎵 acoustic_kick.wav
│   ├── 🎵 electronic_kick.wav
│   └── 🎵 vintage_kick.wav
├── 📁 SNARES/
│   ├── 🎵 classic_snare.wav
│   ├── 🎵 rim_shot.wav
│   └── 🎵 clap.wav
├── 📁 HIHATS/
│   ├── 🎵 closed_hat.wav
│   ├── 🎵 open_hat.wav
│   └── 🎵 pedal_hat.wav
├── 📁 PERCUSSION/
│   ├── 🎵 shaker.wav
│   ├── 🎵 tambourine.wav
│   └── 🎵 cowbell.wav
├── 📁 SYNTHS/
│   ├── 🎵 bass_synth.wav
│   ├── 🎵 lead_synth.wav
│   └── 🎵 pad_synth.wav
└── 📁 USER/
    ├── 🎵 my_sample1.wav
    └── 🎵 my_sample2.wav
```

### 3. Conversion des Samples

#### Spécifications Requises
- **Format** : WAV PCM 16-bit
- **Sample Rate** : 44100 Hz
- **Canaux** : Mono (recommandé) ou Stéréo
- **Durée max** : ~4.5 secondes (200KB max)

#### Conversion avec Audacity
```
1. Ouvrir le fichier audio
2. Pistes → Mélanger et Rendu (pour stéréo → mono)
3. Projet → Fréquence → 44100 Hz
4. Fichier → Exporter → WAV
5. Options : PCM signé 16-bit
```

#### Conversion avec FFmpeg
```bash
# Mono 44.1kHz 16-bit
ffmpeg -i input.mp3 -ar 44100 -ac 1 -sample_fmt s16 output.wav

# Avec limitation de durée (4 secondes)
ffmpeg -i input.mp3 -t 4 -ar 44100 -ac 1 -sample_fmt s16 output.wav
```

## ⚡ Premiers Tests

### 1. Vérification Système

Après insertion de la carte SD et démarrage :

```
=== INITIALISATION SYSTÈME SD ===
Type de carte SD : SDHC
Taille carte SD : 16000MB
Scan des samples dans : /SAMPLES/
Sample trouvé : acoustic_kick (44100Hz, 1ch, 250ms)
Sample trouvé : classic_snare (44100Hz, 1ch, 180ms)
Total samples trouvés : 6
Système SD initialisé avec succès
==============================
```

### 2. Test de Chargement

```cpp
// Dans le Serial Monitor, vous devriez voir :
"Sample chargé : /SAMPLES/KICKS/acoustic_kick.wav -> slot 0 (11025 échantillons, 22050 bytes)"
"Sample SD 'acoustic_kick' assigné à la voix 0"
```

### 3. Test Audio

- Appuyer sur le pad 1 → Le sample SD doit jouer
- La LED du pad doit s'allumer lors du trigger

## 🎛️ Utilisation de l'Interface

### Navigation des Samples

1. **Activer le navigateur** : Appui long sur bouton "SD BROWSE"
2. **Parcourir** : Utiliser l'encodeur rotatif
3. **Sélectionner** : Appuyer sur "SD LOAD"
4. **Assigner à une voix** : Sélectionner le pad destination

### Modes de Fonctionnement

| Mode | Description | Utilisation |
|------|------------|-------------|
| **0** | Samples intégrés uniquement | Performance stable |
| **1** | Samples SD uniquement | Maximum de flexibilité |
| **2** | Hybride | Meilleur des deux mondes |

### Commandes Rapides

```
SD BROWSE : Navigateur ON/OFF
SD LOAD   : Charger sample sélectionné  
Encodeur  : Parcourir samples disponibles
Shift+Pad : Assigner sample à la voix
```

## 📊 Monitoring et Debug

### Afficher les Statistiques

```cpp
// Via Serial Monitor
sdPrintCacheStats();
```

Output typique :
```
=== STATISTIQUES CACHE SD ===
Slots utilisés : 4/16 (25%)
Mémoire utilisée : 88200 bytes
Samples disponibles : 12
Samples en cache :
  Slot 0 : acoustic_kick.wav (11025 échantillons)
  Slot 1 : classic_snare.wav (7938 échantillons)
============================
```

### Codes d'État LED

| État | Signification |
|------|--------------|
| 🟢 Vert continu | SD prête, samples chargés |
| 🟡 Jaune clignotant | Chargement en cours |
| 🔴 Rouge | Erreur SD ou sample |
| 🔵 Bleu | Mode navigateur actif |

## 🔧 Résolution Rapide des Problèmes

### ❌ SD non détectée

**Symptômes** : Message "Erreur initialisation SD"

**Solutions** :
1. ✅ Vérifier insertion carte SD
2. ✅ Tester avec carte différente  
3. ✅ Vérifier connexions SPI
4. ✅ Reformater en FAT32

### ❌ Samples non trouvés

**Symptômes** : "Total samples trouvés : 0"

**Solutions** :
1. ✅ Vérifier structure répertoires `/SAMPLES/`
2. ✅ Contrôler format fichiers (.wav)
3. ✅ Éviter caractères spéciaux dans noms
4. ✅ Vérifier spécifications audio

### ❌ Audio dégradé

**Symptômes** : Clics, distorsion, coupures

**Solutions** :
1. ✅ Utiliser carte SD Class 10
2. ✅ Réduire taille samples (<200KB)
3. ✅ Préférer mono au stéréo
4. ✅ Vider cache si saturé

### ❌ Mémoire insuffisante

**Symptômes** : "Impossible d'allouer la mémoire"

**Solutions** :
1. ✅ Réduire nombre de samples chargés
2. ✅ Utiliser samples plus courts
3. ✅ Vider cache avec `sdClearCache()`
4. ✅ Redémarrer le système

## 📋 Checklist de Validation

### ✅ Hardware
- [ ] Carte SD insérée et détectée
- [ ] Connexions SPI correctes
- [ ] Alimentation stable (USB ou externe)

### ✅ Software  
- [ ] Système SD initialisé avec succès
- [ ] Samples scannés et détectés
- [ ] Cache fonctionnel (chargement/déchargement)
- [ ] Audio sans distorsion

### ✅ Samples
- [ ] Format WAV PCM 16-bit 44.1kHz
- [ ] Taille < 200KB par fichier
- [ ] Noms sans caractères spéciaux
- [ ] Organisation en répertoires

### ✅ Interface
- [ ] Navigation avec encodeur
- [ ] Boutons SD fonctionnels
- [ ] Assignation aux voix
- [ ] Feedback visuel (LEDs)

## 🎵 Exemples de Samples

### Pack de Démarrage Recommandé

```
KICKS/ (4 samples)
├── kick_acoustic_01.wav (80ms, mono)
├── kick_electronic_01.wav (120ms, mono) 
├── kick_sub_01.wav (200ms, mono)
└── kick_vintage_01.wav (150ms, mono)

SNARES/ (4 samples)  
├── snare_acoustic_01.wav (180ms, mono)
├── snare_electronic_01.wav (100ms, mono)
├── rimshot_01.wav (50ms, mono)
└── clap_01.wav (200ms, mono)

HIHATS/ (4 samples)
├── hihat_closed_01.wav (80ms, mono)
├── hihat_open_01.wav (300ms, mono)
├── hihat_pedal_01.wav (100ms, mono)
└── hihat_ride_01.wav (400ms, mono)
```

**Taille totale** : ~1.5MB  
**Temps de chargement** : ~2-3 secondes  
**Utilisation RAM** : ~300KB

## 🔗 Ressources Utiles

### Outils de Conversion
- **Audacity** (gratuit) : https://audacityteam.org/
- **FFmpeg** (ligne de commande) : https://ffmpeg.org/
- **Reaper** (commercial) : https://reaper.fm/

### Banques de Samples Libres
- **Freesound** : https://freesound.org/
- **Zapsplat** : https://zapsplat.com/
- **BBC Sound Effects** : https://sound-effects.bbcrewind.co.uk/

### Documentation Technique
- ESP32 SPI : https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html
- Format WAV : http://soundfile.sapp.org/doc/WaveFormat/

---

🎛️ **Prêt à faire de la musique avec votre DRUM_2025_VSAMPLER !** 🎛️