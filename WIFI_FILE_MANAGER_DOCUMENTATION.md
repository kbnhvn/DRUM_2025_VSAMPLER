# 📶 WiFi File Manager - DRUM 2025 VSAMPLER
## Documentation Utilisateur Complète

### Version 1.0 - ZCARLOS 2025

---

## 🎯 PRÉSENTATION GÉNÉRALE

Le **WiFi File Manager** est un module avancé du DRUM 2025 VSAMPLER qui permet la gestion à distance des échantillons audio via une interface web moderne. Il offre une solution complète pour uploader, télécharger et organiser vos samples WAV directement depuis votre navigateur web.

### ✨ Fonctionnalités Principales

- **🌐 Interface Web Responsive** : Gestion intuitive depuis tout appareil (PC, tablette, smartphone)
- **📤 Upload de Samples** : Glissez-déposez vos fichiers WAV directement dans le navigateur
- **📥 Téléchargement Facile** : Sauvegardez vos samples créés sur votre ordinateur
- **🔒 Sécurité Intégrée** : Protection des fichiers système, seuls les samples utilisateur sont modifiables
- **🎵 Priorité Audio** : Désactivation automatique du WiFi pendant l'utilisation audio
- **📊 Monitoring Temps Réel** : Suivi des transferts et statistiques d'utilisation
- **🔧 Configuration Flexible** : Mode Client WiFi ou Access Point selon vos besoins

---

## 🚀 DÉMARRAGE RAPIDE

### 1. Accéder au WiFi Manager

1. **Depuis le menu principal** du DRUM 2025 VSAMPLER
2. **Appuyez sur la zone tactile 17** ou sélectionnez **"WIFI SETTINGS"**
3. L'interface WiFi Manager s'affiche à l'écran

### 2. Configuration Initiale

#### Mode Access Point (Recommandé pour débuter)
```
1. Sélectionnez [ACCESS POINT] (zone 21)
2. Appuyez sur [DÉMARRER] (zone 22)
3. Attendez l'activation (5-10 secondes)
4. Connectez-vous au réseau "DRUM_2025_AP" depuis votre appareil
5. Mot de passe par défaut : "DrumSampler2025"
6. Ouvrez votre navigateur et allez à : http://192.168.4.1
```

#### Mode Client WiFi (Connexion à votre réseau)
```
1. Appuyez sur [SCAN] (zone 25) pour détecter les réseaux
2. Sélectionnez votre réseau (zones 16-19)
3. Saisissez le mot de passe si nécessaire
4. Appuyez sur [DÉMARRER] (zone 22)
5. L'IP locale s'affiche à l'écran
6. Ouvrez cette IP dans votre navigateur
```

---

## 💻 INTERFACE WEB

### Page d'Accueil

L'interface web moderne présente :

#### 📊 Barre de Statut
- **🟢 Réseau** : État de la connexion WiFi
- **💾 SD Card** : Disponibilité de la carte SD  
- **🎵 Audio** : État du système audio (priorité)
- **👥 Clients** : Nombre d'appareils connectés

#### 📤 Zone d'Upload
- **Glissez-déposez** vos fichiers WAV
- **Ou cliquez** pour parcourir vos fichiers
- **Formats supportés** : WAV 16-bit mono/stéréo
- **Taille maximum** : 10MB par fichier

#### 📁 Bibliothèque de Samples
- **Liste interactive** de tous vos samples
- **Informations détaillées** : taille, durée, canaux
- **Actions rapides** : télécharger, supprimer
- **Tri automatique** par nom et date

#### 📈 Statistiques Système
- **Nombre total** de samples
- **Espace utilisé** sur la SD
- **Compteurs** d'uploads/downloads
- **Mise à jour temps réel** toutes les 5 secondes

---

## 🎮 CONTRÔLES TACTILES

### Interface Principal WiFi Manager

