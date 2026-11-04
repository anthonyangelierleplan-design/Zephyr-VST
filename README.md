# Zephyr - Foudre Audio

Synthe experimental de feedback : larsens, tuyaux qui sifflent, filtres qui s'auto-oscillent.
Chaque voix est une boucle (ligne a retard + filtre passe-bande + saturation) accordee sur la note.
La pression (aftertouch polyphonique) pousse le gain de la boucle et/ou le "vent" : la note monte par paliers sur les harmoniques
(arpege naturel) ou sur les degres d'une gamme, avec hysteresis.

- RESONATOR : PIPE (ligne a retard) ou FILTER (filtre qui s'auto-oscille).
- LOCK : HARMONIC (serie harmonique de la note) ou SCALE (gamme + tonique au choix).
- 3 assignations de la pression (FEEDBACK, WIND, FOCUS, BRIGHT, DRIVE, BREATH, PITCH, EXT INPUT), chacune avec son dosage +/-.
- Pression par note, pression de canal, mode MPE, molette de modulation (CC1) : tout est pris en compte. Temoin MIDI dans l'interface.
- HOLD (drone) et pedale de sustain. Entree audio via le sidechain de l'hote (AUDIO IN) : le son est resonne par les boucles.
- Sortie toujours limitee sous 0 dBFS. Fenetre redimensionnable. Presets utilisateur : `%APPDATA%\Foudre Audio\Zephyr\Presets`.

Compilation : pousser sur GitHub, onglet Actions -> artefacts `Zephyr-VST3-Windows` et `Zephyr-Standalone-Windows`.
