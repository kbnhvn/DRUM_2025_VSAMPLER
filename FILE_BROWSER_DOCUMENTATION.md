# FILE BROWSER - DRUM_2025_VSAMPLER
## Documentation Complète de l'Application

### CONTEXTE DU PROJET
L'application File Browser est maintenant complètement intégrée au projet DRUM_2025_VSAMPLER, offrant une navigation intuitive des samples sur carte SD avec preview audio et chargement dynamique dans les slots du synthétiseur.

---

## FONCTIONNALITÉS IMPLÉMENTÉES

### ✅ Navigation Complète des Samples SD
- **Scan automatique** des répertoires : `/SAMPLES/KICKS/`, `/SAMPLES/SNARES/`, `/SAMPLES/HIHATS/`, etc.
- **Navigation hiérarchique** : support des sous-répertoires illimités
- **Remontée parent** : fonction ".." pour revenir au niveau supérieur
- **Affichage dynamique** : nom des fichiers, type (DIR/WAV), statut de validité

### ✅ Preview Audio Intégré
- **Preview instantané** des samples sélectionnés via le bouton PLAY
- **Utilisation du synthétiseur existant** : intégration avec `synthESP32_TRIGGER()`
- **Timeout automatique** : arrêt après 5 secondes
- **Slot dédié** : utilise le slot 15 pour les previews sans perturber la composition

### ✅ Chargement Dynamique des Samples
- **Chargement intelligent** dans les slots du synthétiseur (0-14)
- **Auto-incrémentation** : passage automatique au slot suivant après chargement
- **Intégration hybride** : support des samples intégrés + samples SD
- **Feedback visuel** : indication du slot cible et statut de chargement

### ✅ Interface Utilisateur Intuitive
- **Layout optimisé** pour l'écran tactile 480x320
- **Feedback tactile** : indicateurs visuels de sélection et d'état
- **Couleurs cohérentes** : utilise la palette existante (ZGREEN, ZRED, ZYELLOW, etc.)
- **Navigation fluide** : scroll automatique et sélection visuelle

---

## ARCHITECTURE TECHNIQUE

### Intégration dans l'Écosystème Existant
```cpp
// Utilise l'API SD existante
sdScanSamples("/SAMPLES/")     // Scanner répertoires
sdLoadSample(slot, path)       // Charger sample dans cache
assignSampleToVoice()          // Assigner à une voix du synthé
synthESP32_TRIGGER(slot)       // Jouer preview

// Réutilise les fonctions d'affichage
drawBT(zone, color, text)      // Boutons interface
gfx->setTextColor()            // Couleurs cohérentes
BPOS[zone]                     // Zones tactiles définies
```

### Variables d'État (intégrées dans menu_system.ino)
```cpp
char fbCurrentPath[128]        // Répertoire actuel
char fbFileNames[256][64]      // Noms des fichiers
char fbFilePaths[256][128]     // Chemins complets
byte fbFileTypes[256]          // Types : 0=parent, 1=dir, 2=sample
uint16_t fbFileCount           // Nombre d'items
uint16_t fbSelectedIndex       // Item sélectionné
byte fbTargetSlot              // Slot de destination
bool fbIsPreviewPlaying        // État preview
```

---

## INTERFACE UTILISATEUR

### Layout File Browser
```
┌─────────────────────────────────────┐
│          FILE BROWSER               │
│ Path: /SAMPLES/KICKS/               │
├─────────────────────────────────────┤
│ 📁 ../                              │  ← Zone 0
│ 📁 808/                             │  ← Zone 1
│ 🎵 kick_basic.wav                   │  ← Zone 2
│ > 🎵 kick_deep.wav     (sélectionné)│  ← Zone 3
│ 🎵 kick_punchy.wav                  │  ← Zone 4
│ 🎵 kick_sub.wav                     │  ← Zone 5
│ ...                                 │  ← Zones 6-9
├─────────────────────────────────────┤
│ [UP] [DOWN] [PLAY] [LOAD] [RETOUR]  │  ← Zones 16-20
├─────────────────────────────────────┤
│ Items: 4/12  Target Slot: 3         │
│ PREVIEW PLAYING...                  │
└─────────────────────────────────────┘
```

### Zones Tactiles
| Zone | Fonction | Description |
|------|----------|-------------|
| 0-9 | Sélection Liste | Sélectionner fichier/dossier dans la liste |
| 16 | Scroll UP | Remonter dans la liste |
| 17 | Scroll DOWN | Descendre dans la liste |
| 18 | PLAY | Preview du sample sélectionné |
| 19 | LOAD | Charger sample dans slot synthé |
| 20 | RETOUR | Retour au menu principal |

### Indicateurs Visuels
- **Couleurs** :
  - `ZGREEN` : Élément sélectionné, samples valides
  - `ZBLUE` : Répertoires
  - `ZCYAN` : Navigation parent (..)
  - `ZRED` : Preview en cours, erreurs
  - `ZYELLOW` : Bouton LOAD
  - `LIGHTGREY` : Texte normal

---

## INTÉGRATION AVEC LE MENU SYSTEM

