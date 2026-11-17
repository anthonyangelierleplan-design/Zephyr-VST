#include <vector>
#include <cmath>
#include <cstdio>
#include "../../Source/Synth.h"
using namespace zephyr;
// "clic" : saut de la derivee seconde au moment de l'evenement, compare au regime etabli (sinus, filtre ouvert)
static double clickDb (const std::vector<float>& x, int at)
{
    auto d2 = [&] (int i) -> double { return std::fabs (x[i + 1] - 2 * x[i] + x[i - 1]); };
    double base = 0; for (int i = at - 4000; i < at - 200; ++i) base = std::max (base, d2 (i));
    double ev = 0; for (int i = at - 50; i < at + 400; ++i) ev = std::max (ev, d2 (i));
    return 20 * std::log10 ((ev + 1e-9) / (base + 1e-9));
}
int main()
{
    struct Case { const char* name; int assign; int mode; };
    for (int c = 0; c < 4; ++c)
    {
        Synth s; s.prepare (48000.f); SynthParams p; p.wave1 = 3; p.wave2 = 3; p.lvl1 = 0.6f; p.lvl2 = 0.f; p.lpf = 1.f; p.lpfRes = 0.f; p.ftype = 1;
        p.aA = 0.05f; p.aD = 0.3f; p.aS = 1.f; p.aR = 0.5f;
        if (c == 1) p.assign = 1;  // mono
        s.setParams (p);
        std::vector<float> L (480), R (480), o; int at = 0;
        auto run = [&] (int blocks) { for (int b = 0; b < blocks; ++b) { s.process (L.data(), R.data(), 480); o.insert (o.end(), L.begin(), L.end()); } };
        const char* nm = "";
        if (c == 0) { nm = "vol de voix (13e note, 12 voix tenues)"; for (int n = 0; n < 12; ++n) s.noteOn (48 + n, 0.9f); run (40); at = (int) o.size(); s.noteOn (72, 0.9f); run (20); }
        if (c == 1) { nm = "MONO : nouvelle note (relachee puis rejouee)"; s.noteOn (48, 0.9f); run (40); s.noteOff (48); at = (int) o.size(); s.noteOn (55, 0.9f); run (20); }
        if (c == 2) { nm = "MONO : legato (note tenue + nouvelle)"; s.noteOn (48, 0.9f); run (40); at = (int) o.size(); s.noteOn (55, 0.9f); run (20); }
        if (c == 3) { nm = "fin de release (voix qui s'eteint)"; p.aR = 0.002f; s.setParams (p); s.noteOn (60, 0.9f); run (40); s.noteOff (60); at = (int) o.size(); run (20); }
        std::printf ("  %-48s : saut %+6.1f dB au-dessus du regime normal\n", nm, clickDb (o, at));
    }
}
