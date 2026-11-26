// Zephyr v4 (repris d'Orage v23) : module CASSETTE, arpegiateur CHORD, vitesse par pas. Compilation :
//   g++ -O2 -std=c++17 -I../../Source v4_cassette_arp.cpp -o t && ./t
#include "Engine.h"
#include "Arp.h"
#include <chrono>
#include <cstdio>
#include <vector>
using namespace zephyr;
static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { ++fails; printf ("  ECHEC : "); printf (__VA_ARGS__); printf ("\n"); } else { printf ("  ok : "); printf (__VA_ARGS__); printf ("\n"); } } while (0)

// ---------------------------------------------------------------- cassette
static void runTape (Tape& t, float fs, std::vector<float>& L, std::vector<float>& R, float wow, float fl, float sat, float age, float hiss, float cr, float mix)
{
    Buf in {}, o {};
    for (size_t p = 0; p < L.size(); p += kSub)
    {
        const int n = (int) std::min<size_t> (kSub, L.size() - p);
        for (int i = 0; i < n; ++i) { in.ch[0][i] = L[p + i]; in.ch[1][i] = R[p + i]; }
        t.process (in, o, n, 2, wow, fl, sat, age, hiss, cr, mix);
        for (int i = 0; i < n; ++i) { L[p + i] = o.ch[0][i]; R[p + i] = o.ch[1][i]; }
    }
}
static double rms (const std::vector<float>& v, size_t from = 0) { double s = 0; for (size_t i = from; i < v.size(); ++i) s += (double) v[i] * v[i]; return std::sqrt (s / (double) (v.size() - from)); }

