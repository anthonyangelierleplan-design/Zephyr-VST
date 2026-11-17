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

## v2c (section FX réorganisée, écrans animés)
- **Patchbay retiré.** Zephyr (synthé) : la matrice du synthé est l'unique système de modulation des effets (anneaux visibles sur les potards). Zephyr FX : chaque source (LFO 1, LFO 2, S&H, suiveur d'enveloppe) a un menu de destination ; destinations : saturation, wide, phaser, ensemble, filtre, **delay time / feedback / mix**, **reverb mix / decay**, dry/wet. Les câbles de modulation des anciens projets Zephyr FX sont convertis ; les câbles audio ne sont plus repris (chaîne normale du rack).
- **SERIAL / PARALLEL** (sur le delay et la reverb) : en parallèle, la reverb reçoit le même signal que le delay et les deux s'additionnent (la reverb ne traite pas les échos).
- **Rack pleine largeur**, modules moins hauts (5 visibles), un seul ordre partout (spectre : saturation → wide), tuiles colorées.
- **Presets du synthé** : le rack ne garde que les modules utilisés (réglage non neutre ou cible d'une macro / de la matrice).
- **Colonne de gauche** : OUTPUT seul dans le synthé (DRY/WET et ON restent dans Zephyr FX), CLEAR RACK, **RESET FX** (ne touche plus au synthé). Textes en anglais.
- **Bypass = vrai A/B** (réglages conservés) ; un module ajouté depuis sa tuile démarre à zéro.
- **Écrans animés** : chaque module affiche son état réel dans sa couleur — courbes d'EQ et de filtre, sinus saturé, transfert du compresseur avec point de fonctionnement et réduction de gain, creux du phaser qui glissent, lignes de l'ensemble, tranches du stutter au tempo, salves d'échos du delay, ondes de la reverb (figées en FREEZE), flèches du wide. 30 images/s, uniquement sur la page EFFECTS et pour les modules actifs.

## v2d (MPE : plus de tremblement des filtres en polyphonie)
- **Cause** : l'Hydrasynth en mode **MPE** joue chaque note sur son propre canal MIDI, avec sa pression (aftertouch) et son CC74 sur ce canal. Zephyr traitait la pression de canal comme une valeur unique pour tout le synthé : avec plusieurs notes, elle sautait sans cesse de la pression d'une touche à celle d'une autre, et le filtre de **toutes** les voix tremblait. Une seule note : pas de saut, pas de grésillement ; le réglage SETTINGS > aftertouch n'y changeait rien. Le moteur des filtres lui-même était sain (testé avec aftertouch polyphonique lent, rapide, tremblant, dense).
- **Correction** : détection automatique du MPE (notes tenues sur plusieurs canaux, ou canal qui change à chaque note). Alors :
  - chaque note suit **sa propre pression** (vrai aftertouch polyphonique) ; la source AFTERTOUCH globale prend la plus forte pression, stable ;
  - le **pitch bend** d'un canal de note ne touche que sa note (étendue MPE ±48 demi-tons) ; celui du canal 1 reste global ;
  - les **CC des canaux de notes** (CC74 « timbre » par note) ne pilotent plus les potards : seul le canal maître 1 le fait (sustain accepté partout).
  - Avec un clavier classique (un seul canal) rien ne change.
- Les contrôles continus (pression, bend, molette) ne découpent plus le rendu à l'échantillon près (au plus tous les 32 échantillons) : moins de charge avec un flux MIDI dense.
- Test : `tools/tests/midi_hydrasynth.cpp` (MPE : chaque note suit sa pression, CC74 sans effet sur le cutoff) ; jeu MPE identique à l'échantillon près au même jeu en aftertouch polyphonique.

## v3 (nouvelle interface et fonctions d'Orage v22c)
- **Page SYNTH redessinée selon le trajet du signal** : sources (VCO 1, VCO 2, MIXER & DRIFT) → **VC FILTER** au centre avec sa courbe en direct (MS-10, Ladder CEM3320, SEM, **Prophet**) → enveloppes avec leurs courbes ADSR, LFO / S&H. Bandeau du bas : FM, **POLY-MOD**, WAVEFOLDER, MACROS (nommées d'après leur destination), KEY ASSIGN, raccourci ARP. Flancs en bois (teinte du thème, aussi sur Zephyr FX), lunettes d'écran, anneaux lumineux fins à la place des chiffres 0..10.
- **FM** : onde sinus, FM **linéaire** ou exponentielle, **FEEDBACK**, **KEY SCALE**, 18 rapports, menu CHARACTER (17 caractères), DICE et MORE ; se combine avec le poly-mod.
- **Matrice** : un **sens** par routage (AVANT / ARRIÈRE / LES DEUX) ; les anciens projets et presets sont convertis pour sonner pareil.
- **MPE** : SETTINGS > MPE MODE et étendue de bend par note (±2 / 12 / 24 / 48), slide CC74 par note ; la détection automatique de la v2d reste active.
- Changement de mode KEY ASSIGN sans coupure (l'arpège continue, rotation POLY sur 4 VCO conservée) ; SETTINGS > CLEAR ALL ASSIGNMENTS / RESTORE HYDRASYNTH MAP.
- Drive du Ladder qui compense la perte de graves (visible sur la courbe), potard LFO du filtre sur le LFO 2, CHORD LEARN réparé, reprise SCALE corrigée, GATE de l'ARP recalé sur la plage 5..100 du CC 107, LFO de 0,05 à 1000 Hz (bloc de contrôle adaptatif pour les LFO rapides).
- Gardé de Zephyr : filtre Prophet, sub / drift / poly-mod, 12 voix, limiteur de bus et réserve de polyphonie, section FX v2c (sans patchbay, SERIAL / PARALLEL, écrans animés), thèmes Aluminium / Crème / Minuit, macros CC 16..23. Les tiroirs FX d'Orage ne sont pas repris (le patchbay a été retiré en v2c).
- **Filtre PROPHET calé sur un vrai Prophet-6** (LPF, dent de scie en Do, balayages du cutoff à 0 / 25 / 50 / 75 % de résonance et en auto-oscillation) :
  - la pente était déjà la bonne (±0,5 dB) ; la **loi du potard RESONANCE** suit maintenant celle du Prophet-6 : quasi pas de bosse jusqu'à 50 % (+2 dB), montée rapide jusqu'à +21 dB à 75 %, auto-oscillation tout en haut (pure, comme le vrai) ;
  - le pic perd un peu de hauteur au-dessus de 7 kHz, comme sur le vrai ; la courbe affichée suit la même loi ;
  - les 15 presets sur le filtre Prophet et les anciens projets gardent exactement leur son (réglage RES converti automatiquement).
- Tests : `tools/tests/` : `midi_hydrasynth`, `midi_mpe`, `modes_matrice`, `reprise_midi_chord`, `fm_lineaire`, `stabilite_filtres`, `prophet6_resonance`.

## v3b (corrections)
- **FM** : avec un rapport FM (1:2, 2:1, 3:1...), OCTAVE, TUNE et FINE du VCO 2 se reglent toujours, par-dessus le rapport (avant, ils n'avaient plus d'effet). CHARACTER et DICE remettent le VCO 2 au neutre pour que leur rapport s'entende tel quel.
- **Sequenceur sans arpegiateur** : TYPE = SEQ suffit, plus besoin d'allumer l'ARP (les touches transposent le motif ; LATCH reste disponible).
- **Sequenceur en tempo** : changer de note ne relance plus l'horloge ; elle continue meme entre deux notes. Quand le DAW joue (cadence synchronisee), les pas sont cales sur sa mesure. Une touche a peine en retard (< 30 ms) fait quand meme partir le pas en cours ; apres une longue pause (un motif entier), le motif repart du debut.
- Anciens projets : memes hauteurs et meme jeu (VCO 2 remis au neutre s'il etait desaccorde sans effet ; SEQ choisi avec ARP eteint -> type ARP).
- Test : `tools/tests/seq_tempo_fm`.

## Compilation (GitHub Actions)
Chaque push sur `main` lance le build Windows et publie `Zephyr-VST3-Windows.zip` dans la Release **latest** du dépôt.
Dézipper et copier `Zephyr.vst3` et `Zephyr FX.vst3` dans `C:\Program Files\Common Files\VST3` (remplacer la v1 : mêmes identifiants, les projets se rouvrent).
