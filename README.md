# Zephyr — Foudre Audio

Synthétiseur polyphonique vintage pour sons riches, lumineux et subtils, bâti sur l'architecture d'Orage.
Deux plugins, même design :

- **Zephyr** (instrument VST3) : synthé 12 voix + rack d'effets. Onglets SYNTH / EFFECTS + volet MATRIX. Répond au MIDI.
- **Zephyr FX** (effet VST3) : le rack d'effets seul, pour les pistes audio.

## Synthé
- 2 oscillateurs **SAW / PULSE / TRI / SIN**, octaves, pitch / fine, PWM, **hard sync**, **FM**, **ring mod**, **sub-oscillateur** (carré, une octave sous l'OSC 1), bruit, glide.
- **DRIFT** : chaque voix dérive lentement et différemment (désaccord, cutoff, temps d'enveloppe), oscillateurs libres.
- **Filtre** au choix : **SEM** (12 dB, morph LP / notch / HP), **LADDER** (24 dB), **PROPHET** (24 dB façon CEM3320, graves conservés).
  HPF doux en amont, **SATURATION** dans la boucle du filtre (brillant à petite dose, claquant à fond).
- **POLY-MOD** façon Prophet : enveloppe du filtre et OSC 2 vers la fréquence ou la largeur d'impulsion de l'OSC 1, et/ou le filtre.
- Enveloppes ADSR filtre et ampli, **4 LFO** (SIN / TRI / SAW / SQR / S&H, slew, fade-in, retrig, synchro), **wavefolder**.
- **Matrice 12 slots** (menus rangés par familles) et **8 macros** pilotables en MIDI CC (16 à 23 = macros 1 à 8 de l'Hydrasynth, clic droit = learn).
- **Key Assign** : POLY / MONO / UNISON (8 voix empilées) / SHARE / CHORD MEMORY, HOLD. **Arpégiateur** façon Monopoly.

## Effets (rack réordonnable, prises de patch)
- **WARM SATURATION** : TAPE / CONSOLE / TUBE, DRIVE, TONE.
- **EQ** : LOW / MID / AIR.
- **FILTER** : HPF > LPF résonants.
- **PHASER** : 4 à 12 étages, MIX, RATE (ou synchro), DEPTH, FEEDBACK.
- **COMPRESSOR** : OPTO (doux, relâchement lent) ou GLUE (bus), RATIO, THRESHOLD, ATTACK, VU-mètre.
- **STRING ENSEMBLE** façon Solina : trois lignes, chorus lent + vibrato rapide, MIX, DEPTH, SPEED, TONE.
- **DELAY multimode** : TAPE (pleurage, saturation, s'assombrit) / ICE (chaque répétition monte d'une octave et se diffuse) /
  PING-PONG / DIGITAL (TONE = passe-bas, MOD = passe-haut) ; TIME ou synchro, FEEDBACK, TONE, MOD, MIX.
- **PLATE REVERB** : SIZE, DECAY, PRE-DELAY, DAMPING, MOTION, WIDTH, SHIMMER, LOW CUT, FREEZE.
- **STEREO WIDE** : WIDTH (mid/side), SPREAD (crée de la stéréo à partir d'un son mono), BASS MONO.
- Tous les modules démarrent neutres (mix et drive à zéro) ; les presets d'usine gardent leur son.

## Design
- Trois thèmes (bouton en haut) : **ALUMINIUM** (aluminium brossé et bleu, par défaut), **CREME** (crème, bois et ambre), **MINUIT** (bleu nuit).
- Plaques de modules vissées, flancs en bois, potards à jupe moletée et chapeau en aluminium, arc lumineux de la valeur.
- **Prisme** dans l'en-tête : un faisceau blanc entre, un éventail de couleurs sort ; il s'allume avec le niveau et s'ouvre avec la brillance du son.
  Un clic bascule sur l'**oscilloscope**.
- Fenêtre redimensionnable, cran magnétique au zéro des potards bipolaires.

## Compilation (GitHub Actions)
Chaque push sur `main` lance le build Windows et publie `Zephyr-VST3-Windows.zip` dans la Release **latest** du dépôt.
Dézipper et copier `Zephyr.vst3` et `Zephyr FX.vst3` dans `C:\Program Files\Common Files\VST3`.
L'ancien Zephyr (synthé à larsens) avait un autre identifiant : supprimer son `.vst3` avant de copier le nouveau.