| Zone | Fonction | Description |
|------|----------|-------------|
| **16-19** | **RÉSEAUX 1-4** | Sélection des réseaux WiFi détectés |
| **20** | **CLIENT MODE** | Mode connexion à réseau existant |
| **21** | **ACCESS POINT** | Mode création de réseau propre |
| **22** | **DÉMARRER/ARRÊTER** | Activation/désactivation du WiFi |
| **23** | **ARRÊT** | Arrêt complet du WiFi |
| **24** | **RETOUR** | Retour au menu principal |
| **25** | **SCAN** | Recherche des réseaux disponibles |
| **28** | **CONFIG** | Configuration avancée |

### États d'Interface

#### 🔴 WiFi Désactivé
```
┌─────────────────────────────────────┐
│           WiFi MANAGER              │
│ Status: DÉSACTIVÉ    [SCAN] [CONFIG]│
├─────────────────────────────────────┤
│ Mode actuel: CLIENT                 │
│ SSID configuré: MonReseauWiFi       │
│ Réseaux détectés: 4                 │
├─────────────────────────────────────┤
│ Réseaux disponibles :               │
│ ● MonReseauWiFi    ████░  [CONNECT] │
│ ● WiFiVoisin       ███░░  [CONNECT] │
│ ● FreeWiFi         ██░░░  [CONNECT] │
├─────────────────────────────────────┤
│ [CLIENT] [ACCESS POINT] [DÉMARRER]  │
│              [RETOUR]               │
└─────────────────────────────────────┘
```

#### 🟢 WiFi Actif avec Serveur
```
┌─────────────────────────────────────┐
│         WiFi ACTIVE - SERVEUR       │
│ 🌐 http://192.168.1.100             │
├─────────────────────────────────────┤
│ 📊 Statistiques :                  │
│ Uploads: 3    Downloads: 7          │
│ Clients: 2    Actif depuis: 5min    │
├─────────────────────────────────────┤
│ Transfert actuel: sample_kick.wav   │
│ Progress: ████████░░ 80%            │
├─────────────────────────────────────┤
│ [PAUSE] [ARRÊTER] [RETOUR]          │
└─────────────────────────────────────┘
```

#### 🟡 Scan en Cours
```
┌─────────────────────────────────────┐
│           WiFi MANAGER              │
│ Status: SCAN EN COURS...            │
├─────────────────────────────────────┤
│         📡 Recherche...             │
│            •••                      │
│                                     │
│     Veuillez patienter              │
├─────────────────────────────────────┤
│              [RETOUR]               │
└─────────────────────────────────────┘
```

---

## 🔧 CONFIGURATION AVANCÉE

### Paramètres Réseau

#### Mode Client
- **SSID** : Nom du réseau WiFi à rejoindre
- **Mot de passe** : Clé de sécurité WPA/WPA2
- **IP automatique** : Attribution DHCP
- **Timeout connexion** : 15 secondes

#### Mode Access Point
- **SSID par défaut** : "DRUM_2025_AP"
- **Mot de passe** : "DrumSampler2025"
- **Canal** : 1 (2.4GHz)
- **Clients maximum** : 4 simultanés
- **IP fixe** : 192.168.4.1

### Sécurité et Limitations

#### Fichiers Autorisés
- **Répertoire accessible** : `/SAMPLES/USER/` uniquement
- **Formats supportés** : WAV 16-bit PCM
- **Fréquences** : 44.1kHz recommandé (autres acceptées)
- **Canaux** : Mono ou stéréo
- **Taille maximum** : 10MB par fichier

#### Protection Système
- **Fichiers système** : Inaccessibles via web
- **Samples intégrés** : Lecture seule
- **Répertoires protégés** : `/SOUNDS/`, `/SYSTEM/`

---

## ⚠️ PRIORITÉ AUDIO - SYSTÈME CRITIQUE

### Fonctionnement Automatique

Le WiFi Manager intègre un **système de priorité audio critique** qui garantit que les performances audio ne sont jamais compromises :

#### Désactivation Automatique
```
🎵 AUDIO DÉTECTÉ → 🔴 WiFi ARRÊTÉ IMMÉDIATEMENT
```

#### Conditions de Désactivation
- **Déclenchement de samples** (pads, séquenceur)
- **Enregistrement audio** (sampling studio)
- **Lecture de patterns** temps réel
- **Utilisation WM8731** (entrée/sortie audio)

