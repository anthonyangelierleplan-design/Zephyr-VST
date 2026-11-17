// VCO : phase libre ou recalee a chaque note (SETTINGS > VCO PHASE) ; hard sync sans repliement
#include "../../Source/Synth.h"
#include <cstdio>
#include <vector>
#include <cmath>
#include <complex>
using namespace zephyr;
static int bad = 0;
static void check (bool ok, const char* w) { std::printf ("%s %s\n", ok ? "ok  " : "ECHEC", w); if (! ok) ++bad; }
static std::vector<float> note (Synth& s, int n)
{
    std::vector<float> L (480), R (480), o; s.noteOn (n, 1.f);
    for (int b = 0; b < 4; ++b) { s.process (L.data(), R.data(), 480); o.insert (o.end(), L.begin(), L.end()); }
    s.noteOff (n); for (int b = 0; b < 200; ++b) s.process (L.data(), R.data(), 480);   // la voix s'eteint
    return o;
}
static double diff (const std::vector<float>& a, const std::vector<float>& b) { double d = 0, e = 0; for (size_t i = 0; i < a.size(); ++i) { d += (a[i] - b[i]) * (a[i] - b[i]); e += a[i] * a[i]; } return std::sqrt (d / std::max (1e-12, e)); }
static double worstAlias (const std::vector<float>& x, double fs, double f0)        // pire raie hors harmoniques (dB sous la plus forte harmonique), DFT par raies
{
    const int N = (int) x.size(); std::vector<double> w (N); for (int i = 0; i < N; ++i) w[i] = 0.42 - 0.5 * std::cos (2 * M_PI * i / (N - 1)) + 0.08 * std::cos (4 * M_PI * i / (N - 1));
    auto mag = [&] (double f) { std::complex<double> s = 0; for (int i = 0; i < N; i += 1) s += x[i] * w[i] * std::polar (1.0, -2 * M_PI * f * i / fs); return std::norm (s); };
    double hmax = 0; for (int k = 1; k * f0 < fs / 2; ++k) hmax = std::max (hmax, mag (k * f0));
    double amax = 0; for (double f = 200; f < fs / 2 - 200; f += 37.0) { const double r = std::fmod (f, f0); if (r < 60 || f0 - r < 60) continue; amax = std::max (amax, mag (f)); }
    return 10 * std::log10 (amax / hmax);
}
int main()
{
    for (int mode = 0; mode < 2; ++mode)
    {
        Synth s; s.prepare (48000.f); SynthParams p; p.wave1 = 0; p.wave2 = 0; p.lvl1 = 0.7f; p.lvl2 = 0.7f; p.fine2 = 7.f; p.lpf = 1.f; p.ftype = 1; p.aA = 0.f; p.aS = 1.f; p.aR = 0.01f;
        s.setParams (p); s.setPhaseFree (mode == 0);
        const auto a = note (s, 48), b = note (s, 48), c = note (s, 48);
        const double d = std::max (diff (a, b), diff (b, c));
        std::printf ("  %s : ecart entre deux attaques %.3f\n", mode == 0 ? "libre" : "recale", d);
        check (mode == 0 ? d > 0.2 : d < 1e-4, mode == 0 ? "phase libre : chaque attaque est differente" : "phase recalee : chaque attaque est identique");
    }
    {
        Synth s; s.prepare (48000.f); SynthParams p; p.wave1 = 0; p.wave2 = 0; p.lvl1 = 0.f; p.lvl2 = 0.8f; p.sync = 1; p.tune2 = 15.7f; p.lpf = 1.f; p.ftype = 1; p.aA = 0.f; p.aS = 1.f;
        s.setParams (p); s.setPhaseFree (false); s.noteOn (84, 1.f);
        std::vector<float> L (480), R (480), o; for (int b = 0; b < 20; ++b) s.process (L.data(), R.data(), 480);
        for (int b = 0; b < 20; ++b) { s.process (L.data(), R.data(), 480); o.insert (o.end(), L.begin(), L.end()); }
        const double f0 = 440.0 * std::pow (2.0, (84 - 69) / 12.0), wa = worstAlias (o, 48000, f0);
        std::printf ("  hard sync Do 6, VCO 2 +15,7 demi-tons : pire raie parasite %.1f dB\n", wa);
        check (wa < -45.0, "hard sync sans repliement notable (< -45 dB ; avant : env. -38 dB)");
    }
    std::printf (bad ? "%d ECHEC(S)\n" : "TOUT OK\n", bad); return bad;
}
