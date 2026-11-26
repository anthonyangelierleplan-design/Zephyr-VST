# Tests

## Automatiques (à chaque push, avant le build Windows)
`bash tools/tests/run.sh` compile et lance tous les tests qui n'utilisent pas JUCE. Si l'un échoue, le build et la Release ne partent pas.

- `audit_effets_zephyr` : chaque module d'effet : bruit résiduel, denormaux, coût processeur, queue qui s'éteint
- `continuite_voix` : pas de clic quand une voix est volée, rejouée en MONO ou s'éteint (rapport, ne bloque pas)
- `env_link` : LINK des enveloppes (ENV FILTRE -> volume, ENV 2 libre)
- `fm_lineaire` : FM linéaire / exponentielle stable, sans valeur invalide
- `stabilite_filtres` : grille de stabilité des filtres (notes, cutoff, résonance, drive, accords) : rapport, ne bloque pas
- `v4_cassette_arp` : module CASSETTE (transparent à zéro, réglages à fond, fuzz), arpège CHORD, vitesse par pas, anciens patchs
- `vco_phase_sync` : phase libre / recalée des VCO, hard sync sans repliement

## À la main (utilisent JUCE : processeur complet, MIDI, presets)
Ils se compilent avec le plugin (mêmes sources + modules JUCE) et se lancent avant chaque envoi qui touche au MIDI, aux presets ou à l'état sauvegardé.

- `midi_hydrasynth` : carte CC Hydrasynth, Program Change, sustain, bend, MPE, arpège POLY / MONO
- `midi_mpe` : MPE : pression, bend et CC74 par note
- `modes_matrice` : sens des routages de la matrice, conversion des anciens projets
- `prophet6_resonance` : loi de résonance du Prophet-6, conversion des presets et anciens projets
- `reprise_midi_chord` : reprise des potards (SCALE / PICKUP / DIRECT), CHORD LEARN
- `seq_tempo_fm` : rapports FM + OCTAVE/TUNE, séquenceur sans ARP et en tempo

`mesure_repliement.py` : mesure du repliement de spectre d'un rendu (numpy / scipy).

## Ajouter un test
Un fichier `.cpp` avec son `main()` qui renvoie 0 si tout va bien. Sans `#include <juce...>`, il est pris automatiquement par `run.sh`.