#### Indicateurs Visuels
- **🔴 "AUDIO ACTIF"** : WiFi temporairement désactivé
- **🟢 "AUDIO LIBRE"** : WiFi peut être activé
- **⏰ Auto-timeout** : 10 minutes d'inactivité

#### Messages Système
```
🔊 AUDIO ACTIF - Désactivation WiFi immédiate
🔇 Audio arrêté - WiFi peut être réactivé
❌ Impossible de démarrer WiFi - Audio actif
```

---

## 📡 API REST TECHNIQUE

### Endpoints Disponibles

#### Gestion des Fichiers
```
GET  /api/files          # Liste des samples
POST /api/upload         # Upload nouveau sample
GET  /download?file=...  # Téléchargement sample
DELETE /api/delete?file=... # Suppression sample
```

#### Informations Système
```
GET /api/status          # Statut temps réel
GET /api/info           # Informations système
```

#### Réponses JSON
```json
{
  "name": "kick_drum.wav",
  "size": 524288,
  "duration": 1200,
  "channels": 2,
  "sampleRate": 44100,
  "path": "/SAMPLES/USER/kick_drum.wav"
}
```

---

## 🐛 DÉPANNAGE

### Problèmes Courants

#### WiFi ne se connecte pas
```
✅ Solutions :
• Vérifier le mot de passe WiFi
• Rapprocher l'appareil du routeur
• Redémarrer le scan des réseaux
• Utiliser le mode Access Point
```

#### Interface web inaccessible
```
✅ Solutions :
• Vérifier l'IP affichée à l'écran
• Contrôler la connexion réseau
• Redémarrer le serveur web
• Vider le cache du navigateur
```

#### Upload ne fonctionne pas
```
✅ Solutions :
• Vérifier l'espace libre sur SD
• Contrôler le format WAV du fichier
• Taille maximum 10MB respectée
• Arrêter la lecture audio
```

#### WiFi se désactive automatiquement
```
✅ Normal ! C'est la priorité audio :
• Arrêter la lecture/enregistrement
• Attendre 5 secondes après l'audio
• Réactiver manuellement le WiFi
```

### Messages d'Erreur

| Message | Cause | Solution |
|---------|-------|----------|
| **"Audio system active"** | Audio en cours | Arrêter l'audio puis réessayer |
| **"SD card not available"** | Carte SD manquante | Insérer une carte SD formatée |
| **"File too large"** | Fichier > 10MB | Réduire la taille ou diviser |
| **"Format not supported"** | Fichier non-WAV | Convertir en WAV 16-bit |
| **"Connection timeout"** | Réseau inaccessible | Vérifier SSID/mot de passe |

---

## 📊 OPTIMISATION PERFORMANCES

### Conseils d'Utilisation

#### Transferts Optimaux
- **Uploads multiples** : 2-3 fichiers simultanés maximum
- **Taille recommandée** : 1-5MB par sample pour rapidité
- **Format optimal** : WAV 16-bit 44.1kHz mono
- **Réseau stable** : Signal WiFi > 50% recommandé

#### Gestion Mémoire
- **Cache intelligent** : Système LRU automatique
- **Mémoire disponible** : Surveillée en temps réel
- **Auto-nettoyage** : Samples non utilisés libérés
- **Fragmentation** : Évitée par allocation contiguë

#### Sécurité Réseau
- **Changement mot de passe** : Recommandé en AP mode
- **Réseau privé** : Préférer WiFi domestique
- **Timeout automatique** : 10 minutes d'inactivité
- **Clients limités** : 4 connexions simultanées max

---

## 🔄 INTÉGRATION SYSTÈME

### Avec Autres Modules

#### Système SD Card
- **Cache partagé** : Optimisation mémoire commune
- **Samples SD** : Accessibles via web interface
- **Sauvegarde auto** : Configuration persistante
- **Monitoring espace** : Alerte espace libre

