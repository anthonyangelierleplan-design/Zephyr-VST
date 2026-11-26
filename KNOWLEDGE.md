# Foudre Audio — Base de connaissances plugins

Ce que la construction d'Orage et d'Orage FX a appris, pour que le prochain plugin évite les mêmes pièges.
À relire avant d'écrire un nouveau plugin ; dérouler les tests et la checklist avant chaque envoi.
Les chiffres viennent des tests automatiques, pas d'une écoute : l'oreille reste le dernier juge.

## Architecture type
- JUCE 8.0.4, C++17, build Windows par GitHub Actions, release `latest`.
- `Synth.h` : voix, oscillateurs, filtres, matrice. **Aucune dépendance JUCE** : se teste seul.
- `Engine.h` : rack d'effets, câblage, modulations externes (blocs de contrôle de 32 échantillons).
- `Arp.h` : arpégiateur et pistes de modulation.
- `PluginProcessor.cpp` : paramètres, presets, MIDI, état. Les tables de paramètres sont lues dans l'ordre : l'ordre doit rester identique partout.
- `PluginEditor.cpp`, `SynthEditor.cpp` : interface. Jamais de calcul audio ici.
- Une même source donne deux plugins via `ORAGE_SYNTH` (1 = synthé, 0 = effets seuls).
- Après modification d'un en-tête : recompiler TOUS les objets (un objet périmé = plantage `free(): invalid pointer`).

