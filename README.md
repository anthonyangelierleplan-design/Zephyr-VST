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
- **Filtres circuit** : MS-10, LADDER CEM3320, SEM (morph), **PROPHET** (calé sur un Prophet-6 enregistré), **au même volume** à réglage égal (chacun garde son timbre) ; passe-haut, saturation dans la boucle, KEY TRACK, LFO 2, courbe de réponse en direct.
- **POLY-MOD** façon Prophet-5 : enveloppe filtre et VCO 2 vers la fréquence ou la largeur du VCO 1, et/ou le filtre.
- **Enveloppes** ADSR filtre et ampli (LINK : ENV 2 libre), **4 LFO** de 0,05 à 1000 Hz (écran animé : l'onde défile comme sur un oscilloscope, à la vitesse du LFO), **wavefolder**.
- **Matrice 16 slots** avec un sens par routage, anneaux de modulation sur les potards, **8 macros** (CC 16 à 23).
- **KEY ASSIGN** : POLY / MONO / UNISON (8 voix empilées) / SHARE / CHORD MEMORY, HOLD ; changer de mode ne coupe pas le son.

## Arpégiateur et séquenceur (onglet ARP)
- Un seul choix **OFF / ARP / SEQ** : ARP joue les touches tenues, SEQ joue les pas mémorisés (transposés par la touche jouée). Les réglages qui ne servent pas dans le mode choisi sont grisés.
- **HOLD** (les notes et le motif continuent quand on lâche les touches ; c'est aussi le HOLD de KEY ASSIGN), **RUN** (le séquenceur et les pistes tournent sans toucher le clavier), **THRU** (les touches jouées sonnent aussi).
- Horloge : **DIVISION** calée sur le tempo (dont 1/4T, 1/8T, 1/16T pour les triolets) ou **HZ (free)** : horloge libre réglée par le potard HZ (grisé sinon) ; TAP TEMPO ; swing, chance, gate.
- Grille de 16 pas (silence / note / accent), **ratchet** et **vitesse** par pas (×1, ×2, ×4, ×½, ×3/2) ; **ALL RATCHET** passe tous les pas au ratchet suivant.
- Grille lisible : numéros des pas au-dessus des cases, pas groupés par 4 (1, 5, 9, 13 en couleur), accent = case plus lumineuse, tête de lecture = cadre blanc ; les pistes de modulation suivent les mêmes groupes.
- **Sélection** : clic (ou glisser) sur les numéros des pas, ou MAJ + clic ; ÉCHAP désélectionne. **CTRL+D** duplique la sélection juste après elle (sans sélection : tout le motif, longueur doublée). **CTRL+Z** annule, **CTRL+Y** (ou CTRL+MAJ+Z) refait : pas, pistes, RANDOM, CLEAR, longueur… (64 retours ; cliquer d'abord dans la grille pour que le plugin reçoive le clavier).
- **Chord Seq** : en SEQ, REC puis jouer ; les notes jouées ensemble (moins de 60 ms d'écart) vont dans le même pas, jusqu'à 6 notes, et le pas affiche le nom de l'accord (Fm7, D♭maj7, C/E…). Bouton **CHORD / 1 NOTE** (1 NOTE = une note par pas, comme avant). Molette sur un pas = transpose l'accord entier.
- 7 modes d'arpège : UP, DOWN, UP-DN, PLAYED, RANDOM, CONVRG, **CHORD** ; 1 à 4 octaves.
- Façon Mono/Poly : en **POLY**, les pas tournent sur 4 VCO (SHIFT décale chaque VCO, CLUSTER resserre en accord plaqué ou gratté) ; MONO / UNISON arpègent l'accord sur une voix.
- **SEQ** calé sur le DAW ; **8 pistes de modulation**.

## Effets (onglet EFFECTS / Zephyr FX)
- 11 modules dans un ordre « spectre », chacun avec sa couleur et son écran animé : WARM SATURATION (tape / console / tube), EQ, FILTER, PHASER, COMPRESSOR (opto / glue), STRING ENSEMBLE (Solina), STUTTER, DELAY (tape / ice / ping-pong / digital), PLATE REVERB + SHIMMER, **CASSETTE / VHS**, STEREO WIDE.
- Delay et reverb en SERIAL ou PARALLEL ; bypass = vrai A/B ; tous les modules démarrent neutres.
- Synthé : les effets se modulent par la matrice. Zephyr FX : LFO 1, LFO 2, S&H et suiveur d'enveloppe, chacun avec sa destination.
- **TAPE** (remplace CASSETTE / VHS) : un magnétophone à cassette calé sur un **Philips D6410** mesuré (cassette type I). DRIVE (saturation de bande, 50 % = passe propre du Philips, 100 % = passe saturée), ALC (compression automatique d'enregistrement), COLOR (grave en moins, bosse d'aigus vers 8 kHz, plus rien au-dessus de 12 kHz), WOW et FLUTTER (50 % = Philips, au-delà : bande qui tangue), HISS (souffle, 100 % = celui du Philips, premiers crans discrets), HUM (50 Hz, 10 % = Philips), MIX, MONO. Tout à zéro : le son passe sans changement. Les projets d'avant gardent l'ancien CASSETTE dans le même emplacement (bouton « → TAPE » pour changer).

## MIDI et SETTINGS
- Carte CC de l'**Hydrasynth** par défaut (effets sur ses CC FX), MIDI learn par clic droit, reprise SCALE / PICKUP / DIRECT, lissage MIDI, courbe et réactivité de l'aftertouch.
- **MPE** (détection automatique, pression / CC74 / bend par note), **profil Osmose CE** (SETTINGS > CONTROLLER).
- Program Change → preset d'usine, pédale de sustain (arpège compris), étendue du pitch bend réglable.
- **Qualité audio** : Performance / Quality (défaut) / High Fidelity ; **VCO PHASE** et **VCO MODEL**. Réglages globaux sauvés avec le projet, jamais dans les presets.

## Design
- Trois thèmes : ALUMINIUM (défaut), CREME, MINUIT. Plaques vissées, flancs en bois, prisme dans l'en-tête (un clic : oscilloscope).
- Fenêtre redimensionnable, cran magnétique au zéro des potards, potards crantés pour les choix (MODE, OCTAVE, RATCHET…).
- 57 presets d'usine + Init, dont une catégorie **Organ** (Velvet Drawbars, Chapel Glow, Combo '66, Gospel Swirl, Tape Harmonium, et les pianos électriques Tine Piano et Wurli Gold) (Zephyr FX : 7 presets d'effets + Init).

## Pour aller plus loin
- `CHANGELOG.md` : ce qui a changé à chaque version, avec les mesures.
- `KNOWLEDGE.md` : base de connaissances Foudre Audio (pièges, méthodes, checklist avant envoi).
- `tools/tests/LISEZMOI.md` : les tests. La grille de stabilité des filtres signale 28 réglages, tous sur le filtre PROPHET à résonance ≥ 80 % : depuis le calage sur le Prophet-6, le filtre commence à auto-osciller à ces réglages (raie qui suit le cutoff), comme le vrai.

## Compilation
JUCE 8.0.4 (téléchargé par CMake), C++17. GitHub Actions : les tests automatiques d'abord, puis le build Windows ; la Release `latest` n'est publiée que depuis `main`.
En local : `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release`.