### Navigation depuis le Menu Principal
1. **Démarrage** : L'utilisateur sélectionne "FILE BROWSER" dans le menu principal
2. **Initialisation** : `fileBrowserBegin()` vérifie la SD et scan `/SAMPLES/`
3. **Affichage** : Interface File Browser remplace le menu
4. **Retour** : Bouton RETOUR ramène au menu principal

### Gestion des Touches (keys.ino)
```cpp
// Intégration dans DO_KEYPAD()
if (getCurrentApplication() == APP_FILE_BROWSER) {
    fileBrowserHandleTouch(nkey);
    return;
}
```

### Cycle de Mise à Jour (DRUM_2025_VSAMPLER.ino)
```cpp
// Dans loop()
if (getCurrentApplication() == APP_FILE_BROWSER) {
    fileBrowserUpdate();
}
```

---

## GESTION DES ERREURS

### Scénarios Gérés
1. **SD Card absente** : Message d'erreur affiché, bouton retour disponible
2. **Répertoire vide** : Affichage "Aucun fichier trouvé"
3. **Sample corrompu** : Marqué en rouge, non chargeable
4. **Mémoire insuffisante** : Erreur logged, tentative avec slot suivant
5. **Timeout preview** : Arrêt automatique après 5 secondes

### Messages de Debug
- Tous les événements sont loggés sur `Serial` pour debug
- Statuts de chargement, erreurs, et navigation affichés
- Cache SD statistics disponibles via `sdPrintCacheStats()`

---

## TESTS ET VALIDATION

### Tests Fonctionnels Recommandés
1. **Navigation** :
   - Tester navigation dans plusieurs niveaux de répertoires
   - Vérifier remontée avec ".." jusqu'à `/SAMPLES/`
   - Valider scroll avec plus de 10 items

2. **Preview Audio** :
   - Tester preview de différents samples
   - Vérifier timeout automatique
   - Confirmer que preview n'interfère pas avec la composition

3. **Chargement Samples** :
   - Charger samples dans différents slots
   - Vérifier auto-incrémentation des slots
   - Tester intégration avec synthétiseur existant

4. **Interface Tactile** :
   - Valider toutes les zones tactiles (0-20)
   - Tester feedback visuel de sélection
   - Confirmer cohérence avec interface existante

### Tests de Performance
- **Temps de scan** : Scanner répertoire avec 100+ samples
- **Utilisation mémoire** : Monitorer avec `sdGetMemoryUsage()`
- **Réactivité interface** : Vérifier fluidité navigation
- **Stabilité** : Test longue durée avec cycles preview/chargement

---

## STRUCTURE DES FICHIERS

### Fichiers Modifiés
- **`menu_system.ino`** : Contient l'implémentation complète du File Browser
- **`keys.ino`** : Intégration du routage tactile
- **`DRUM_2025_VSAMPLER.ino`** : Déclarations et intégration setup/loop

### Fichiers Utilisés (inchangés)
- **`sd.ino`** : API SD existante pour chargement samples
- **`synthESP32.ino`** : Engine audio pour preview et lecture
- **`LCD_tools.ino`** : Fonctions d'affichage et couleurs
- **`touch.ino`** : Système de zones tactiles

### Compatibilité
- **100% compatible** avec l'architecture existante
- **Aucune modification** des APIs existantes
- **Intégration transparente** dans le workflow utilisateur

---

## UTILISATION PRATIQUE

### Workflow Typique
1. **Accès** : Menu Principal → "FILE BROWSER"
2. **Navigation** : Utiliser zones 0-9 pour sélectionner dossiers
3. **Exploration** : Entrer dans `/SAMPLES/KICKS/` par exemple
4. **Preview** : Sélectionner un sample et appuyer PLAY
5. **Chargement** : Appuyer LOAD pour intégrer dans le synthé
6. **Retour** : RETOUR pour revenir au menu principal

### Organisation Recommandée SD Card
```
/SAMPLES/
├── KICKS/
│   ├── 808/
│   ├── ACOUSTIC/
│   └── ELECTRONIC/
├── SNARES/
│   ├── CLAP/
│   └── RIM/
├── HIHATS/
│   ├── CLOSED/
│   └── OPEN/
├── PERCUSSION/
├── SYNTHS/
└── USER/
```

---

## CONCLUSION

Le File Browser est maintenant **complètement opérationnel** et s'intègre parfaitement dans l'écosystème DRUM_2025_VSAMPLER. Il offre une expérience utilisateur fluide pour la gestion des samples SD, avec preview audio, chargement dynamique, et navigation intuitive.

### Avantages Clés
- ✅ **Interface unifiée** : Cohérence visuelle avec le reste du système
- ✅ **Performance optimisée** : Utilise le cache SD existant intelligemment
- ✅ **Workflow intuitif** : Navigation familière type explorateur de fichiers
- ✅ **Intégration transparente** : Aucun impact sur les fonctionnalités existantes
- ✅ **Extensibilité** : Architecture permettant facilement nouvelles fonctionnalités

Le File Browser débloque maintenant la **gestion intuitive des samples SD** et complète l'infrastructure UI de la Phase 2 du projet DRUM_2025_VSAMPLER.