static void testTape()
{
    printf ("CASSETTE\n");
    const float fs = 48000.f; const int N = (int) fs * 3;
    std::vector<float> sig (N);
    for (int i = 0; i < N; ++i) sig[i] = 0.3f * std::sin (2.f * kPi * 220.f * (float) i / fs) + 0.15f * std::sin (2.f * kPi * 1760.f * (float) i / fs);
    {   // tout a zero = transparent, sans retard
        Tape t; t.prepare (fs); auto L = sig, R = sig; runTape (t, fs, L, R, 0, 0, 0, 0, 0, 0, 1);
        double e = 0; for (int i = 0; i < N; ++i) e = std::max (e, (double) std::fabs (L[i] - sig[i]));
        CHECK (e < 1e-6, "reglages a zero : sortie = entree (ecart max %.2g)", e);
    }
    {   // chaque reglage a fond, puis tout a fond : pas de NaN, crete raisonnable, niveau proche de l'entree
        const char* nm[7] { "WOW", "FLUTTER", "SATURATE", "AGE", "HISS", "CRUNCH", "TOUT" };
        const double r0 = rms (sig, N / 3);
        for (int k = 0; k < 7; ++k)
        {
            float v[6] {}; if (k < 6) v[k] = 1.f; else for (auto& x : v) x = 1.f;
            Tape t; t.prepare (fs); auto L = sig, R = sig; runTape (t, fs, L, R, v[0], v[1], v[2], v[3], v[4], v[5], 1);
            bool fin = true; float pk = 0; for (int i = 0; i < N; ++i) { fin &= std::isfinite (L[i]) && std::isfinite (R[i]); pk = std::max (pk, std::fabs (L[i])); }
            const double db = 20.0 * std::log10 (rms (L, N / 3) / r0);
            CHECK (fin && pk < 1.2f && db > -12.0 && db < 3.0, "%-8s a fond : crete %.2f, niveau %+.1f dB", nm[k], pk, db);
        }
    }
    {   // fuzz : reglages aleatoires changes en cours de route
        Rng r; r.seed (77); int bad = 0; float pk = 0;
        Tape t; t.prepare (44100.f);
        std::vector<float> L (4410), R (4410);
        for (int it = 0; it < 300; ++it)
        {
            for (int i = 0; i < 4410; ++i) { L[i] = r.bi() * (it % 7 == 0 ? 3.f : 0.5f); R[i] = L[i] * 0.7f; }
            runTape (t, 44100.f, L, R, r.uni(), r.uni(), r.uni(), r.uni(), r.uni(), r.uni(), r.uni());
            for (int i = 0; i < 4410; ++i) { if (! std::isfinite (L[i])) ++bad; pk = std::max (pk, std::fabs (L[i])); }
        }
        CHECK (bad == 0, "fuzz 300 reglages aleatoires : %d NaN, crete %.2f", bad, pk);
    }
    {   // le pleurage s'entend : ecart de hauteur mesure par passages a zero sur un sinus 1 kHz
        Tape t; t.prepare (fs); const int M = (int) fs * 4; std::vector<float> L (M), R (M);
        for (int i = 0; i < M; ++i) L[i] = R[i] = 0.3f * std::sin (2.f * kPi * 1000.f * (float) i / fs);
        runTape (t, fs, L, R, 1, 0, 0, 0, 0, 0, 1);
        double lo = 1e9, hi = 0; int last = -1; std::vector<double> per;
        for (int i = (int) fs; i < M; ++i) if (L[i - 1] < 0 && L[i] >= 0) { if (last >= 0) per.push_back (i - last); last = i; }
        for (size_t k = 0; k + 20 < per.size(); k += 20) { double s = 0; for (int j = 0; j < 20; ++j) s += per[k + j]; const double f = fs / (s / 20.0); lo = std::min (lo, f); hi = std::max (hi, f); }
        const double cents = 1200.0 * std::log2 (hi / lo);
        CHECK (cents > 15.0 && cents < 90.0, "WOW a fond : la hauteur flotte de %.0f cents crete a crete", cents);
    }
    {   // CPU
        Tape t; t.prepare (48000.f); std::vector<float> L (48000 * 10), R (48000 * 10);
        for (size_t i = 0; i < L.size(); ++i) L[i] = R[i] = 0.2f * std::sin ((float) i * 0.05f);
        const auto t0 = std::chrono::steady_clock::now();
        runTape (t, 48000.f, L, R, 0.7f, 0.5f, 0.6f, 0.6f, 0.3f, 0.4f, 1);
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        CHECK (sec < 1.0, "CPU : 10 s de stereo tout actif en %.0f ms (%.1f %% d'un coeur)", sec * 1000, sec * 10);
    }
}

// ---------------------------------------------------------------- arpegiateur
struct Played { int pos; int note; bool on; };
static std::vector<Played> runArp (Arp& a, int samples, float stepHz, int block = 256)
{
    std::vector<Played> v; Arp::Ev ev[512];
    for (int p = 0; p < samples; p += block)
    {
        const int n = std::min (block, samples - p), c = a.process (n, stepHz, ev, 512);
        for (int k = 0; k < c; ++k) if (ev[k].lane < 0) v.push_back ({ p + ev[k].pos, ev[k].note, ev[k].on });
    }
    return v;
}
static void testChord()
{
    printf ("ARP CHORD\n");
    ArpPattern pat; Arp a; a.prepare (48000.f); a.setPattern (&pat);
    ArpParams P; P.on = 1; P.dir = kArpDirChord; P.range = 1; P.len = 4; P.gate = 0.5f; a.setParams (P);
    a.noteOn (60, 0.8f); a.noteOn (64, 0.8f); a.noteOn (67, 0.8f);
    auto v = runArp (a, 48000, 8.f);                  // 8 pas par seconde
    std::vector<std::vector<int>> steps; int lastPos = -1;
    for (auto& e : v) if (e.on) { if (e.pos != lastPos) { steps.push_back ({}); lastPos = e.pos; } steps.back().push_back (e.note); }
    CHECK (steps.size() >= 7, "%zu pas joues en 1 s a 8 pas/s", steps.size());
    bool ok = steps.size() >= 2;
    for (size_t k = 0; k < steps.size() && ok; ++k)
    {
        const int o = (k % 2) * 12;
        ok = steps[k].size() == 3 && steps[k][0] == 60 + o && steps[k][1] == 64 + o && steps[k][2] == 67 + o;
    }
    CHECK (ok, "chaque pas = l'accord entier (do-mi-sol), RANGE 2 : une octave sur deux");
    int on = 0; for (auto& e : v) on += e.on ? 1 : -1;
    CHECK (on >= 0 && on <= 3, "toutes les notes s'eteignent avant le pas suivant (%d encore tenues)", on);
    // accent + ratchet sur l'accord
    pat.ratchet[1] = 2; Arp b; b.prepare (48000.f); b.setPattern (&pat); b.setParams (P);
    b.noteOn (60, 0.8f); b.noteOn (64, 0.8f);
    auto w = runArp (b, 11000, 8.f); int ons = 0; for (auto& e : w) ons += e.on ? 1 : 0;
    CHECK (ons == 2 + 4, "ratchet x2 sur le pas 2 : l'accord est rejoue deux fois (%d note-on sur 2 pas)", ons);
}