## Journal des problèmes (symptôme → cause → correction)
- v18d — Cri strident du Ladder CEM cutoff à fond, parasites en polyphonie (SEM CKT) → (1) g = tan(pi f/fs) monte à 6 près de Nyquist : Newton non amorti diverge et le trapèze sonne à Nyquist ; (2) SEM : diodes placées sur l'amortissement au lieu de la régénération → amortissement qui BAISSE à fort niveau → suréchantillonnage ×2 des filtres circuit (g < 1), Newton amorti (pas ≤ 1 Vs), SEM = amortissement fixe d0 − régénération écrêtée. ERREUR DE MÉTHODE : j'avais validé les filtres seulement à cutoff moyen (800 / 3000 Hz) et sur une note.
- v18c — Anneau de modulation qui ne fait pas le tour du potard à 100 % → le moteur applique 0,5 / 0,45 / 0,6 sur certaines destinations et l'anneau en tenait compte → la matrice divise par cette échelle (modNative dans Synth.h), l'anneau affiche 100 % = course complète.
- v18c — Potard à fond à l'allumage d'un module FX → valeurs de départ « legacy » non nulles pour tous les presets et aucun reset à l'activation → defauts à zéro, reset des potards du module quand on l'allume (bouton du module).
- v18 — Résonance MS-20 trop « générique » → filtre modélisé par réponse, pas par circuit → MS-10 écrit depuis le schéma (équations nodales, trapèze + Newton 1D réduit à la tension de diode), tables de cutoff/compensation issues de l'auto-oscillation SIMULÉE (pas de formule), calibré sur enregistrements réels.
- v17 — Aigus parasites wavefolder/drive sur notes hautes → repliement de spectre → suréchantillonnage ×4 (demi-bandes 23 et 11 taps), triangle polyBLAMP.
- v17 — Composante continue en sortie → saturation asymétrique → coupe-continu 6 Hz en sortie du synthé.
- v16 — Escalier sur le cutoff → réglages lus par blocs sans lissage, CC MIDI à 128 pas → 2 lisseurs en cascade (5,5 ms chacun) + coefficients interpolés par échantillon.
- v16 — Aigus audibles filtre fermé avec résonance (MS-20) → bande passante ajoutée à la sortie non filtrée → refiltrée à 2× le cutoff (+42 dB → +6 dB).
- v16 — NaN sur ~1 réglage extrême sur 100 → FM forte sur note très aiguë, phase hors [0,1[ → incréments bornés à 0,49, phase repliée avec floor, garde-fou par voix.
- v16 — Source assignée sans effet la première fois → routage existant à quantité zéro ou LFO très lent → réassigner donne 50 %, point vivant sur l'anneau.
- v15b — Anneaux coupés, chiffres « 82 » collés → dessin hors zone du potard → anneaux entre corps et graduations, chiffres rapprochés.
- v15 — Latence au contrôleur MIDI → réglages du synthé lus avant le MIDI → CC appliqués en début de bloc, une fois par bloc.
- v15 — Drive qui assombrit → saturation large bande → accentuation > 1,2 kHz avant saturation.
- v14c — Molette non assignable en MIDI learn → CC1 exclu → accepté.
- v14 — Molette sans effet → aucun routage par défaut → molette → LPF (slot 13) + moniteur MIDI.
- v13 — Macros sur de mauvais CC → numéros sauvegardés dans les presets → déplacés dans le projet, défaut CC 16–23.
- v13 — Tuiles du rack mal alignées → deux listes de modules → une liste partagée.

## Règles de qualité audio
1. Lisser tout réglage continu (2 étages, ~20 ms au total).
2. Interpoler les coefficients à chaque échantillon.
3. Suréchantillonner toute saturation forte (×4).
4. Band-limiter les oscillateurs (polyBLEP, polyBLAMP).
5. Borner ce qui peut exploser (phase, résonance, sortie en tanh).
6. Garde-fou par voix : valeur non finie → états remis à zéro.
7. Un filtre fermé doit être vraiment fermé : tout signal ajouté après le filtre est lui-même filtré.
8. Coupe-continu en sortie dès qu'une saturation est asymétrique.
9. Lire le MIDI avant les réglages du bloc.
10. Démarrer neutre (volume prudent, effets à zéro).

## MIDI, presets, compatibilité
- Ne jamais réordonner une liste enregistrée (sources, destinations, modules) : ajouter à la fin.
- Paramètres identifiés par leur nom ; un paramètre absent prend sa valeur par défaut.
- Garder le chargement des anciens formats.
- Assignations MIDI dans le projet et la configuration globale, pas dans les presets.
- Macros CC 16–23, molette CC1, apprentissage MIDI par clic droit, modes SCALE / PICKUP / DIRECT.
- Un moniteur MIDI visible dans l'en-tête.

## Interface
- Rien ne dépasse de la zone d'un potard ; vérifier les voisins sur capture agrandie.
- Menus longs en sous-menus par module.
- Modulation : clic droit « Assigner à », 2 sources max, un anneau par source, Ctrl + clic gauche + glisser.
- Montrer une capture avant d'envoyer ; attendre l'accord avant un nouveau design.
- Non testable sur le banc : menus ouverts, souris réelle.

## Batterie de tests (résultats Orage v17)
- Réglages aléatoires extrêmes : 0 NaN sur 250, pic max 0,84.
- 65 presets d'usine : pic < 1, continu < 0,02 ; 5 signalés pour souffle ou larsen voulu.
- Filtre fermé + résonance : +6 dB d'aigus au plus.
- Repliement : drive −38 dB (−22 avant), wavefolder à fond −20 dB (−11 avant).
- CPU : 8 % d'un cœur, 6 notes, rack complet, compilation optimisée.
- Apprentissage MIDI, état sauvegardé, captures agrandies.
- Méthode : un test qui ne distingue pas avant/après mesure la mauvaise chose ; pour un NaN, remettre un réglage à zéro à la fois.

## Checklist avant de pousser
1. Compilation synthé sans erreur, syntaxe de la version effets.
2. Tous les objets recompilés.
3. Batterie de tests.
4. Listes enregistrées intactes.
5. Capture agrandie regardée.
6. README (état actuel) et CHANGELOG (nouvelle version, en haut) à jour ; `bash tools/tests/run.sh` sans échec.
7. Accord donné.
8. Push sur les deux dépôts, deux builds verts.
9. Compte rendu : mesuré, non vérifié, à juger à l'oreille.

## Méthode : modéliser un filtre d'après son schéma
1. Schéma → inconnues (nœuds) et équations de courant ; OTA : I = Gp·Vs·tanh(v/Vs) ; condensateurs en trapèze (h = 2·G·v − h_ancien).
2. Réduire à une inconnue (ici la tension de diode) → Newton 1D, départ extrapolé, ≤ 10 itérations, garde-fou non fini → reset.
3. Cutoff : table construite à partir de la fréquence d'auto-oscillation simulée (inclut la pré-déformation bilinéaire), jamais d'une formule théorique.
4. Valider sur enregistrements : réponse harmonique relative au filtre ouvert, ajustement du cutoff par trame, erreur RMS ancien/nouveau.
5. Mesurer CPU en vrai plugin, fuzz de réglages extrêmes, presets.
6. OBLIGATOIRE avant de pousser un filtre : grille de stabilité (tools/tests/stabilite_filtres.cpp) : notes 48 à 96 et accords, cutoff 0,5 à 1,0, résonance jusqu'à 0,9, drive, enveloppe, à 44,1 et 48 kHz ; critère : énergie hors harmoniques < -20 dB et comparaison aux anciens filtres. Un filtre non linéaire implicite se teste TOUJOURS cutoff à fond.

## Méthode : générer et calibrer des presets
Table de données (tools/presets/presets.py) → code C++ → banc de test qui joue chaque preset → niveau de sortie calculé par catégorie (RMS pour les sons tenus, crête pour les plucks/arpèges) → 2 passes. Toujours garder une table séparée des fonctions : refaire les presets coûte alors une heure, pas une journée.

## Reste à faire sur Orage
- Écouter les 74 presets v18 (niveaux mesurés, timbres non écoutés) ; presets Rack FX gardent encore les anciens réglages de départ.
- Ladder CEM3320 et SEM circuit faits d'après les schémas seulement (v18) ; les caler sur des enregistrements du matériel quand le user en a (MS-10 à 80 % / 90 % aussi). Compensation de hauteur de l'auto-oscillation CEM (+7,5 % sur g) ajoutée à la main : l'OTA saturé fait baisser la fréquence.
- Écouter le MS-10 circuit ; décider du roll-off aigu et d'un réglage d'asymétrie.
- À l'oreille : lissage 20 ms, MS-20 adouci, drive brillant, wavefolder.
- Dans Ableton : menu « Assigner à », anneaux à la souris, sous-menus.
- Wavefolder à fond sur notes très aiguës : reste −20 dB (×8 possible).
- Rack FX : saturations non suréchantillonnées, pas d'audit à l'écoute module par module.
- Clics possibles sur changements à positions fixes pendant une note.

## Adapter un synthé à un contrôleur MIDI (exemple Hydrasynth)
- Une table `id de paramètre → CC` par défaut (une fonction `defaultCcOf`) ; ne sauvegarder que les écarts par rapport à elle, ainsi les anciens projets ne cassent pas et on peut changer la carte plus tard.
- Tester l'absence de collision (deux paramètres sur un même CC) dans un test automatique.
- Program Change : ne jamais charger un preset depuis le thread audio ; poser un atomic + `AsyncUpdater`, et un compteur que l'éditeur surveille pour se rafraîchir.
- Pédale de sustain : retenir les note-off dans le processeur (indépendant du moteur de voix).
- Ajouter un paramètre = toujours à la FIN de la liste des paramètres (et de sa lecture), pour rester compatible avec les états sauvegardés.

## Zephyr v2 : porter un plugin "frere" sur un nouveau moteur
- Methode : fusion a trois voies des editeurs (`git merge-file` : base = Orage au moment ou Zephyr v1 en est parti, "ours" = Zephyr v1, "theirs" = Orage v20b, noms de classes alignes par sed avant la fusion). Les conflits restants sont presque tous de l'identite visuelle (noms des modules, peinture des plaques, logo) : les resoudre a la main en gardant les fonctions nouvelles (bypass, MIDI, SETTINGS) dans le dessin de Zephyr.
- Moteur d'effets : garder l'infrastructure la plus recente (bypass, lissage 2 etages, 38 entrees de modulation) et y remettre les modules du plugin ; garder la meme numerotation des entrees de modulation que le synthe (alias de noms `MdPhMix = MdFxUnc`...) pour ne pas toucher au moteur de voix.
- Parametres : ceux de la v1 gardent leur position (automation des projets), les nouveaux vont a la fin. Etat : un drapeau (`zv2`) declenche la migration des anciens projets (types de filtre, numeros de destinations).
- Niveaux : mesurer chaque preset (accord tenu, RMS et crete) dans l'ancienne et la nouvelle version avec le meme harnais ; corriger par un trim de sortie par preset plutot que par un gain global (les ecarts viennent surtout du type de filtre et de la resonance).
- Piege : en comparant deux tableaux de mesures, verifier les colonnes (une premiere comparaison crete/RMS a fait croire a +12 dB).

## Filtre Prophet (SSM2040, Zephyr v2)
- 4 poles OTA (tanh par etage), resolution trapezoidale + Newton amorti comme le CEM3320 ; retour de resonance ecrete (tanh(0,55 x) / 0,55) avec sa derivee dans le jacobien.
- Niveau d'attaque des cellules 1,0 (a 1,6, les accords a forte resonance intermodulent fort) ; compensation des graves 1 + 0,12 k ; sortie x (0,75 + 0,4 res) pour rester au niveau des autres filtres.
- Reste a faire : caler sur un enregistrement de Prophet-5 rev 1-2 (frequence d'auto-oscillation, kProTune = 0,075 copie du CEM).

## Reste a faire sur Zephyr v2
- Ecouter les 43 presets (niveaux mesures, timbres non ecoutes), en particulier ceux qui etaient sur le Ladder v1 (passes au CEM3320) et le Prophet.
- CPU High Fidelity avec 12 voix : jusqu'a ~1 coeur du serveur de build sur CEM3320 / Prophet. Pistes : ne pas suréchantillonner les voix en release, ou High Fidelity limite a 8 voix.

## MPE (Zephyr v2d)
- Symptome : « en polyphonie, le filtre tremble sur toutes les notes avec l'aftertouch », une note seule OK, le lissage de l'aftertouch n'y change rien -> penser MPE : chaque note sur son canal, pression de canal PAR NOTE, CC74 par note, bend par note. Traiter la pression de canal comme globale la fait sauter d'une note a l'autre.
- Correction : retenir le canal de chaque note ; pression d'un canal -> aftertouch de SA note ; bend d'un canal de note -> sa note (±48) ; CC des canaux de notes ignores pour les potards (canal maitre 1 seulement). Detection automatique (notes tenues sur plusieurs canaux / canal qui tourne), y compris sur les notes du bloc courant (les CC74 arrivent avec la note).
- Piege de test : getSampleRate() vaut 0 hors hote (prepareToPlay appele a la main) -> toute duree calculee avec elle vaut 0.

## Zephyr v3 : reprendre Orage v22c
- Meme methode que la v2 : fusion a trois voies (base = Orage v20b 5ff41fa, ours = Zephyr v2d, theirs = Orage v22c, renommages sed avant). Pour l'editeur du synthe (entierement refait dans Orage), partir du fichier d'Orage et y remettre les specificites Zephyr (Prophet, MIXER & DRIFT, unite POLY-MOD, SHIFT / CLUSTER de l'ARP, familles de destinations) plutot que fusionner.
- A retirer apres fusion : `pruneUnusedModules` d'Orage (parametres FX d'Orage) -> Zephyr a `pruneRack` ; champs de theme absents (`metalHi/Lo` -> `capHi/Lo`) ; largeur figee 1440 de l'editeur d'effets seul -> largeur de conception + flancs.
- Piege : un booleen de membre par defaut (`patchOpen = true`) reactive le patchbay dans la version FX seule si son forcage est sous `#if ZEPHYR_SYNTH` : toujours tester les DEUX cibles (rendu de Zephyr FX).
- Piege de rendu : le harnais lit le theme dans un fichier partage -> ne pas lancer plusieurs rendus de themes differents en parallele.
- Evenements MPE dans le processeur : 11 = bend par note, 16 = slide (le 12 d'Orage est pris par l'arp de Zephyr).

## Caler un filtre sur des enregistrements (Prophet-6, Zephyr v3)
- Materiel ideal : une note fixe (dent de scie), balayage lent du cutoff aller-retour, une prise par position de resonance (0/25/50/75 %, plein). Les niveaux d'enregistrement peuvent differer : ne comparer que des rapports DANS une prise.
- Mesure : amplitude de chaque harmonique image par image (f0 suivie image par image : la derive du vrai synthe fait ±1 %) ; reponse = harmoniques / spectre "filtre ouvert" de la prise 0 % ; hauteur du pic = harmonique la plus forte - fondamentale. Rendre le plugin avec la meme note et la meme analyse (l'echantillonnage par harmoniques sous-estime les pics etroits, mais pareil des deux cotes).
- Resultats Prophet-6 LPF : pente identique au modele 4 poles a ±0,5 dB ; pic +0 dB a 25 %, +2..3 dB a 50 %, +16..27 dB a 75 % (monte avec le cutoff), auto-oscillation sinus tres pure (H2 -45 dB, H3 -49 dB) a niveau constant de 200 Hz a 12 kHz.
- Correction : table de 17 points (PCHIP) part du retour maximal = f(potard) [0, .11, .265, .88, 1 a 0/25/50/75/100 %] + perte de retour dans l'aigu (-9 % a 13 kHz, nulle a pleine resonance). Presets et anciens projets : RES converti par la table inverse (drapeau d'etat `proP6`) pour garder le son.
- Piege : la prise "auto oscillation" ne contenait plus du tout la dent de scie (fondamentale au bruit de fond) -> elle ne sert qu'a mesurer l'oscillation elle-meme.

## v3b : FM, sequenceur
- Rapport FM : la hauteur du VCO 2 = VCO 1 x rapport ; ajouter 12*oct2 + tune2 + fine2 par-dessus, sinon ces potards sont morts.
- SEQ : dans Arp, ne jamais remettre phase/stepCount a zero sur noteOn en mode SEQ ; faire tourner l'horloge a vide quand aucune touche n'est tenue ; caler sur le DAW au debut du bloc (position en pas = ppq x pas par temps) en comparant les positions "sans swing" (pas pair long de 1+sw, impair de 1-sw), recaler seulement si l'ecart > 0,02 pas.
- Processeur : TYPE SEQ force ap.on = 1 (sauf enregistrement pas a pas).
- LINK des enveloppes : le VCA doit rester au rythme audio -> quand LINK est allume, l'enveloppe audio `aenv` est simplement cadencee avec les reglages FILTRE (pas de marches d'un bloc), et une 3e enveloppe `e2` (reglages AMP, cadencee au bloc) sert de source ENV AMP / raccourci. La fin de voix reste `aenv` (donc la release FILTRE). Parametres ajoutes en fin de liste : s_envLink, s_env2Dst, s_env2Amt.

## v3c : VCO
- Mesurer le repliement : FFT fenetre Blackman-Harris (Blackman simple : ses lobes a -58 dB autour de la fondamentale faussent la mesure), f0 exacte par passages a zero, pire raie hors harmoniques sous 16 kHz.
- Hard sync : instant du recalage = (depassement de phase du VCO 1) / increment ; saut J = forme(0) - forme(phase du VCO 2 a cet instant) ; polyBLEP : echantillon precedent += J*x^2/2, echantillon courant += J*(-(1-x)^2/2) moins ce qu'osc() corrige deja (saut -2 dent de scie, +2 carre) ; sortie des VCO retardee d'un echantillon. Bug corrige : apres recalage, ph2 doit valoir x*inc2 (on retire inc2 avant l'increment).
- Surechantillonnage : boucle de facOsc sous-echantillons autour de toute la section VCO, puis decimation demi-bande (23 puis 15 coefficients). Appeler les deux down() du x4 dans des instructions separees (ordre d'evaluation des arguments non garanti en C++ -> son faux en High Fidelity). Bruit x sqrt(facOsc) pour garder son niveau.
- Phase libre : phase aleatoire a chaque nouvelle voix (Zephyr ne le faisait que si DRIFT > 0).

## v3c : audit enveloppes et effets
- Voix reutilisee (vol, MONO pendant le release) : ne pas faire `*v = Voice()` si la voix sonne encore ; ne remettre que l'expression par note -> plus de clic. Mesure : saut de la derivee seconde au moment de l'evenement.
- Banc d'effets : toujours activer FTZ/DAZ (`_mm_setcsr (... | 0x8040)`, comme juce::ScopedNoDenormals) sinon faux "ralentissements" ; sinus de test en double precision (en float, raies parasites a -68 dB qui viennent du generateur).
- Reverb (Dattorro) : la lecture lineaire des lignes assombrissait la queue en boucle ; en Hermite il faut dampHz x0,73 + passe-bas 8 kHz en sortie pour garder la couleur.
- Surechantillonneur x4 : elliptique normalise (ordre 9 a 2x, ordre 5 a 4x, -95 dB). Un ecretage tres dur sur fondamentale aigue replie quand meme (harmoniques > 100 kHz) : ADAA necessaire pour aller plus loin.

## v4 : port d'Orage v23 (cassette, CHORD, vitesse par pas)
- Le patch d'Orage ne s'applique pas tel quel (Zephyr a diverge) : port a la main ; `struct Tape` copiee telle quelle d'Orage.
- Nouveau noeud NTape insere avant NOut ; patch "z3:". Relecture : z2 = 12 noeuds, z1 = 11 noeuds ; le dernier noeud d'un ancien patch -> NOut, masque des sources `(1u<<NOut)-1u`. kMaxRack reste 11.
- A zero, la cassette ne lit pas sa ligne a retard (sinon dephasage avec le signal sec). Compensation de la saturation : 1 + 0,8 x SATURATE. pruneRack retire la cassette si MIX ~ 0 ou tous les reglages ~ 0.
- CHORD : le tableau des notes qui sonnent est remplace a chaque pas, toutes les notes du pas s'eteignent avant le suivant ; emises avec slot -1 (pas d'allocation VCO par pas comme le POLY de Zephyr). Branche placee avant le POLY `planned`.
- Vitesse par pas : increment de phase = base x vitesse / (1 +- swing) ; au changement de pas on convertit la phase restante. Calage DAW : position = somme des durees de pas sur un cycle (x2 si longueur impaire, a cause du swing) -> posOf / locate.
- Vitesse sauvee comme 5e champ ":" du pas (anciens projets : x1).
- Potards crantes (<= 8 crans) : la taille du potard tient compte du plus long libelle lateral ; libelles decales de la moitie de leur largeur.
- Presets cassette : niveaux mesures avec tools (accord tenu, RMS / crete) et cales sur leur categorie.
