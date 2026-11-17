#include <vector>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <string>
#include <functional>
#include "../../Source/Engine.h"
using namespace zephyr;
static const float FS = 48000.f;
static std::vector<float> runE (Engine& e, const std::vector<float>& in, double* msOut = nullptr)
{
    std::vector<float> L (in), R (in);
    auto t0 = std::chrono::steady_clock::now();
    for (size_t p = 0; p + 512 <= L.size(); p += 512) { float* io[2] { L.data() + p, R.data() + p }; e.process (io, 512, 2); }
    if (msOut) *msOut = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
    return L;
}
static std::vector<float> sine (float f, float a, float sec) { std::vector<float> x ((size_t) (FS * sec)); for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (a * std::sin (6.283185307179586 * f * (double) i / 48000.0)); return x; }
static void save (const std::string& n, const std::vector<float>& x) { FILE* f = fopen (("fx_" + n + ".raw").c_str(), "wb"); fwrite (x.data(), 4, x.size(), f); fclose (f); }
struct T { std::string name; int node; std::function<void (Params&)> set; };
#include <xmmintrin.h>
#include <pmmintrin.h>
int main()
{
    _mm_setcsr (_mm_getcsr() | 0x8040);   // comme juce::ScopedNoDenormals (FTZ + DAZ)
    std::vector<T> ts {
        { "sat0", NSat, [] (Params& p) { p.satMode = 0; p.satDrive = 0.8f; } },
        { "sat1", NSat, [] (Params& p) { p.satMode = 1; p.satDrive = 0.8f; } },
        { "sat2", NSat, [] (Params& p) { p.satMode = 2; p.satDrive = 0.8f; } },
        { "color", NColor, [] (Params& p) { p.low = 0.5f; p.mid = -0.3f; p.high = 0.5f; } },
        { "filter", NFilter, [] (Params& p) { p.lpf = 0.7f; p.lpfPeak = 0.6f; } },
        { "comp", NComp, [] (Params& p) { p.compAmt = 0.8f; } },
        { "phaser", NPhaser, [] (Params& p) { p.phMix = 1.f; } },
        { "ens", NEnsemble, [] (Params& p) { p.ensMix = 1.f; } },
        { "delay", NDelay, [] (Params& p) { p.dlyMix = 1.f; p.dlyFb = 0.f; } },
        { "verb", NVerb, [] (Params& p) { p.revMix = 1.f; } },
        { "wide", NWide, [] (Params& p) { p.wideWidth = 0.8f; p.wideSpread = 0.6f; } } };
    for (auto& t : ts)
    {
        // 1) sinus 3,1 kHz (saturations : repliement) puis 9,7 kHz (modulations : bruit d'interpolation)
        for (float f : { 3100.f, 9700.f })
        {
            Engine e; e.prepare (FS); Params p; p.output = 0.f; t.set (p); e.setParams (p); e.setRack ({ t.node });
            auto y = runE (e, sine (f, 0.5f, 2.5f)); y.erase (y.begin(), y.begin() + (size_t) FS / 2); save (t.name + "_" + std::to_string ((int) f), y);
        }
        // 2) silence apres un signal : temps de calcul (denormaux) et bruit residuel
        {
            Engine e; e.prepare (FS); Params p; p.output = 0.f; t.set (p); e.setParams (p); e.setRack ({ t.node });
            double msA, msB; runE (e, sine (220.f, 0.5f, 1.f), &msA); auto y = runE (e, std::vector<float> ((size_t) (FS * 20.f), 0.f), &msB);
            float tail = 0; for (size_t i = y.size() - 48000; i < y.size(); ++i) tail = std::max (tail, std::fabs (y[i]));
            std::printf ("%-7s : calcul 1 s de son %.1f ms, 1 s de silence %.1f ms (x%.1f) ; residu apres 20 s %.1f dB\n", t.name.c_str(), msA, msB / 20.0, msB / 20.0 / msA, 20 * std::log10 (tail + 1e-12));
        }
    }
    // 3) reverb : reponse impulsionnelle
    { Engine e; e.prepare (FS); Params p; p.output = 0.f; p.revMix = 1.f; p.revDecay = 0.6f; e.setParams (p); e.setRack ({ NVerb }); std::vector<float> x ((size_t) (FS * 6), 0.f); x[100] = 0.8f; save ("verb_ir", runE (e, x)); }
}
