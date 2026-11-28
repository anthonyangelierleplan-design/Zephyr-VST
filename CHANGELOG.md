# Zephyr — historique des versions

Le plus récent en haut (v2 et suivantes ; la v1 n'est pas documentée). L'état actuel du plugin est décrit dans le README.

## v8 (orgues, finitions visuelles)
- **Nouvelle catégorie Organ** (5 presets, ajoutés après les synthés : les numéros de Program Change existants ne bougent pas) : Velvet Drawbars (16' 8' 4' ronds, cabine lente), Chapel Glow (petit orgue de chapelle, longue reverb), Combo '66 (orgue à transistors, vibrato), Gospel Swirl (8' + quinte, percussion, lampe à l'aftertouch), Tape Harmonium (harmonium sur cassette TAPE). Cabine par phaser + ensemble, **molette = cabine rapide**. Volumes calés au banc sur les Keys / Bells, aucun clic ni écrêtage. Silk Organ reste dans Keys / Bells ; Tine Piano et Wurli Gold (pianos électriques) rejoignent Organ (même son, même numéro).
- Menu du clic droit : « Réassigner MIDI » s'affichait « RÃ©assigner » (accent mal converti).
- En-tête : l'écran du prisme et le bloc titre (sous-titre, « FOUDRE AUDIO… ») ne touchent plus le filet bleu sous l'en-tête.
- Titres des modules : la petite barre bleue devant le nom (à moitié cachée par la vis, sans information) est retirée ; le filet sous le titre reste.

## v7 (arpèges : volume et notes plus tenues ; séquenceur, LFO animés — repris d'Orage v26)
- **Presets Arp / Seq remontés à -27 LUFS** : ils sortaient de -35 à -44 LUFS, soit ~20 dB sous les nappes (les autres catégories : -20 à -34, médiane -24). Leurs notes sont courtes et pincées, et leur volume avait été calé comme celui d'un son tenu. Starlight Arp plafonne à -28,6 LUFS (OUTPUT au maximum, +12 dB). Crêtes entre -10 et -17 dBFS, aucun écrêtage.
- **Arpèges moins « pluck »** : Crystal Cascade, Prism Sequence, Monopoly Lights, Chord Pulse 82 et Pine Forest Signal ont des notes plus tenues (sustain ≥ 0,6, relâche ≥ 0,45, sustain du filtre ≥ 0,25, gate ≥ 75 %), toujours à -27 LUFS. Starlight Arp et Half-Speed Memory restent pincés exprès ; Film Society était déjà tenu.
- **Séquenceur** : sélection de pas (clic ou glisser sur les numéros, MAJ + clic, ÉCHAP), **CTRL+D** duplique (sélection ou motif entier), **CTRL+Z / CTRL+Y** annuler / refaire (64 retours).
- **Grille NOTE STEPS** : numéros lisibles, groupes de 4 pas (pistes alignées), accent = case plus lumineuse, vitesses plus grandes, tête de lecture = cadre blanc.
- **Écran des LFO animé** : l'onde défile comme sur un oscilloscope à la vitesse réelle du LFO. Le son ne change pas.
- Mesures au banc : accord do-sol-do, vélocité 100, 4 s, tempo 120. Les anciens projets ne changent pas (leur volume et leurs réglages sont sauvés dans le projet).

## v6 (module TAPE, calé sur un Philips D6410)
- **Module TAPE** à la place de CASSETTE / VHS (même emplacement du rack, pas de module en plus) : un magnétophone à cassette calé sur un Philips D6410 mesuré au banc (fichier témoin enregistré sur cassette type I puis réenregistré). Code commun avec Orage dans Foudre-Core (`FoudreTape.h`).
  - DRIVE : saturation de bande suréchantillonnée ×4 (harmonique 3 surtout) ; 50 % = passe propre du Philips (H3 -24 dB, 6 à 9 % de distorsion), 100 % = passe saturée (12 à 17 %).
  - ALC : compression automatique d'enregistrement du Philips (attaque 60 ms, relâche 0,7 s, jusqu'à 5:1 : 19 dB d'écart à l'entrée → 4 dB à la sortie).
  - COLOR : courbe mesurée (bruit rose) : -10 dB à 160 Hz, -24 dB à 63 Hz, +10 dB vers 8 kHz, coupure au-delà de 12 kHz (écart au Philips : 1,3 dB rms de 40 Hz à 20 kHz).
  - WOW / FLUTTER : raies mesurées (1,4 Hz ; 6,3 - 7,2 - 9,4 Hz ; 72 Hz) + dérive aléatoire ; 50 % = Philips (0,10 % / 0,21 % rms), jusqu'à 8× / 3× au-delà.
  - HISS : souffle à la couleur du Philips, 100 % = -40,7 dBFS ; loi au carré (50 % = -52,7 dBFS). HUM : 50 Hz et harmoniques, 10 % = Philips.
  - MONO (le Philips est mono), MIX. Tout à zéro : sortie = entrée, retardée de 36 échantillons (0,75 ms, le son direct de MIX attend autant : pas d'effet de peigne).
  - Vérifié : le fichier témoin passé dans TAPE réglé « Philips » donne les mêmes mesures que la vraie cassette (distorsion, courbe, pleurage, souffle, ronflement).
- **Anciens projets** : le module reste l'ancien CASSETTE (même son), titre « CASSETTE / VHS (old project) » et bouton **→ TAPE** pour passer au nouveau moteur.
- **Presets** : les presets cassette passent sur TAPE (volumes recalés sur ceux d'avant) ; **6 presets façon Boards of Canada** : Kodachrome Fade (Pad), Summer Camp 1979 (Lead), Pine Forest Signal (Arp), Overwound Spool (Bass), Film Society (Chord Seq : Fm7, D♭maj7, A♭, E♭), Rewind Memory (Rack FX, Philips complet).

## v5 (Chord Seq, Arp/Seq rangé, filtres au même volume)
- **Chord Seq** : en SEQ, les notes jouées ensemble (moins de 60 ms d'écart) vont dans le même pas, jusqu'à 6 notes ; le pas affiche le nom de l'accord (Fm7, D♭maj7, C/E…) et sa basse. Bouton CHORD / 1 NOTE ; molette = transpose l'accord. Le nom des accords et le regroupement des notes viennent de Foudre-Core (`FoudreAccords.h`, commun avec Orage).
- **Arp/Seq rangé** (doublons retirés) : un seul choix OFF / ARP / SEQ (avant : OFF-ON-LATCH puis ARP-SEQ), aussi sur le mini-module ARP de la page SYNTH ; LATCH fusionné avec HOLD ; TRIPLET retiré (les triolets sont dans DIVISION) ; FREE renommé RUN ; « Free » de DIVISION renommé HZ (free) et le potard TEMPO renommé HZ, grisé quand l'horloge suit le tempo ; potard RATCHET global remplacé par ALL RATCHET sur la grille ; réglages inutiles grisés selon le mode (MODE / OCTAVE / SHIFT / CLUSTER en SEQ, REC / REST / TIE / BACK / CHORD en ARP).
- **Anciens projets convertis à l'identique** : LATCH → ARP + HOLD ; TRIPLET + 1/4, 1/8, 1/16 → 1/4T, 1/8T, 1/16T ; TRIPLET en horloge libre → HZ ×1,5 ; RATCHET global → chaque pas de la grille. Mesure : projet converti et non converti identiques (0,0 dB). Le CC 109 de l'Hydrasynth (potard RATCHET de l'ARP) pilote toujours le ratchet global, invisible : il s'ajoute à la grille comme avant.
- **Filtres au même volume** (en gardant leur caractère) : à cutoff et résonance égaux, MS-10, LADDER, SEM et PROPHET sont à ±0,1 dB l'un de l'autre sur la note de mesure, en général moins de 1,6 dB sur une note deux octaves plus haut (avant : jusqu'à 9 dB d'écart ; le PROPHET était 2,5 dB plus bas même sans résonance). Pas de correction filtre fermé sous la note. Les **anciens projets gardent l'ancien volume** ; les **presets d'usine** sont recalés à leur volume d'avant (51 presets, ±0,3 dB) ; le son Init baisse de 1,9 dB.
- Tests : banc d'essai Foudre-Core (enregistrement d'accords, conversion d'anciens projets, niveaux des filtres et des presets, captures de l'interface).

## v4 (cassette, arpege en accords, vitesse par pas — repris d'Orage v23)
- **Module CASSETTE / VHS** dans le rack (11e module, Z-11 TAPE, ecran a bobines) : WOW (pleurage lent), FLUTTER (scintillement rapide), SATURATE (saturation de bande, niveau compense), AGE (bande usee : aigus et graves qui s'en vont), HISS (souffle), CRUNCH (pertes et micro-coupures), MIX. Tout a zero : sortie = entree exactement (le module ne lit meme pas sa ligne a retard). Absent du rack des anciens projets.
- **Arpegiateur : mode CHORD** (7e cran du potard MODE) : chaque pas rejoue l'accord tenu entier ; OCTAVE monte d'une octave a chaque tour ; ratchet, chance, gate et swing s'appliquent a l'accord.
- **Vitesse par pas** : au-dessus de la grille NOTE STEPS, RATCHET / STEP SPEED. En STEP SPEED, la rangee du bas regle la vitesse de chaque pas : x1, x2, x4, x1/2, x3/2 (clic = suivante, molette, clic droit = x1). Fonctionne en ARP et en SEQ, avec le swing, et reste cale sur le DAW.
- **Potards crantes** : les libelles des crans ne sont plus coupes (MODE, OCTAVE, RATCHET...).
- **9 presets cassette** : Faded Polaroid (Pad), Worn Solina (Strings), Filmstrip Keys (Keys / Bells), Lullaby Sine (Brass / Lead), VHS Bass (Bass), Chord Pulse 82 et Half-Speed Memory (Arp / Seq), Rainy Projector (Texture), Cassette Memories (Rack FX, aussi dans Zephyr FX).
- Projets et patchs des versions precedentes relus a l'identique. Test : `tools/tests/v4_cassette_arp`.

## v3c+ (profil de contrôleur Hydrasynth / Osmose CE)
- **SETTINGS → CONTROLLER** : *Hydrasynth* (table MIDI d'origine) ou *Osmose CE*. Le profil Osmose active le MPE (canal 1 maître), règle le bend par note sur ±48, vide la table MIDI (l'Osmose n'a pas de potards) et passe la reprise en DIRECT. Le slider de modulation (CC1) pilote la molette ; pression par note, CC74 et bend par touche passent par le MPE. Pédales : clic droit sur un potard > Assign MIDI. Choix sauvé avec les réglages globaux ; « RESTORE HYDRASYNTH MAP » revient au profil Hydrasynth.

## v3c (audit enveloppes et effets)
- **Plus de clic quand une voix est reutilisee** (vol de voix a la 13e note, MONO qui rejoue pendant le release) : comme sur un vrai synthe, la voix garde son etat (enveloppes qui repartent de leur niveau, filtres et oscillateurs qui continuent). Mesure : saut de +14 dB (vol) et +36 dB (MONO) -> au niveau du jeu normal.
- **Lignes a retard en interpolation cubique** (ensemble, delay, reverb, pitch-shift du shimmer) : l'ensemble perd son grain dans l'aigu (-27 -> -35 dB), la reverb est plus propre (-52 -> -58 dB) ; couleur de la reverb conservee (amortissement et filtre de sortie recales : duree de queue et spectre a moins d'1 dB / 5 % d'avant).
- **Saturation surechantillonnee avec filtres elliptiques** (-95 dB) : repliement -74 / -89 dB -> -123 dB.
- Verifies sans defaut : courbes et plages des enveloppes (1 ms a 5 s, attaque en charge de condensateur), fin de note, crepitement des reglages (aucun), denormaux (le plugin est protege), bruit residuel des effets.
- Tests : `tools/tests/continuite_voix`, `tools/tests/audit_effets_zephyr`.

## v3c (VCO)
- **VCO surechantillonnes** (x2, x4 en High Fidelity) : dent de scie, carre, FM, retour, poly-mod et sync restent propres jusque dans l'aigu (pire raie parasite audible : -47 -> -66 dB en dent de scie au Sol 7 ; FM lineaire -15 -> -53 dB). Cout processeur : +1 a +4 %.
- **Hard sync anti-repliement** : correction a l'instant exact du recalage (et le VCO 2 n'avance plus d'un echantillon de trop apres le recalage) : -24 -> -61 dB dans l'aigu, toutes formes d'onde.
- **SETTINGS > VCO PHASE** : *Free running* (defaut, comme un vrai VCO : chaque note demarre ailleurs dans le cycle, le battement de deux VCO desaccordes n'est jamais le meme) ou *Reset* (attaque identique a chaque note).
- **SETTINGS > VCO MODEL** : *Analog* (defaut : rampe legerement bombee, sommets du triangle arrondis, liaison capacitive ~5 Hz qui fait s'affaisser le carre dans le grave, petites differences de forme / largeur / niveau entre les voix) ou *Clean digital*.
- Test : `tools/tests/vco_phase_sync`.

## v3b (corrections)
- **FM** : avec un rapport FM (1:2, 2:1, 3:1...), OCTAVE, TUNE et FINE du VCO 2 se reglent toujours, par-dessus le rapport (avant, ils n'avaient plus d'effet). CHARACTER et DICE remettent le VCO 2 au neutre pour que leur rapport s'entende tel quel.
- **Sequenceur sans arpegiateur** : TYPE = SEQ suffit, plus besoin d'allumer l'ARP (les touches transposent le motif ; LATCH reste disponible).
- **Sequenceur en tempo** : changer de note ne relance plus l'horloge ; elle continue meme entre deux notes. Quand le DAW joue (cadence synchronisee), les pas sont cales sur sa mesure. Une touche a peine en retard (< 30 ms) fait quand meme partir le pas en cours ; apres une longue pause (un motif entier), le motif repart du debut.
- Anciens projets : memes hauteurs et meme jeu (VCO 2 remis au neutre s'il etait desaccorde sans effet ; SEQ choisi avec ARP eteint -> type ARP).
- **LINK des enveloppes** (bouton sur le panneau AMP) : l'enveloppe FILTRE pilote aussi le volume ; l'enveloppe AMP devient **ENV 2 libre** (ses ADSR restent reglables) : source ENV AMP de la matrice, ou raccourci DEST (VCO 2 pitch, VCO 1+2 pitch, FM, PWM, wavefold, niveau VCO 2, ring) + AMOUNT (+-1 = +-1 octave pour la hauteur). LINK eteint : rien ne change.
- Tests : `tools/tests/seq_tempo_fm`, `tools/tests/env_link`.

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

## v2d (MPE : plus de tremblement des filtres en polyphonie)
- **Cause** : l'Hydrasynth en mode **MPE** joue chaque note sur son propre canal MIDI, avec sa pression (aftertouch) et son CC74 sur ce canal. Zephyr traitait la pression de canal comme une valeur unique pour tout le synthé : avec plusieurs notes, elle sautait sans cesse de la pression d'une touche à celle d'une autre, et le filtre de **toutes** les voix tremblait. Une seule note : pas de saut, pas de grésillement ; le réglage SETTINGS > aftertouch n'y changeait rien. Le moteur des filtres lui-même était sain (testé avec aftertouch polyphonique lent, rapide, tremblant, dense).
- **Correction** : détection automatique du MPE (notes tenues sur plusieurs canaux, ou canal qui change à chaque note). Alors :
  - chaque note suit **sa propre pression** (vrai aftertouch polyphonique) ; la source AFTERTOUCH globale prend la plus forte pression, stable ;
  - le **pitch bend** d'un canal de note ne touche que sa note (étendue MPE ±48 demi-tons) ; celui du canal 1 reste global ;
  - les **CC des canaux de notes** (CC74 « timbre » par note) ne pilotent plus les potards : seul le canal maître 1 le fait (sustain accepté partout).
  - Avec un clavier classique (un seul canal) rien ne change.
- Les contrôles continus (pression, bend, molette) ne découpent plus le rendu à l'échantillon près (au plus tous les 32 échantillons) : moins de charge avec un flux MIDI dense.
- Test : `tools/tests/midi_hydrasynth.cpp` (MPE : chaque note suit sa pression, CC74 sans effet sur le cutoff) ; jeu MPE identique à l'échantillon près au même jeu en aftertouch polyphonique.

## v2c (section FX réorganisée, écrans animés)
- **Patchbay retiré.** Zephyr (synthé) : la matrice du synthé est l'unique système de modulation des effets (anneaux visibles sur les potards). Zephyr FX : chaque source (LFO 1, LFO 2, S&H, suiveur d'enveloppe) a un menu de destination ; destinations : saturation, wide, phaser, ensemble, filtre, **delay time / feedback / mix**, **reverb mix / decay**, dry/wet. Les câbles de modulation des anciens projets Zephyr FX sont convertis ; les câbles audio ne sont plus repris (chaîne normale du rack).
- **SERIAL / PARALLEL** (sur le delay et la reverb) : en parallèle, la reverb reçoit le même signal que le delay et les deux s'additionnent (la reverb ne traite pas les échos).
- **Rack pleine largeur**, modules moins hauts (5 visibles), un seul ordre partout (spectre : saturation → wide), tuiles colorées.
- **Presets du synthé** : le rack ne garde que les modules utilisés (réglage non neutre ou cible d'une macro / de la matrice).
- **Colonne de gauche** : OUTPUT seul dans le synthé (DRY/WET et ON restent dans Zephyr FX), CLEAR RACK, **RESET FX** (ne touche plus au synthé). Textes en anglais.
- **Bypass = vrai A/B** (réglages conservés) ; un module ajouté depuis sa tuile démarre à zéro.
- **Écrans animés** : chaque module affiche son état réel dans sa couleur — courbes d'EQ et de filtre, sinus saturé, transfert du compresseur avec point de fonctionnement et réduction de gain, creux du phaser qui glissent, lignes de l'ensemble, tranches du stutter au tempo, salves d'échos du delay, ondes de la reverb (figées en FREEZE), flèches du wide. 30 images/s, uniquement sur la page EFFECTS et pour les modules actifs.

## v2b (polyphonie propre, modules FX personnalisés)
- **Accords qui « s'entrechoquent » : corrigé.** Mesure : rendues une par une puis additionnées, les voix donnent exactement l'accord (écart −55 à −70 dB) ; les filtres sont donc indépendants. La cause était la sortie du synthé : la somme des 12 voix passait dans une saturation instantanée (tanh), et un accord dense (8 notes, résonance) l'attaquait jusqu'à 2,5 fois sa pleine échelle, ce qui faisait intermoduler les notes entre elles.
  Correction : 6 dB de réserve sur le bus des voix + un limiteur à gain lissé (attaque 0,8 ms, relâchement 180 ms) au lieu de la tanh, écrêtage doux seulement au-delà. Sur un accord de 8 notes, la part hors harmoniques passe de −20/−35 dB à −38/−63 dB, le niveau des filtres seuls (MS-10 et SEM : −56/−64 dB contre −20/−25 avant).
- Niveaux : les presets d'usine (et une nouvelle instance) remontent la sortie de 4 dB ; écart moyen avec la v1 −1,6 dB, crête maximale −7,5 dBFS. Les projets déjà sauvegardés en v2 sonnent 6 dB plus bas : remonter OUTPUT.
- **Modules FX personnalisés** : chaque module prend une couleur du spectre du prisme (du rouge de la saturation au violet de la reverb, magenta pour le stereo wide), une pastille de verre allumée quand il est actif, un filet lumineux sous le titre, un grand pictogramme gravé dans la plaque (sinus saturé, courbe d'EQ, passe-bas résonant, peigne du phaser, coude du compresseur, trois lignes de l'ensemble, tranches du stutter, échos qui s'éteignent, ondes de la reverb, flèche stéréo) et une référence gravée (Z-1 SAT … Z-10 WIDE). Les tuiles MODULES portent la même couleur.

## v2 (reconstruit sur le moteur d'Orage v20b)
- **Base** : moteur, interface et MIDI d'Orage v20b ; modules d'effets, thèmes, presets et arpégiateur Mono/Poly de Zephyr v1.
- **Filtre Prophet** : modèle à 4 pôles OTA (résolution trapézoïdale + Newton amorti, comme le CEM3320), retour de résonance écrêté. Grille de stabilité (`tools/tests/stabilite_filtres.cpp`, 1632 réglages par mode de qualité sur CEM3320, SEM et Prophet) : MS-10, CEM3320 et SEM sans parasite ; Prophet : 1 cas (accord de 5 notes, résonance 90 %), de l'intermodulation attendue à ce niveau de résonance.
- **Fuzz** : 120 réglages tirés au hasard (44,1 et 96 kHz, 3 qualités, 4 filtres, poly-mod, drift, sub) : aucune valeur invalide ni explosion.
- **CPU** (12 voix tenues + rack complet, un cœur du serveur de build, sans optimisation spécifique) : Performance 21–38 %, Quality 22–56 %, High Fidelity 35–98 % (CEM3320 et Prophet les plus coûteux). *High Fidelity avec 12 voix est réservé au rendu ou aux sons à peu de notes.*
- **Presets** : les 43 presets de Zephyr v1 sont conservés (filtres v1 → circuits : SEM → SEM, LADDER → CEM3320, PROPHET → Prophet). Niveau mesuré contre la v1 (accord tenu) : 29 presets à ±1,5 dB sans retouche, 14 recalés par un trim de sortie (de −3,6 à +2,4 dB). Timbres non écoutés : à juger à l'oreille.
- **Projets Zephyr v1** : filtres et destinations de matrice (SUB, POLY-MOD, DRIFT) migrés à l'ouverture, vitesse des LFO conservée, patch du rack lu (le stutter n'existait pas).
- **Test MIDI** (`tools/tests/midi_hydrasynth.cpp`) : collisions de CC, CC74, Program Change, sustain, bend, LFO du filtre, CC des effets, macros 16-23, rack de départ, MONO arpège l'accord, POLY tourne sur 4 VCO, l'arpège continue après un changement de mode, aucune note bloquée, migration v1 : **TOUT OK**.
