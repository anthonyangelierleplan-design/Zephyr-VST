// LINK des enveloppes : l'enveloppe FILTRE pilote le volume, l'enveloppe AMP devient ENV 2 libre (raccourci DEST / AMOUNT)
#include "../../Source/Synth.h"
#include <cstdio>
#include <vector>
#include <cmath>
using namespace zephyr;
static int bad = 0;
static void check (bool ok, const char* w) { std::printf ("%s %s\n", ok ? "ok  " : "ECHEC", w); if (! ok) ++bad; }
struct R { std::vector<float> rmsT; double hz1, hz2; int activeAfter; };
static double pitchOf (const std::vector<float>& x, double fs) { int z = 0; double f = -1, l = -1; for (size_t i = 1; i < x.size(); ++i) if (x[i-1] <= 0.f && x[i] > 0.f) { double t = i - x[i] / (x[i] - x[i-1]); if (f < 0) f = t; l = t; ++z; } return z > 1 ? (z - 1) * fs / (l - f) : 0; }
static R run (bool link, int dst, float amt)
{
    Synth s; s.prepare (48000.f); SynthParams p; p.wave1 = 3; p.lvl1 = 0.8f; p.lvl2 = 0.f; p.lpf = 1.f; p.lpfRes = 0.f; p.ftype = 1; p.fltEnv = 0.f;
    p.fA = 0.f; p.fD = 0.15f; p.fS = 0.f; p.fR = 0.1f;          // enveloppe filtre : courte, sustain 0
    p.aA = 0.6f; p.aD = 0.3f; p.aS = 1.f; p.aR = 0.5f;          // enveloppe amp : lente, sustain 1
    p.envLink = link; p.env2Dst = dst; p.env2Amt = amt; s.setParams (p); s.noteOn (57, 1.f);
    R r {}; std::vector<float> L (480), Rr (480), early, late;
    for (int b = 0; b < 300; ++b)                                // 3 s, note tenue 2 s
    {
        if (b == 200) s.noteOff (57);
        s.process (L.data(), Rr.data(), 480);
        double e = 0; for (float v : L) e += v * v; r.rmsT.push_back ((float) std::sqrt (e / 480));
        if (b >= 2 && b < 6) early.insert (early.end(), L.begin(), L.end());
        if (b >= 150 && b < 190) late.insert (late.end(), L.begin(), L.end());
    }
    r.hz1 = pitchOf (early, 48000); r.hz2 = pitchOf (late, 48000);
    return r;
}
int main()
{
    const R off = run (false, 0, 0.f), on = run (true, 0, 0.f);
    std::printf ("  LINK off : niveau a 20 ms %.3f, 1 s %.3f   | LINK on : 20 ms %.3f, 1 s %.3f, apres relachement %.4f\n", off.rmsT[2], off.rmsT[100], on.rmsT[2], on.rmsT[100], on.rmsT[260]);
    check (off.rmsT[2] < off.rmsT[100] * 0.6f, "LINK eteint : le volume suit l'enveloppe AMP (attaque lente)");
    check (on.rmsT[2] > 0.05f && on.rmsT[100] < 0.002f, "LINK allume : le volume suit l'enveloppe FILTRE (attaque franche, sustain 0)");
    // ENV 2 -> VCO 2 pitch : VCO 2 seul, l'enveloppe libre monte lentement -> la hauteur monte
    {
        Synth s; s.prepare (48000.f); SynthParams p; p.wave2 = 3; p.lvl1 = 0.f; p.lvl2 = 0.8f; p.lpf = 1.f; p.ftype = 1; p.fA = 0.f; p.fD = 0.5f; p.fS = 1.f; p.fR = 0.2f;
        p.aA = 0.7f; p.aD = 0.3f; p.aS = 1.f; p.aR = 0.3f; p.envLink = true; p.env2Dst = 1; p.env2Amt = 1.f; s.setParams (p); s.noteOn (57, 1.f);
        std::vector<float> L (480), Rr (480), e1, e2; for (int b = 0; b < 250; ++b) { s.process (L.data(), Rr.data(), 480); if (b >= 2 && b < 6) e1.insert (e1.end(), L.begin(), L.end()); if (b >= 200 && b < 240) e2.insert (e2.end(), L.begin(), L.end()); }
        const double h1 = pitchOf (e1, 48000), h2 = pitchOf (e2, 48000);
        std::printf ("  ENV 2 -> VCO 2 PITCH (amount +1) : debut %.1f Hz, apres 2 s %.1f Hz (rapport %.2f)\n", h1, h2, h2 / h1);
        check (h2 / h1 > 1.3, "raccourci ENV 2 -> VCO 2 PITCH agit");
        p.envLink = false; s.setParams (p); 
    }
    {
        const R noDst = run (true, 0, 0.f), dstOff = run (false, 1, 1.f);
        check (std::abs (dstOff.hz2 - 220.0) < 2.0, "raccourci inactif quand LINK est eteint");
        (void) noDst;
    }
    std::printf (bad ? "%d ECHEC(S)\n" : "TOUT OK\n", bad); return bad;
}
