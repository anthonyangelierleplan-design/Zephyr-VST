# Zephyr — notes pour Claude

Synthé 12 voix + rack d'effets (Zephyr) et rack seul (Zephyr FX), JUCE 8 / C++17. Tout est décrit dans :
- `README.md` : ce que fait le plugin aujourd'hui ; `CHANGELOG.md` : l'historique, avec les mesures ;
- `KNOWLEDGE.md` : base de connaissances Foudre Audio + sections propres à Zephyr (filtre PROPHET, POLY-MOD, ensemble, presets) — **à lire avant de toucher au moteur** ;
- `tools/tests/LISEZMOI.md` : les tests.

Carte du code : `Source/Synth.h` (voix, VCO, filtres circuit + PROPHET, dérive, enveloppes, LFO), `Source/Engine.h` (rack d'effets), `Source/Arp.h` (arpégiateur / séquenceur, POLY façon Mono/Poly), `Source/PluginProcessor.cpp` (paramètres, MIDI, état sauvé, presets d'usine), `Source/PluginEditor.cpp` (rack, thèmes, potards), `Source/SynthEditor.cpp` (pages SYNTH / ARP / MATRIX). `ZEPHYR_SYNTH=1` → instrument, `0` → Zephyr FX.

Identité à garder : sons riches, luxuriants, vintage mais lumineux et subtils ; design aluminium brossé / plaques vissées / bois, prisme dans l'en-tête ; pas de patchbay (il la trouvait de trop).

## Qui, comment
- Le propriétaire est musicien, pas développeur (il se dit novice). Parle-lui **en français**, simplement, sans jargon inutile. Sois franc : si une méthode ou un outil plus efficace existe, propose-le d'emblée.
- Le partage des rôles : lui = les oreilles et les mains (il teste dans Ableton avec son **ASM Hydrasynth Keyboard** et un **Expressive E Osmose**), Claude = le code. Claude ne peut pas écouter : il mesure (niveaux, spectre, clics, repliement) et le laisse trancher à l'oreille.
- Avant un gros chantier ou un choix de design : montre un aperçu (capture, maquette) **avant** de coder ou de lancer un build.

## Règles de la gamme Foudre Audio (à garder d'un plugin à l'autre)
- Fenêtre redimensionnable ; cran magnétique au zéro des potards ; **démarrage neutre** (rien d'agressif à l'ouverture, modules d'effets à zéro quand on les active).
- Potards crantés pour les choix, libellés jamais coupés ; rien ne se chevauche ni n'est masqué.
- Les anciens projets et presets doivent se rouvrir **à l'identique** (toute nouveauté est neutre par défaut ; versionner les formats sauvegardés).
- Presets : niveaux mesurés et calés sur leur catégorie ; seuls les modules réellement utilisés sont activés.
- Les thèmes clairs actuels sont jugés trop peu lisibles : il joue en thème sombre.

## Circuit de travail
1. Une branche par version (`preview/...` ou `vX-nom`), jamais directement sur `main`.
2. Avant d'envoyer : la checklist de `KNOWLEDGE.md` (`bash tools/tests/run.sh` sans échec — aussi lancé automatiquement par GitHub avant le build), README (état actuel) et CHANGELOG (nouvelle version en haut) à jour.
3. Push de la branche → GitHub Actions compile (Windows). Il teste. **Fusion sur `main` seulement après son « go »** : `main` publie la Release `latest` qu'il télécharge.
4. Jamais de fichiers générés dans le dépôt (`build/`, `.raw`, `.wav`, `.o`). Ne pas mettre l'adresse e-mail du propriétaire dans les commits ou les fichiers.

## Compiler sur son PC (Windows)
- Pré-requis : Git, CMake, Visual Studio 2022 Build Tools (C++). JUCE 8.0.4 est téléchargé par CMake.
- `cmake -B build -G "Visual Studio 17 2022"` puis `cmake --build build --config Release`.
- Hors GitHub, le build copie les `.vst3` dans `C:\Program Files\Common Files\VST3` (il faut un terminal lancé en administrateur, sinon copier à la main depuis `build\*_artefacts\Release\VST3`). Il doit relancer Ableton (ou rescanner) pour charger la nouvelle version.

## La famille (dépôts GitHub de anthonyangelierleplan-design)
- **Orage-Synth** : Orage + Orage FX. Le plus brut / expérimental. (Orage-FX-V3 = ancien dépôt.)
- **Zephyr-VST** : Zephyr + Zephyr FX. Bâti sur le moteur d'Orage (v20b puis v22c), identité propre : lumineux, vintage, filtre PROPHET calé sur un Prophet-6.
- **Thunder-VST** : claviers vintage créatifs (orgue à roues, Rhodes, Wurlitzer, Vox, Clavinet), moteur de synthèse (pas de samples).
- Orage et Zephyr ont divergé : une fonction commune se reporte **à la main**, en gardant l'identité de chacun (ex. Orage v23 → Zephyr v4). Projet décidé : un dépôt commun **Foudre-Core** (ergonomie, thèmes, arpégiateur / séquenceur, modules identiques comme la cassette) ; les modules qui font la couleur de chaque synthé restent dans leur dépôt.

## Où on en est (26 nov. 2026)
- En test (27 nov. 2026) : **v5** sur sa branche (Chord Seq, Arp/Seq rangé, filtres au même volume), pas encore fusionnée.
  Code partagé : `FoudreAccords.h` de **Foudre-Core** (FetchContent, figé sur un commit dans `CMakeLists.txt`). Captures de l'interface : `foudre-banc capture <vst3> --onglet 1` (le plugin lit `FOUDRE_ONGLET`).
- Dernière version : **v4** (module CASSETTE / VHS, arpège CHORD, vitesse par pas, 9 presets cassette, profil Osmose CE, THRU), fusionnée sur `main`. Dépôt nettoyé (README / CHANGELOG / tests automatiques).
- Ses goûts : adore les 11 modules et l'arpégiateur / séquenceur ; joue en thème MINUIT.
- La grille de stabilité signale 28 réglages, tous PROPHET à résonance ≥ 80 % : auto-oscillation voulue depuis le calage Prophet-6 (voir README), pas un bug.
- Idées discutées, pas commencées (lui demander par quoi commencer) :
  - onde des LFO animée à leur vitesse sur la page SYNTH (~60 images/s, ne redessiner que l'écran du LFO) ;
  - thèmes clairs (ALUMINIUM, CREME) : plus de contraste ;
  - son un peu « mat » comparé à l'Hydrasynth : mesurer le spectre avant de corriger, éventuellement un réglage de brillance neutre par défaut ;
  - banc d'essai local : un hôte sans Ableton qui charge le `.vst3`, joue du MIDI, enregistre en WAV et compare à un enregistrement de référence ;
  - dépôt Foudre-Core (voir plus haut).
