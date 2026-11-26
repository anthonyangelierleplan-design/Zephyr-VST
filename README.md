# Zephyr — Foudre Audio

Synthétiseur polyphonique vintage pour sons riches, lumineux et subtils, bâti sur le moteur d'Orage avec sa propre identité. Deux plugins, même design, même dépôt :

- **Zephyr** (instrument VST3) : synthé 12 voix + rack d'effets. Onglets SYNTH / ARP / EFFECTS + volet MATRIX.
- **Zephyr FX** (effet VST3) : le rack d'effets seul, pour les pistes audio.

## Installer
Chaque push sur `main` publie `Zephyr-VST3-Windows.zip` dans la Release **latest** du dépôt.
Dézipper et copier `Zephyr.vst3` et `Zephyr FX.vst3` dans `C:\Program Files\Common Files\VST3` (remplacer l'ancienne version : mêmes identifiants, les projets se rouvrent).

## Synthé (onglet SYNTH, rangé selon le trajet du signal)
- **Sources** : 2 VCO SAW / PULSE / TRI / SIN, suréchantillonnés, modèle analogique ou numérique propre, phase libre ou recalée ; hard sync sans repliement, ring, **sub-oscillateur**, bruit, glide ; vibrato du VCO 1 (LFO AMT, + WHEEL, + AFTER).
- **MIXER & DRIFT** : chaque voix dérive lentement et différemment (accord, cutoff, temps d'enveloppe).
- **FM** : linéaire ou exponentielle, FEEDBACK, KEY SCALE, 18 rapports, 17 caractères (CHARACTER) et DICE.
- **Filtres circuit** : MS-10, LADDER CEM3320, SEM (morph), **PROPHET** (calé sur un Prophet-6 enregistré) ; passe-haut, saturation dans la boucle, KEY TRACK, LFO 2, courbe de réponse en direct.
- **POLY-MOD** façon Prophet-5 : enveloppe filtre et VCO 2 vers la fréquence ou la largeur du VCO 1, et/ou le filtre.
- **Enveloppes** ADSR filtre et ampli (LINK : ENV 2 libre), **4 LFO** de 0,05 à 1000 Hz, **wavefolder**.
- **Matrice 16 slots** avec un sens par routage, anneaux de modulation sur les potards, **8 macros** (CC 16 à 23).
- **KEY ASSIGN** : POLY / MONO / UNISON (8 voix empilées) / SHARE / CHORD MEMORY, HOLD ; changer de mode ne coupe pas le son.

## Arpégiateur et séquenceur (onglet ARP)
- Grille de 16 pas (silence / note / accent), **ratchet** et **vitesse** par pas (×1, ×2, ×4, ×½, ×3/2), ratchet global, triolets, tap tempo, swing, chance, gate.
- 7 modes : UP, DOWN, UP-DN, PLAYED, RANDOM, CONVRG, **CHORD** ; 1 à 4 octaves. FREE (horloge libre), **THRU** (les touches jouées sonnent aussi pendant que l'arpège tourne).
- Façon Mono/Poly : en **POLY**, les pas tournent sur 4 VCO (SHIFT décale chaque VCO, CLUSTER resserre en accord plaqué ou gratté) ; MONO / UNISON arpègent l'accord sur une voix.
- **SEQ** : une note mémorisée par pas, calé sur le DAW ; **8 pistes de modulation**.

## Effets (onglet EFFECTS / Zephyr FX)
- 11 modules dans un ordre « spectre », chacun avec sa couleur et son écran animé : WARM SATURATION (tape / console / tube), EQ, FILTER, PHASER, COMPRESSOR (opto / glue), STRING ENSEMBLE (Solina), STUTTER, DELAY (tape / ice / ping-pong / digital), PLATE REVERB + SHIMMER, **CASSETTE / VHS**, STEREO WIDE.
- Delay et reverb en SERIAL ou PARALLEL ; bypass = vrai A/B ; tous les modules démarrent neutres.
- Synthé : les effets se modulent par la matrice. Zephyr FX : LFO 1, LFO 2, S&H et suiveur d'enveloppe, chacun avec sa destination.

## MIDI et SETTINGS
- Carte CC de l'**Hydrasynth** par défaut (effets sur ses CC FX), MIDI learn par clic droit, reprise SCALE / PICKUP / DIRECT, lissage MIDI, courbe et réactivité de l'aftertouch.
- **MPE** (détection automatique, pression / CC74 / bend par note), **profil Osmose CE** (SETTINGS > CONTROLLER).
- Program Change → preset d'usine, pédale de sustain (arpège compris), étendue du pitch bend réglable.
- **Qualité audio** : Performance / Quality (défaut) / High Fidelity ; **VCO PHASE** et **VCO MODEL**. Réglages globaux sauvés avec le projet, jamais dans les presets.

## Design
- Trois thèmes : ALUMINIUM (défaut), CREME, MINUIT. Plaques vissées, flancs en bois, prisme dans l'en-tête (un clic : oscilloscope).
- Fenêtre redimensionnable, cran magnétique au zéro des potards, potards crantés pour les choix (MODE, OCTAVE, RATCHET…).
- 52 presets d'usine + Init (Zephyr FX : 7 presets d'effets + Init).

## Pour aller plus loin
- `CHANGELOG.md` : ce qui a changé à chaque version, avec les mesures.
- `KNOWLEDGE.md` : base de connaissances Foudre Audio (pièges, méthodes, checklist avant envoi).
- `tools/tests/LISEZMOI.md` : les tests. La grille de stabilité des filtres signale 28 réglages, tous sur le filtre PROPHET à résonance ≥ 80 % : depuis le calage sur le Prophet-6, le filtre commence à auto-osciller à ces réglages (raie qui suit le cutoff), comme le vrai.

## Compilation
JUCE 8.0.4 (téléchargé par CMake), C++17. GitHub Actions : les tests automatiques d'abord, puis le build Windows ; la Release `latest` n'est publiée que depuis `main`.
En local : `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release`.