#### WM8731 Audio Codec
- **Priorité absolue** : Audio toujours prioritaire
- **Détection activité** : Monitoring temps réel
- **Isolation parfaite** : Pas d'interférence WiFi
- **Qualité préservée** : 44.1kHz/16-bit garantis

#### Menu System
- **Navigation fluide** : Intégration transparente
- **État persistant** : Configuration sauvegardée
- **Bouton retour** : Sécurité arrêt WiFi
- **Indicateurs visuels** : Statut temps réel

---

## 📋 SPÉCIFICATIONS TECHNIQUES

### Compatibilité Hardware
- **Microcontrôleur** : ESP32-S3
- **Module WiFi** : 802.11 b/g/n 2.4GHz
- **Mémoire RAM** : 512KB minimum disponible
- **Stockage** : Carte SD Class 10 recommandée
- **Écran tactile** : 480x320 compatible

### Limites Système
- **Clients simultanés** : 4 maximum
- **Taille fichier** : 10MB maximum
- **Formats supportés** : WAV PCM 16-bit uniquement
- **Fréquences** : Toutes (44.1kHz optimal)
- **Timeout connexion** : 15 secondes
- **Auto-désactivation** : 10 minutes inactivité

### Consommation
- **Mode désactivé** : 0mA supplémentaire
- **Mode connecté** : +50-80mA
- **Transfert actif** : +100-150mA
- **Impact audio** : Aucun (priorité garantie)

---

## 🎓 GUIDE UTILISATEUR AVANCÉ

### Workflow Optimal

#### 1. Préparation Studio
```
1. Organiser vos samples sur ordinateur
2. Convertir en WAV 16-bit 44.1kHz
3. Nommer clairement (kick_01.wav, snare_02.wav...)
4. Vérifier tailles < 10MB chacun
```

#### 2. Session de Travail
```
1. Activer WiFi Manager en mode AP
2. Connecter tablette/ordinateur au réseau
3. Ouvrir interface web (bookmark conseillé)
4. Uploader nouveaux samples
5. Tester samples dans le DRUM 2025
6. Sauvegarder créations finales
7. Arrêter WiFi avant performance live
```

#### 3. Sauvegarde Projet
```
1. Télécharger tous samples modifiés
2. Exporter configuration système
3. Archiver sur ordinateur principal
4. Nettoyer samples temporaires
```

### Conseils Pro

#### Organisation Samples
- **Nommage cohérent** : Style_BPM_Note (ex: House_128_C)
- **Dossiers virtuels** : Préfixes (KICK_, SNARE_, HAT_)
- **Versions multiples** : V1, V2, V3 pour variations
- **Métadonnées** : Durée et tonalité dans le nom

#### Performances Live
- **WiFi OFF obligatoire** : Priorité audio absolue
- **Samples pré-chargés** : Tout en local avant show
- **Backup SD** : Carte de secours préparée
- **Monitoring batterie** : WiFi consomme +30%

#### Production Studio
- **WiFi pendant pauses** : Économie batterie
- **Uploads groupés** : Efficacité transferts
- **Preview web** : Test samples avant import
- **Historique transferts** : Traçabilité projets

---

## 🆘 SUPPORT ET ASSISTANCE

### Contacts
- **Développeur** : ZCARLOS 2025
- **Version** : 1.0
- **Compatibilité** : DRUM 2025 VSAMPLER v2.0+

### Ressources
- **Documentation complète** : Ce fichier
- **Code source** : wifi_manager.ino + wifi_manager_interface.ino
- **Tests unitaires** : Intégrés au système
- **Logs système** : Monitoring Serial à 115200 bauds

### Contribution
Le WiFi File Manager est un module open-source du projet DRUM 2025 VSAMPLER. Les contributions, suggestions d'amélioration et rapports de bugs sont les bienvenus.

---

## 📄 LICENCE ET ATTRIBUTION

**DRUM 2025 VSAMPLER - WiFi File Manager**
© 2025 ZCARLOS - Tous droits réservés

*Développé avec passion pour la communauté des beatmakers et producteurs électroniques.*

---

**🎵 Que la musique soit avec vous ! 🎵**