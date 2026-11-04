# Zephyr - Foudre Audio

Synthe experimental de feedback : larsens, tuyaux qui sifflent, filtres qui s'auto-oscillent.
Chaque voix est une boucle (ligne a retard + filtre passe-bande + saturation) accordee sur la note.
La pression (aftertouch polyphonique) pousse le gain de la boucle et/ou le "vent" : la note monte par paliers sur les harmoniques
(arpege naturel) ou sur les degres d'une gamme, avec hysteresis.

- RESONATOR : PIPE (ligne a retard) ou FILTER (filtre qui s'auto-oscille).
- LOCK : HARMONIC (serie harmonique de la note) ou SCALE (gamme + tonique au choix).
- Matrice de modulation : 6 emplacements source -> cible -> dosage. Sources : aftertouch poly, aftertouch de canal, molette, expression, velocite, macros 1-8 (CC16-23), 2 CC en apprentissage MIDI (LEARN A/B). Cibles : FEEDBACK, WIND, FOCUS, BRIGHT, DRIVE, BREATH, PITCH, FM LEVEL, FM CHARACTER, DELAY MIX, DELAY FEEDBACK, REVERB MIX.
- Pression par note, pression de canal, mode MPE, molette de modulation (CC1) : tout est pris en compte. Temoin MIDI dans l'interface.
- HOLD (drone) et pedale de sustain. Petit synthe FM (Bell, Metal, Wood, Spring, Laser, Glass) qui excite le tuyau et/ou sonne a cote (ROUTE). Delay Normal / Spring / Psycho, routage Reverb>Delay, Delay>Reverb ou Parallele, renvoi dans la boucle (TO LOOP).
- Sortie toujours limitee sous 0 dBFS. Fenetre redimensionnable. Presets utilisateur : `%APPDATA%\Foudre Audio\Zephyr\Presets`.

Compilation : pousser sur GitHub, onglet Actions -> artefacts `Zephyr-VST3-Windows` et `Zephyr-Standalone-Windows`.


## Corps (BODY)
- PIPE, FILTER, BOW (corde frottee, aftertouch = pression d archet), REED (anche de clarinette, aftertouch = souffle).
- STRING (corde pincee Karplus-Strong : FOCUS = position du pincement, FEEDBACK = sustain),
- PLATE (plaque/cloche/bol, 8 modes : WIND morphe barre > cloche > plaque > bol),
- ROOM (larsen de piece : la piece est le tuyau, WIND ajoute une derive lente de la distance),
- CHOIR (choeur de voyelles : WIND = A E I O U, FOCUS = nettete des formants).
- LADDER (filtre en echelle auto-oscillant : prise de sortie passe-bas + saturation asymetrique chaude),
- MULTIMODE (filtre multimode : BRIGHT balaie passe-bas > passe-bande > passe-haut ; le souffle et le bruit passent selon le mode).
  Dans FILTER, LADDER et MULTIMODE la hauteur reste celle du filtre qui oscille ; seule la couleur change.

## Enveloppe ADSR
ATTACK / DECAY / SUSTAIN / RELEASE. Par defaut SUSTAIN = 10 (aucun decay audible). Pour un pluck : BREATH a 0, ATTACK court,
DECAY 150-300 ms, SUSTAIN 0. Voir les presets Short Pluck et Pipe Pluck.
