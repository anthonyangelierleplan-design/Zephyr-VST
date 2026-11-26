#!/usr/bin/env bash
# Lance tous les tests qui ne dependent pas de JUCE (moteur, filtres, effets, arpegiateur).
# Utilise par le build GitHub (job "tests") : si un test echoue, la Release n'est pas publiee.
#   bash tools/tests/run.sh            (depuis la racine du depot)
# Les tests qui incluent JUCE (MIDI, presets, etat) se lancent a la main : voir LISEZMOI.md.
set -u
cd "$(dirname "$0")"
out="${TMPDIR:-/tmp}/foudre-tests"; mkdir -p "$out"
CXX="${CXX:-g++}"
pass=0; fail=0; failed=""
for f in *.cpp; do
    grep -q '#include <juce' "$f" && continue
    n="${f%.cpp}"
    if ! "$CXX" -O2 -std=c++17 -I../../Source "$f" -o "$out/$n" 2> "$out/$n.err"; then
        echo "ECHEC (compilation) $n"; sed -n 1,20p "$out/$n.err"; fail=$((fail+1)); failed="$failed $n"; continue
    fi
    if (cd "$out" && "./$n" > "$n.log" 2>&1); then   # lance dans le dossier temporaire : les fichiers que le test ecrit n'atterrissent pas dans le depot
        echo "ok      $n : $(tail -n 1 "$out/$n.log")"; pass=$((pass+1))
    else
        echo "ECHEC   $n"; tail -n 30 "$out/$n.log"; fail=$((fail+1)); failed="$failed $n"
    fi
done
echo "----- $pass test(s) OK, $fail echec(s)${failed:+ :$failed}"
[ "$fail" -eq 0 ]