static std::vector<int> stepStarts (Arp& a, int samples, float hz)
{
    auto v = runArp (a, samples, hz, 128); std::vector<int> s; int last = -1;
    for (auto& e : v) if (e.on && e.pos != last) { s.push_back (e.pos); last = e.pos; }
    return s;
}
static void testSpeed()
{
    printf ("VITESSE PAR PAS\n");
    const float fs = 48000.f, hz = 8.f; const double L = fs / hz;      // 6000 echantillons par pas
    ArpPattern pat; pat.speed[1] = 1; pat.speed[3] = 3;                  // x1, x2, x1, x1/2
    Arp a; a.prepare (fs); a.setPattern (&pat);
    ArpParams P; P.on = 1; P.dir = 0; P.len = 4; a.setParams (P);
    a.noteOn (60, 0.8f);
    auto s = stepStarts (a, (int) (L * 10), hz);
    const double expect[5] { 1.0, 0.5, 1.0, 2.0, 1.0 };
    bool ok = s.size() >= 6;
    for (int k = 0; k < 5 && ok; ++k) ok = std::abs ((double) (s[k + 1] - s[k]) - expect[k] * L) <= 2.0;
    CHECK (ok, "durees des pas x1, x2, x1, x1/2 : %d %d %d %d echantillons (attendu 6000 3000 6000 12000)", s.size() > 4 ? s[1] - s[0] : -1, s.size() > 4 ? s[2] - s[1] : -1, s.size() > 4 ? s[3] - s[2] : -1, s.size() > 4 ? s[4] - s[3] : -1);
    // tous les pas a x1 : comportement d'avant (6000 echantillons, swing compris)
    ArpPattern p2; Arp b; b.prepare (fs); b.setPattern (&p2); P.swing = 0.4f; b.setParams (P); b.noteOn (60, 0.8f);
    auto s2 = stepStarts (b, (int) (L * 6), hz);
    CHECK (s2.size() >= 4 && std::abs ((s2[1] - s2[0]) - 1.2 * L) <= 2 && std::abs ((s2[2] - s2[1]) - 0.8 * L) <= 2, "x1 partout + swing : pas pair %d, impair %d (inchange)", s2.size() > 2 ? s2[1] - s2[0] : -1, s2.size() > 2 ? s2[2] - s2[1] : -1);
    // SEQ cale sur le DAW avec des vitesses : la position suit le morceau, sans recalage a chaque bloc
    ArpPattern p3; p3.speed[2] = 1; p3.speed[5] = 2; p3.speed[6] = 3; for (int k = 0; k < 16; ++k) p3.seqNote[k] = 48 + k;
    Arp c; c.prepare (fs); c.setPattern (&p3);
    ArpParams Q; Q.on = 1; Q.seq = true; Q.len = 7; Q.swing = 0.3f; c.setParams (Q);          // swing 0,3 -> decalage 0,15 (pair 1,15 ; impair 0,85)
    c.noteOn (60, 0.8f);
    Arp::Ev ev[512]; std::vector<int> starts; std::vector<int> notes;
    const int blk = 512; double host = 0.0;
    for (int p = 0; p < (int) (L * 40); p += blk)
    {
        c.setHost (true, host);
        const int n = c.process (blk, hz, ev, 512);
        for (int k = 0; k < n; ++k) if (ev[k].lane < 0 && ev[k].on) { starts.push_back (p + ev[k].pos); notes.push_back (ev[k].note); }
        host += (double) blk / L;
    }
    // duree attendue de chaque pas en partant du pas 0 : (1 +/- sw) / vitesse
    double t = 0; bool ok3 = starts.size() > 20; int worst = 0;
    for (size_t k = 0; k + 1 < starts.size() && ok3; ++k)
    {
        const int ks = notes[k] - 48;                         // la note dit quel pas du motif a joue
        const double d = ((k & 1) ? 0.85 : 1.15) / (double) speedMul (p3.speed[ks].load()) * L;
        (void) t;
        const int err = std::abs ((int) std::lround ((double) (starts[k + 1] - starts[k]) - d));
        worst = std::max (worst, err);
        if (err > 3 && getenv ("DBG")) printf ("    pas %zu (motif %d) : %d au lieu de %.0f\n", k, ks, starts[k + 1] - starts[k], d);
    }
    ok3 = ok3 && worst <= 3;
    bool order = true; for (size_t k = 0; k < notes.size(); ++k) order &= notes[k] - 48 == (int) (k % 7);
    CHECK (ok3 && order, "SEQ cale sur le DAW (7 pas, swing, x2 / x4 / x1/2) : %zu pas dans l'ordre, ecart max %d echantillons", starts.size(), worst);
}

