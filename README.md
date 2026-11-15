# Zephyr — Foudre Audio

Synthétiseur polyphonique vintage pour sons riches, lumineux et subtils. **Zephyr v2** est reconstruit sur le dernier moteur d'Orage (v20b) en gardant toute l'identité de Zephyr.
Deux plugins, même design, même dépôt :

- **Zephyr** (instrument VST3) : synthé 12 voix + rack d'effets. Onglets SYNTH / ARP / EFFECTS + volet MATRIX. Répond au MIDI.
- **Zephyr FX** (effet VST3) : le rack d'effets seul, pour les pistes audio.

## Synthé
- 2 VCO **SAW / PULSE / TRI / SIN**, octaves, pitch / fine, PWM, **hard sync**, **FM** (module complet : direction, ratio, onde, enveloppe, palettes), **ring**, **sub-oscillateur** (carré une octave sous le VCO 1), bruit, glide.
- **Vibrato du VCO 1** : LFO AMT (LFO 1 → hauteur du VCO 1), + WHEEL, + AFTER.
- **DRIFT** : chaque voix dérive lentement et différemment (désaccord, cutoff, temps d'enveloppe), oscillateurs libres.
- **Filtres circuit** : **MS-10**, **LADDER CEM3320**, **SEM**, et **PROPHET** (pôles OTA type SSM2040, résonance qui s'arrondit en saturant, graves conservés). Passe-haut en amont, SATURATION dans la boucle, potard LFO 1 du filtre, SEM MORPH.
- **POLY-MOD** façon Prophet-5 : enveloppe du filtre et VCO 2 vers la fréquence (FREQ A) ou la largeur (PW A) du VCO 1, et/ou le filtre.
- Enveloppes ADSR filtre et ampli, **4 LFO** de 0,05 à **300 Hz** (SIN / TRI / SAW / SQR / S&H, slew, fade-in, retrig, synchro), **wavefolder**.
- **Matrice 16 slots** (menus en sous-menus par module), anneaux de modulation sur les potards, **8 macros** (CC 16 à 23 = macros 1 à 8 de l'Hydrasynth).
- **Key Assign** : POLY / MONO / UNISON (8 voix empilées) / SHARE / CHORD MEMORY, HOLD.

## Arpégiateur (page ARP)
- Arpégiateur / séquenceur d'Orage v20b : grille de 16 pas (silence / note / accent, ratchet par pas), ratchet global, triolets, tap tempo, swing, chance, gate, 6 directions, 1 à 4 octaves, mode SEQ avec enregistrement pas à pas, **8 pistes de modulation**.
- Façon **Mono/Poly**, comme dans Zephyr v1 :
  - **POLY** : les pas tournent sur 4 VCO (chacun réagit un peu différemment à la FM). **SHIFT** décale chaque VCO d'une fraction de pas, **CLUSTER** resserre les 4 notes d'un tour vers son début (à fond : accord plaqué ; avec un peu de SHIFT : accord gratté).
  - **MONO / UNISON** arpègent l'accord tenu sur une seule voix.
  - Changer de mode Key Assign **ne coupe pas l'arpège** : les notes en cours sont relâchées proprement, le motif continue.

## Effets (rack réordonnable, bypass par module, prises de patch)
- **WARM SATURATION** (TAPE / CONSOLE / TUBE), **EQ** (LOW / MID / AIR), **FILTER** (HPF > LPF), **PHASER** (4 à 12 étages), **COMPRESSOR** (OPTO / GLUE).
- **STRING ENSEMBLE** façon Solina, **DELAY multimode** TAPE / ICE / PING-PONG / DIGITAL, **PLATE REVERB + SHIMMER**, **STEREO WIDE**.
- **STUTTER / BEAT REPEAT** (nouveau, d'Orage) : repeat / reverse / gate / offbeat, divisions, feel, pilotable par les pistes de l'arpégiateur.
- Tous les modules démarrent neutres (mix et drive à zéro). Nouvelle instance et preset Init : ensemble + delay + reverb, mix à zéro.

## MIDI (adaptation Hydrasynth)
- **Carte CC par défaut** de l'Hydrasynth (comme Orage) : cutoff 74, résonance 71, saturation 50, keytrack 51, LFO 1 du filtre 52, quantité d'enveloppe 54, passe-haut 55/56, LFO 72/73/76/78, ENV1 81-84 (filtre), ENV2 85-88 (ampli), glide 5, bruit 3, master 7, etc.
- Effets de Zephyr sur les CC FX de l'Hydrasynth : **reverb** mix 91 / decay 65 / tone 67, **delay** mix 92 / time 15 / feedback 14 / tone 63, **Pre-FX → ensemble** 93, **Post-FX → phaser** 94.
- Arpégiateur : division 106, gate 107, mode 108, ratchet 109, chance 110, octaves 120, longueur 122. Macros 1-8 sur CC 16-23.
- **Program Change** → preset d'usine, **pédale de sustain** (arpège compris), étendue du **pitch bend** réglable (±1 à ±24).
- MIDI learn par clic droit sur n'importe quel potard, bouton MIDI (étiquettes CC), moniteur MIDI dans l'en-tête ; reprise SCALE / PICKUP / DIRECT.

## Panneau SETTINGS
- **Qualité audio** : *Performance* (filtres ×2), *Quality* (×4, défaut), *High Fidelity* (×8).
- **Lissage MIDI** (0 à 80 ms, défaut 20 ms), **aftertouch** réglable (courbe et réactivité), mode de reprise.
- Réglages globaux : sauvegardés avec le mapping MIDI et le projet, jamais dans les presets.

## Design
- Trois thèmes : **ALUMINIUM** (aluminium brossé et bleu, par défaut), **CREME** (crème, bois et ambre, façon PolyBrute), **MINUIT** (bleu nuit).
- Plaques vissées, flancs en bois, potards à jupe moletée et chapeau aluminium. Bouton d'activation (bypass) sur chaque module du rack.
- **Prisme** dans l'en-tête (il s'allume avec le niveau et s'ouvre avec la brillance) ; un clic bascule sur l'**oscilloscope**.
- Fenêtre redimensionnable, cran magnétique au zéro des potards bipolaires, potards crantés pour MODE / OCTAVE / RATCHET.

## v2 : historique des tests et des choix
- **Base** : moteur, interface et MIDI d'Orage v20b ; modules d'effets, thèmes, presets et arpégiateur Mono/Poly de Zephyr v1.
- **Filtre Prophet** : modèle à 4 pôles OTA (résolution trapézoïdale + Newton amorti, comme le CEM3320), retour de résonance écrêté. Grille de stabilité (`tools/tests/stabilite_filtres.cpp`, 1632 réglages par mode de qualité sur CEM3320, SEM et Prophet) : MS-10, CEM3320 et SEM sans parasite ; Prophet : 1 cas (accord de 5 notes, résonance 90 %), de l'intermodulation attendue à ce niveau de résonance.
- **Fuzz** : 120 réglages tirés au hasard (44,1 et 96 kHz, 3 qualités, 4 filtres, poly-mod, drift, sub) : aucune valeur invalide ni explosion.
- **CPU** (12 voix tenues + rack complet, un cœur du serveur de build, sans optimisation spécifique) : Performance 21–38 %, Quality 22–56 %, High Fidelity 35–98 % (CEM3320 et Prophet les plus coûteux). *High Fidelity avec 12 voix est réservé au rendu ou aux sons à peu de notes.*
- **Presets** : les 43 presets de Zephyr v1 sont conservés (filtres v1 → circuits : SEM → SEM, LADDER → CEM3320, PROPHET → Prophet). Niveau mesuré contre la v1 (accord tenu) : 29 presets à ±1,5 dB sans retouche, 14 recalés par un trim de sortie (de −3,6 à +2,4 dB). Timbres non écoutés : à juger à l'oreille.
- **Projets Zephyr v1** : filtres et destinations de matrice (SUB, POLY-MOD, DRIFT) migrés à l'ouverture, vitesse des LFO conservée, patch du rack lu (le stutter n'existait pas).
- **Test MIDI** (`tools/tests/midi_hydrasynth.cpp`) : collisions de CC, CC74, Program Change, sustain, bend, LFO du filtre, CC des effets, macros 16-23, rack de départ, MONO arpège l'accord, POLY tourne sur 4 VCO, l'arpège continue après un changement de mode, aucune note bloquée, migration v1 : **TOUT OK**.

## v2b (polyphonie propre, modules FX personnalisés)
- **Accords qui « s'entrechoquent » : corrigé.** Mesure : rendues une par une puis additionnées, les voix donnent exactement l'accord (écart −55 à −70 dB) ; les filtres sont donc indépendants. La cause était la sortie du synthé : la somme des 12 voix passait dans une saturation instantanée (tanh), et un accord dense (8 notes, résonance) l'attaquait jusqu'à 2,5 fois sa pleine échelle, ce qui faisait intermoduler les notes entre elles.
  Correction : 6 dB de réserve sur le bus des voix + un limiteur à gain lissé (attaque 0,8 ms, relâchement 180 ms) au lieu de la tanh, écrêtage doux seulement au-delà. Sur un accord de 8 notes, la part hors harmoniques passe de −20/−35 dB à −38/−63 dB, le niveau des filtres seuls (MS-10 et SEM : −56/−64 dB contre −20/−25 avant).
- Niveaux : les presets d'usine (et une nouvelle instance) remontent la sortie de 4 dB ; écart moyen avec la v1 −1,6 dB, crête maximale −7,5 dBFS. Les projets déjà sauvegardés en v2 sonnent 6 dB plus bas : remonter OUTPUT.
- **Modules FX personnalisés** : chaque module prend une couleur du spectre du prisme (du rouge de la saturation au violet de la reverb, magenta pour le stereo wide), une pastille de verre allumée quand il est actif, un filet lumineux sous le titre, un grand pictogramme gravé dans la plaque (sinus saturé, courbe d'EQ, passe-bas résonant, peigne du phaser, coude du compresseur, trois lignes de l'ensemble, tranches du stutter, échos qui s'éteignent, ondes de la reverb, flèche stéréo) et une référence gravée (Z-1 SAT … Z-10 WIDE). Les tuiles MODULES portent la même couleur.

## Compilation (GitHub Actions)
Chaque push sur `main` lance le build Windows et publie `Zephyr-VST3-Windows.zip` dans la Release **latest** du dépôt.
Dézipper et copier `Zephyr.vst3` et `Zephyr FX.vst3` dans `C:\Program Files\Common Files\VST3` (remplacer la v1 : mêmes identifiants, les projets se rouvrent).
