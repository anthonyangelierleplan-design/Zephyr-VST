// Arpegiateur / sequenceur facon Korg Monopoly, etendu : grille de pas (silence / note / accent, ratchet), gate, swing, probabilite,
// 6 directions et 4 pistes de modulation (16 pas chacune, longueurs independantes) qui pilotent n'importe quelle destination.
// Les pas sont cadences par une horloge propre (libre ou calee sur le tempo). Le motif des notes depend du mode d'affectation des touches
// (POLY arpege les notes tenues en faisant tourner les pas sur 4 VCO, comme le Mono/Poly, avec SHIFT et CLUSTER ;
// MONO / UNISON arpegent aussi les notes tenues mais sur une seule voix ; CHORD l'accord memorise, SHARE le partage des voix).
// Changer de mode d'affectation ne coupe pas l'arpege : les notes en cours sont relachees proprement et le motif continue.
// Aucune dependance JUCE.
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace zephyr
{
constexpr int kArpSteps = 16, kArpLanes = 8;

// motif partage entre l'interface et le thread audio (valeurs atomiques, lecture sans verrou)
struct ArpPattern
{
    std::atomic<int> step[kArpSteps];       // 0 silence, 1 note, 2 accent
    std::atomic<int> ratchet[kArpSteps];    // 1..4 repetitions de la note dans le pas
    std::atomic<float> lane[kArpLanes][kArpSteps];   // 0..1
    std::atomic<int> seqNote[kArpSteps];    // mode SEQ : hauteur MIDI de chaque pas
    std::atomic<int> tie[kArpSteps];        // mode SEQ : 1 = la note tient tout le pas (sans trou avec la suivante)
    std::atomic<int> recPos { 0 };          // mode SEQ : prochain pas a ecrire en enregistrement
    ArpPattern() { reset(); }
    void reset()
    {
        for (int i = 0; i < kArpSteps; ++i) { step[i] = 1; ratchet[i] = 1; seqNote[i] = 60; tie[i] = 0; for (auto& l : lane) l[i] = 0.f; }
        recPos = 0;
    }
    void set (int i, int st, int rt) { step[i] = st; ratchet[i] = rt; }
};

struct ArpParams
{
    int on = 0;            // 0 off, 1 on, 2 latch
    int dir = 0;           // 0 up, 1 down, 2 up-down, 3 played, 4 random, 5 converge
    int range = 0;         // 0..3 : 1 a 4 octaves
    int ratchet = 1;       // ratchet global 1..4 (comme le potard RATCHET de l'Hydrasynth) : repetitions minimum de chaque pas
    int assign = 0;        // mode d'affectation des touches (voir Synth : poly, mono, unison, share, chord)
    bool hold = false;     // HOLD du synthe : meme effet que LATCH
    float gate = 0.5f;     // duree de la note dans le pas (0.1 .. 1)
    float swing = 0.f;     // 0 .. 1 (0.5 de decalage maxi)
    float chance = 1.f;    // probabilite de jouer un pas actif
    bool seq = false;      // mode SEQ : une note memorisee par pas (au lieu des touches tenues)
    bool free = false;     // les pistes de modulation tournent meme sans touche tenue
    int len = 8;           // longueur du motif de notes (1..16)
    int laneLen[kArpLanes] { 8, 8, 8, 8, 8, 8, 8, 8 };
    float shift = 0.f;     // mode POLY : SHIFT, decale chaque VCO d'une fraction de pas differente
    float cluster = 0.f;   // mode POLY : CLUSTER, resserre les 4 pas d'un tour vers son debut (a fond : accord plaque)
};

class Arp
{
public:
    // lane >= 0 : evenement de piste de modulation (valeur 0..1), sinon evenement de note
    // slot >= 0 : note jouee par un VCO precis (mode POLY, rotation sur 4)
    struct Ev { int pos; bool on; int note; float vel; bool stack; int lane = -1; float val = 0.f; int slot = -1; };

    void prepare (float sampleRate) { fs = sampleRate; reset(); }
    void reset() { nPend = 0; rot = 0; for (auto& sn : slotNote) sn = -1; soundingSlot = curSlot = -1; flushReq = false; nKeys = 0; sounding = -1; soundingStack = false; idx = 0; phase = 0.999999f; stamp = 0; stepCount = 0; running = false; stepOdd = false; active = false; ratchetN = 1; lastSub = -1; idle = 1.0e12; }
    void setParams (const ArpParams& p) { P = p; }
    void setPattern (const ArpPattern* p) { pat = p; }
    void setChord (const int* iv, int n) { chordN = std::min (6, std::max (0, n)); for (int i = 0; i < chordN; ++i) chord[i] = iv[i]; }
    void allOff() { nKeys = 0; }
    void resetVoices() { flushReq = true; }
    // mode SEQ : horloge calee sur le DAW quand il joue (position en pas depuis le debut du morceau, au debut du bloc)
    void setHost (bool lock, double stepPos) { hostLock = lock && stepPos >= 0.0; hostStep = stepPos; }        // changement de mode : on relache ce qui sonne, les touches et le motif restent
    int currentStep() const { return running ? (stepCount > 0 ? (stepCount - 1) : 0) : -1; }   // pour l'affichage du pas courant

    void noteOn (int note, float vel)
    {
        const bool latch = P.on == 2 || P.hold;
        if (latch && physCount() == 0 && nKeys > 0) nKeys = 0;           // nouvelle prise apres relachement complet : remplace les notes verrouillees
        for (int i = 0; i < nKeys; ++i) if (keys[i].note == note) { keys[i].vel = vel; keys[i].phys = true; keys[i].t = ++stamp; return; }
        if (nKeys == 0 && ! P.free && ! P.seq) { phase = 0.999999f; idx = 0; stepCount = 0; }   // le premier pas part tout de suite (SEQ : l'horloge continue, voir process)
        if (nKeys < kKeys) keys[nKeys++] = { note, vel, true, ++stamp };
    }
    void noteOff (int note)
    {
        for (int i = 0; i < nKeys; ++i)
            if (keys[i].note == note)
            {
                if (P.on == 2 || P.hold) keys[i].phys = false;
                else { for (int j = i; j + 1 < nKeys; ++j) keys[j] = keys[j + 1]; --nKeys; }
                return;
            }
    }

    // stepHz : cadence des pas ; renvoie le nombre d'evenements ecrits
    int process (int n, float stepHz, Ev* out, int maxOut)
    {
        int cnt = 0;
        auto off = [&] (int pos)
        {
            if (sounding >= 0 && cnt < maxOut)
            {
                Ev e { pos, false, sounding, 0.f, soundingStack }; e.slot = soundingSlot;
                if (soundingSlot >= 0) slotNote[soundingSlot] = -1;
                out[cnt++] = e;
            }
            sounding = -1; soundingSlot = -1;
        };
        auto slotsOff = [&] (int pos)
        {
            nPend = 0;
            for (int k = 0; k < kVco; ++k)
                if (slotNote[k] >= 0 && cnt < maxOut) { Ev e { pos, false, slotNote[k], 0.f, false }; e.slot = k; out[cnt++] = e; slotNote[k] = -1; }
        };
        auto dropPendingOns = [&] { int w = 0; for (int k = 0; k < nPend; ++k) if (! pend[k].ev.on) pend[w++] = pend[k]; nPend = w; };
        if (flushReq) { flushReq = false; off (0); slotsOff (0); active = false; rot = 0; }
        auto lanesOut = [&] (int pos, bool zero)
        {
            for (int l = 0; l < kArpLanes && cnt < maxOut; ++l)
            {
                Ev e { pos, false, 0, 0.f, false, l, 0.f };
                if (! zero && pat != nullptr) e.val = pat->lane[l][stepCount % std::max (1, std::min (kArpSteps, P.laneLen[l]))].load();
                out[cnt++] = e;
            }
        };
        if (P.on == 0) { off (0); slotsOff (0); if (running) { lanesOut (0, true); running = false; } phase = 0.999999f; stepCount = 0; rot = 0; return cnt; }
        if (P.assign != 0 && nPend > 0) slotsOff (0);
        const float stepLen = stepHz > 0.01f ? fs / stepHz : fs;
        const float sh = std::min (1.f, std::max (0.f, P.shift)), cl = std::min (1.f, std::max (0.f, P.cluster));
        const bool planned = P.assign == 0 && ! P.seq && (sh > 0.001f || cl > 0.001f);
        const float base = std::max (0.f, stepHz) / fs;
        const float sw = std::min (0.5f, std::max (0.f, P.swing) * 0.5f);
        if (P.seq && hostLock)
        {
            // SEQ calé sur le DAW : position "sans swing" de l'horloge interne comparee a celle du morceau ; recalage si elles divergent
            const int k = std::max (0, stepCount - 1);
            const double sIn = 2.0 * (double) (k / 2) + ((k & 1) == 0 ? (double) std::min (phase, 1.f) * (1.0 + sw) : (1.0 + sw) + (double) std::min (phase, 1.f) * (1.0 - sw));
            if (std::abs (sIn - hostStep) > 0.02 || stepCount == 0)
            {
                const double m = std::floor (hostStep / 2.0), u = hostStep - 2.0 * m;
                const int kk = (int) (2.0 * m) + (u < 1.0 + sw ? 0 : 1);
                const float ph = (float) (u < 1.0 + sw ? u / (1.0 + sw) : (u - 1.0 - sw) / (1.0 - sw));
                stepOdd = (kk & 1) != 0;
                if (go0() && ph * stepLen < 0.03f * fs) { stepCount = kk; phase = 1.f + ph; }  // le pas vient de commencer (< 30 ms) : il part tout de suite
                else { stepCount = kk + 1; phase = ph; active = false; }
            }
        }
        for (int i = 0; i < n; ++i)
        {
            const bool go = go0();
            if (! go)
            {
                off (i); dropPendingOns();
                if (running) { lanesOut (i, true); running = false; }
                if (P.seq)
                {
                    // SEQ : l'horloge continue sans jouer, pour qu'une nouvelle note reparte en tempo
                    if (idle < 1.0e9) idle += 1.0;
                    phase += base / (stepOdd ? (1.f - sw) : (1.f + sw));
                    if (phase >= 1.f) { phase -= std::floor (phase); const int cs = stepCount++; stepOdd = (cs & 1) != 0; }
                    active = false;
                    continue;
                }
                phase = 0.999999f; idx = 0; stepCount = 0; active = false; rot = 0;
                continue;
            }
            if (P.seq && ! running)
            {
                const double idleMax = (double) stepLen * (double) std::max (4, std::min (kArpSteps, P.len));
                if (! hostLock && idle > idleMax) { phase = 0.999999f; stepCount = 0; idx = 0; }   // longue pause : le motif repart du debut, tout de suite
                else if (phase * stepLen < 0.03f * fs && stepCount > 0) { --stepCount; phase += 1.f; }   // reprise en tempo : touche a peine en retard (< 30 ms) -> le pas en cours joue quand meme
            }
            idle = 0.0;
            running = true;
            phase += base / (stepOdd ? (1.f - sw) : (1.f + sw));
            if (phase >= 1.f)
            {
                phase -= std::floor (phase);
                off (i);
                lastSub = -1;
                lanesOut (i, false);
                const int cs = stepCount++;
                stepOdd = (cs & 1) != 0;
                active = false;
                tieNow = false;
                if (pat != nullptr && P.seq && (nKeys > 0 || P.free))
                {
                    const int pos = cs % std::max (1, std::min (kArpSteps, P.len));
                    const int st = pat->step[pos].load();
                    if (st > 0 && uni() < P.chance)
                    {
                        const int trans = nKeys > 0 ? lastNote() - 60 : 0;         // la touche jouee transpose le motif (do central = hauteurs d'origine)
                        curNote = std::min (127, std::max (0, pat->seqNote[pos].load() + trans));
                        const float v = nKeys > 0 ? lastVel() : 0.8f;
                        curVel = st == 2 ? std::min (1.f, v * 1.15f + 0.25f) : v;
                        ratchetN = std::max (std::max (1, std::min (4, P.ratchet)), std::min (4, pat->ratchet[pos].load()));
                        tieNow = pat->tie[pos].load() != 0 && ratchetN == 1;
                        active = true;
                    }
                }
                else if (pat != nullptr && nKeys > 0 && planned)
                {
                    // POLY avec SHIFT / CLUSTER : chaque tour de 4 pas est planifie d'un coup, un VCO par pas
                    if (rot == 0)
                    {
                        static const float kOff[kVco] { 0.f, 0.46f, 0.21f, 0.71f };
                        int seq[128]; const int len = buildSeq (seq);
                        const int gate = std::max (16, (int) (stepLen * std::min (0.98f, std::max (0.1f, P.gate)) * (1.f + 3.f * cl)));
                        for (int k = 0; k < kVco && len > 0 && nPend + 2 <= kPend; ++k)
                        {
                            const int pos = (cs + k) % std::max (1, std::min (kArpSteps, P.len));
                            const int st = pat->step[pos].load();
                            if (st <= 0 || uni() >= P.chance) continue;
                            if (P.dir == 4) idx = (int) (uni() * len) % len; else idx %= len;
                            const int note = seq[idx]; if (P.dir != 4) ++idx;
                            const float v = lastVel(), vel = st == 2 ? std::min (1.f, v * 1.15f + 0.25f) : v;
                            const int at = i + (int) ((float) k * stepLen * (1.f - cl) + sh * kOff[k] * stepLen);
                            Ev on { 0, true, note, vel, false }; on.slot = k;
                            Ev of { 0, false, note, 0.f, false }; of.slot = k;
                            pend[nPend++] = { at, on }; pend[nPend++] = { at + gate, of };
                        }
                    }
                    rot = (rot + 1) % kVco;
                }
                else if (pat != nullptr && nKeys > 0)
                {
                    const int pos = cs % std::max (1, std::min (kArpSteps, P.len));
                    const int st = pat->step[pos].load();
                    if (st > 0 && uni() < P.chance)
                    {
                        int seq[128]; const int len = buildSeq (seq);
                        if (len > 0)
                        {
                            if (P.dir == 4) idx = (int) (uni() * len) % len; else idx %= len;
                            curNote = seq[idx]; if (P.dir != 4) ++idx;
                            const float v = lastVel();
                            curVel = st == 2 ? std::min (1.f, v * 1.15f + 0.25f) : v;
                            ratchetN = std::max (std::max (1, std::min (4, P.ratchet)), std::min (4, pat->ratchet[pos].load()));
                            active = true;
                            curSlot = -1;
                            if (P.assign == 0) { curSlot = rot; rot = (rot + 1) % kVco; }    // POLY : rotation sur les 4 VCO (comme le Mono/Poly)
                        }
                    }
                }
            }
            if (active)
            {
                const int sub = std::min (ratchetN - 1, (int) (phase * (float) ratchetN));
                const float sp = phase * (float) ratchetN - (float) sub;
                if (sub != lastSub)
                {
                    off (i); lastSub = sub;
                    const bool stack = P.assign == 2;
                    const int sl = P.seq ? -1 : curSlot;
                    if (cnt < maxOut)
                    {
                        Ev e { i, true, curNote, curVel, stack }; e.slot = sl; out[cnt++] = e;
                        sounding = curNote; soundingStack = stack; soundingSlot = sl;
                        if (sl >= 0) slotNote[sl] = curNote;
                    }
                }
                else if (sounding >= 0 && ! tieNow && sp >= std::min (0.98f, std::max (0.05f, P.gate))) off (i);
            }
            else if (sounding >= 0) off (i);
        }
        int w = 0;                                                             // evenements planifies : ceux qui tombent dans ce bloc partent, les autres attendent
        for (int k = 0; k < nPend; ++k)
        {
            if (pend[k].at < n)
            {
                if (cnt < maxOut)
                {
                    auto e = pend[k].ev; e.pos = std::max (0, pend[k].at);
                    if (e.on)
                    {
                        if (slotNote[e.slot] >= 0) { Ev o { e.pos, false, slotNote[e.slot], 0.f, false }; o.slot = e.slot; if (cnt < maxOut) out[cnt++] = o; }
                        slotNote[e.slot] = e.note;
                    }
                    else if (slotNote[e.slot] == e.note) slotNote[e.slot] = -1;
                    else continue;                                             // deja relachee (la note suivante a pris le VCO)
                    if (cnt < maxOut) out[cnt++] = e;
                }
            }
            else { pend[w] = pend[k]; pend[w].at -= n; ++w; }
        }
        nPend = w;
        return cnt;
    }

private:
    static constexpr int kKeys = 16;
    struct Key { int note; float vel; bool phys; uint64_t t; };
    Key keys[kKeys];
    int nKeys = 0, sounding = -1, idx = 0, chord[6] { 0, 4, 7, 11, 0, 0 }, chordN = 4, stepCount = 0, curNote = 60, ratchetN = 1, lastSub = -1;
    bool soundingStack = false, running = false, stepOdd = false, active = false, tieNow = false, flushReq = false;
    static constexpr int kVco = 4, kPend = 64;
    int slotNote[kVco] { -1, -1, -1, -1 }, rot = 0, nPend = 0, curSlot = -1, soundingSlot = -1;
    struct Pend { int at; Ev ev; } pend[kPend];
    float fs = 44100.f, phase = 0.999999f, curVel = 0.8f;
    double idle = 1.0e12, hostStep = 0.0; bool hostLock = false;
    bool go0() const { return nKeys > 0 || P.free; }
    uint64_t stamp = 0; uint32_t rng = 0x1234567u;
    ArpParams P;
    const ArpPattern* pat = nullptr;

    float uni() { rng = rng * 1664525u + 1013904223u; return (float) (rng >> 8) / 16777216.f; }
    int physCount() const { int c = 0; for (int i = 0; i < nKeys; ++i) c += keys[i].phys ? 1 : 0; return c; }
    float lastVel() const { float v = 0.8f; uint64_t t = 0; for (int i = 0; i < nKeys; ++i) if (keys[i].t >= t) { t = keys[i].t; v = keys[i].vel; } return v; }
    int lastNote() const { int nt = 60; uint64_t t = 0; for (int i = 0; i < nKeys; ++i) if (keys[i].t >= t) { t = keys[i].t; nt = keys[i].note; } return nt; }

    int buildSeq (int* seq) const
    {
        int base[16], nb = 0;
        switch (P.assign)
        {
            case 4:                                                                           // chord memory
                base[nb++] = lastNote();
                for (int i = 0; i < chordN && nb < 12; ++i) base[nb++] = lastNote() + chord[i];
                break;
            case 3:                                                                           // share : 6 voix reparties dans l'ordre de jeu
            {
                Key byTime[kKeys]; for (int i = 0; i < nKeys; ++i) byTime[i] = keys[i];
                std::sort (byTime, byTime + nKeys, [] (const Key& a, const Key& b) { return a.t < b.t; });
                for (int v = 0; v < 6; ++v) base[nb++] = byTime[v % nKeys].note;
                break;
            }
            case 1: case 2:                                                                   // mono / unison : les notes tenues, jouees par une seule voix
            default:                                                                          // poly : notes tenues
            {
                if (P.dir == 3)                                                               // ordre de jeu
                {
                    Key byTime[kKeys]; for (int i = 0; i < nKeys; ++i) byTime[i] = keys[i];
                    std::sort (byTime, byTime + nKeys, [] (const Key& a, const Key& b) { return a.t < b.t; });
                    for (int i = 0; i < nKeys; ++i) base[nb++] = byTime[i].note;
                }
                else { for (int i = 0; i < nKeys; ++i) base[nb++] = keys[i].note; std::sort (base, base + nb); }
                break;
            }
        }
        const int oct = std::max (1, std::min (4, P.range + 1));
        int up[64], nu = 0;
        for (int o = 0; o < oct; ++o)
            for (int i = 0; i < nb && nu < 64; ++i) if (base[i] + 12 * o <= 127) up[nu++] = base[i] + 12 * o;
        int len = 0;
        if (P.dir == 1)       { for (int i = nu - 1; i >= 0; --i) seq[len++] = up[i]; }
        else if (P.dir == 2)
        {
            for (int i = 0; i < nu; ++i) seq[len++] = up[i];
            for (int i = nu - 2; i >= 1; --i) seq[len++] = up[i];
        }
        else if (P.dir == 5)                                                                  // converge : bas, haut, bas+1, haut-1...
        {
            int lo = 0, hi = nu - 1;
            while (lo <= hi) { seq[len++] = up[lo++]; if (lo <= hi) seq[len++] = up[hi--]; }
        }
        else                  { for (int i = 0; i < nu; ++i) seq[len++] = up[i]; }            // up, played (ordre deja fait), random (tire dans la liste)
        return len;
    }
};
} // namespace zephyr