static void testPatchZ2()
{
    printf ("PATCH (anciens projets)\n");
    static Engine e;
    // patch z2 : 12 noeuds (NOut etait le 12e) puis les entrees CV ; sortie branchee sur la reverb + l'ensemble, delay <- saturation, LFO1 -> LPF
    std::string s = "z2:";
    for (int i = 0; i < 12; ++i) s += std::to_string (i == 11 ? ((1u << NVerb) | (1u << NEnsemble)) : (i == NDelay ? (1u << NSat) : 0u)) + ",";
    for (int d = 0; d < kNumCv; ++d) s += std::to_string (d == CvLpf ? 1 : 0) + ",";
    e.loadPatch (s);
    CHECK (e.audioIn[NOut].load() == ((1u << NVerb) | (1u << NEnsemble)) && e.audioIn[NDelay].load() == (1u << NSat) && e.audioIn[NTape].load() == 0 && e.cvIn[CvLpf].load() == 1,
           "patch z2 relu : sortie, cables et CV a leur place, CASSETTE libre");
    const std::string z3 = e.serializePatch(); static Engine f; f.loadPatch (z3);
    bool same = true; for (int n = 0; n < kNumNodes; ++n) same &= f.audioIn[n].load() == e.audioIn[n].load();
    CHECK (z3.compare (0, 3, "z3:") == 0 && same, "patch z3 ecrit puis relu a l'identique");
    e.setRack ({ NTape, NVerb }); const auto r = e.getRack();
    CHECK (r.size() == 2 && r[0] == NTape, "CASSETTE se pose sur le rack");
    e.setDefaultRack(); const auto d = e.getRack();
    CHECK (d.size() == 11, "rack par defaut : les 11 modules (%zu)", d.size());
}

int main()
{
    testTape(); testChord(); testSpeed(); testPatchZ2();
    printf (fails ? "\n%d ECHEC(S)\n" : "\nTOUT EST OK\n", fails);
    return fails ? 1 : 0;
}
