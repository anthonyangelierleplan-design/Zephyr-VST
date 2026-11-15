#include "PluginEditor.h"

using namespace zephyr;

namespace
{
juce::String fr (const char* utf8) { return juce::String::fromUTF8 (utf8); }
juce::Font font (float h, bool bold = true) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }
juce::Colour C (juce::uint32 c) { return juce::Colour (c); }

// (aucune usure : Zephyr est neuf et brillant)
void drawSpecks (juce::Graphics&, juce::Rectangle<int>) {}

// petite vis cruciforme polie
void drawScrew (juce::Graphics& g, juce::Point<float> p, float r, int seed)
{
    const auto& th = look::T();
    g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.55f : 0.28f));
    g.fillEllipse (p.x - r - 0.5f, p.y - r + 0.8f, r * 2.f + 1.f, r * 2.f + 1.f);
    juce::ColourGradient gr (juce::Colour (0xfff7f8fa), p.x - r * 0.7f, p.y - r * 0.8f, juce::Colour (0xff6b7079), p.x + r * 0.8f, p.y + r * 0.9f, false);
    gr.addColour (0.55, juce::Colour (0xffb9bec6));
    g.setGradientFill (gr);
    g.fillEllipse (p.x - r, p.y - r, r * 2.f, r * 2.f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (p.x - r, p.y - r, r * 2.f, r * 2.f, 0.7f);
    const float a = 0.35f + (float) (seed % 7) * 0.31f, s = std::sin (a) * r * 0.62f, c = std::cos (a) * r * 0.62f;
    g.setColour (juce::Colour (0xff2b2f36).withAlpha (0.85f));
    g.drawLine (p.x - c, p.y - s, p.x + c, p.y + s, 1.1f);
    g.drawLine (p.x + s, p.y - c, p.x - s, p.y + c, 1.1f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillEllipse (p.x - r * 0.55f, p.y - r * 0.7f, r * 0.5f, r * 0.35f);
}

// plaque de module : metal brosse (ou laque creme), biseau, quatre petites vis
void drawPlate (juce::Graphics& g, juce::Rectangle<float> b, bool screws = true)
{
    const auto& th = look::T();
    const auto r = b.reduced (1.f);
    g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.45f : 0.16f));                       // ombre portee
    g.fillRoundedRectangle (r.translated (0.f, 2.f), 5.f);
    g.setGradientFill (juce::ColourGradient (C (th.plateTop), r.getX(), r.getY(), C (th.plateBot), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.reduced (2.f).toNearestInt());
        juce::Random rr ((juce::int64) (r.getWidth() * 7 + r.getHeight()));
        for (float y = r.getY() + 2.f; y < r.getBottom() - 2.f; y += 2.f)                      // brossage horizontal
        {
            const bool light = rr.nextFloat() < 0.5f;
            g.setColour ((light ? juce::Colours::white : juce::Colours::black).withAlpha ((th.dark ? 0.018f : 0.035f) + 0.03f * rr.nextFloat()));
            g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (th.dark ? 0.05f : 0.22f), r.getX(), r.getY(),     // reflet doux en diagonale
                                                 juce::Colours::white.withAlpha (0.f), r.getX() + r.getWidth() * 0.45f, r.getBottom(), false));
        g.fillRect (r);
    }
    g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.10f : 0.75f));                      // biseau : arete claire en haut
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 5.f, r.getRight() - 5.f);
    g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.6f : 0.22f));
    g.drawRoundedRectangle (r, 5.f, 1.f);
    if (screws && r.getWidth() > 60.f && r.getHeight() > 40.f)
    {
        int k = 0;
        for (auto p : { juce::Point<float> (r.getX() + 8.f, r.getY() + 8.f), { r.getRight() - 8.f, r.getY() + 8.f },
                        { r.getX() + 8.f, r.getBottom() - 8.f }, { r.getRight() - 8.f, r.getBottom() - 8.f } })
            drawScrew (g, p, 3.4f, (int) (r.getX() + r.getY()) + k++);
    }
}

// voyant LED en relief
void drawJewel (juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour ledCol)
{
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (ledCol.withAlpha (0.55f), r.getCentreX(), r.getCentreY(), ledCol.withAlpha (0.f), r.getCentreX(), r.getY() - r.getHeight() * 0.9f, true));
        g.fillEllipse (r.expanded (r.getWidth() * 1.1f));
    }
    g.setGradientFill (juce::ColourGradient (on ? ledCol.brighter (0.8f) : ledCol.withSaturation (0.25f).withMultipliedBrightness (0.45f), r.getX() + r.getWidth() * 0.3f, r.getY() + r.getHeight() * 0.25f,
                                             on ? ledCol.darker (0.4f) : juce::Colour (0xff1a1d22), r.getRight(), r.getBottom(), true));
    g.fillEllipse (r);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (r, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.8f : 0.3f));
    g.fillEllipse (r.getX() + r.getWidth() * 0.22f, r.getY() + r.getHeight() * 0.14f, r.getWidth() * 0.34f, r.getHeight() * 0.26f);
}

// touche : en relief au repos, enfoncee et eclairee (couleur d'accent) quand elle est active
void drawKey (juce::Graphics& g, juce::Rectangle<float> r, bool pressed, bool hover = false)
{
    const auto& th = look::T();
    const juce::Colour hi (th.keyHi), lo (th.keyLo), ac (th.accent);
    if (! pressed)
    {
        g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.5f : 0.22f));
        g.fillRoundedRectangle (r.translated (0.f, 1.5f), 4.f);
        g.setGradientFill (juce::ColourGradient (hover ? hi.brighter (0.06f) : hi, r.getX(), r.getY(), lo, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.12f : 0.8f));
        g.drawHorizontalLine ((int) (r.getY() + 1.f), r.getX() + 4.f, r.getRight() - 4.f);
        g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.55f : 0.25f));
        g.drawRoundedRectangle (r, 4.f, 1.f);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (ac.brighter (0.35f), r.getX(), r.getY(), ac.darker (0.35f), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r.translated (0.f, 0.6f), 4.f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.3f), r.getX(), r.getY(), juce::Colours::black.withAlpha (0.f), r.getX(), r.getY() + 6.f, false));
        g.fillRoundedRectangle (r.translated (0.f, 0.6f), 4.f);                      // ombre interieure
        g.setColour (ac.darker (0.6f));
        g.drawRoundedRectangle (r.translated (0.f, 0.6f), 4.f, 1.f);
    }
}
juce::Colour keyText (bool on) { return on ? juce::Colours::white : look::print.withAlpha (0.92f); }

// ---------------------------------------------------------------- identite des modules FX : une couleur du spectre du prisme par module
// (ordre du rack par defaut : du rouge au violet, comme la lumiere qui sort du prisme de l'en-tete) + un grand pictogramme grave
juce::Colour moduleColour (int node)
{
    juce::uint32 c = 0;
    switch (node)
    {
        case zephyr::NSat:      c = 0xffff5a3c; break;   // rouge orange
        case zephyr::NColor:    c = 0xffff9a2e; break;   // orange
        case zephyr::NFilter:   c = 0xffffcf33; break;   // jaune
        case zephyr::NComp:     c = 0xff8fdc4a; break;   // vert tendre
        case zephyr::NPhaser:   c = 0xff2fd0a8; break;   // turquoise
        case zephyr::NEnsemble: c = 0xff35b9ff; break;   // bleu ciel
        case zephyr::NStut:     c = 0xff4f7bff; break;   // bleu
        case zephyr::NDelay:    c = 0xff7a5cff; break;   // indigo
        case zephyr::NVerb:     c = 0xffb35cff; break;   // violet
        case zephyr::NWide:     c = 0xffff5cc4; break;   // magenta
        default:                return juce::Colour (look::T().accent);
    }
    const juce::Colour col (c);
    return look::T().dark ? col : col.withMultipliedSaturation (1.05f).darker (0.18f);
}
const char* moduleCode (int node)
{
    switch (node)
    {
        case zephyr::NSat: return "Z-1  SAT"; case zephyr::NColor: return "Z-2  EQ"; case zephyr::NFilter: return "Z-3  VCF"; case zephyr::NComp: return "Z-4  OPTO";
        case zephyr::NPhaser: return "Z-5  PHASE"; case zephyr::NEnsemble: return "Z-6  STRINGS"; case zephyr::NStut: return "Z-7  REPEAT"; case zephyr::NDelay: return "Z-8  ECHO";
        case zephyr::NVerb: return "Z-9  PLATE"; case zephyr::NWide: return "Z-10  WIDE"; default: return "";
    }
}
// pictogramme du module dans r (trace au trait)
juce::Path moduleGlyph (int node, juce::Rectangle<float> r)
{
    juce::Path p;
    const float x0 = r.getX(), y0 = r.getY(), w = r.getWidth(), h = r.getHeight(), cy = r.getCentreY();
    auto curve = [&] (std::function<float (float)> f, float xa = 0.f, float xb = 1.f)
    {
        const int N = 64;
        for (int i = 0; i <= N; ++i)
        {
            const float t = xa + (xb - xa) * (float) i / (float) N;
            const juce::Point<float> q (x0 + t * w, cy - juce::jlimit (-1.f, 1.f, f (t)) * h * 0.5f);
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
    };
    switch (node)
    {
        case zephyr::NSat:      curve ([] (float t) { return std::tanh (2.8f * std::sin (t * 12.566f)) / std::tanh (2.8f) * 0.8f; }); break;          // sinus arrondi par la saturation
        case zephyr::NColor:    curve ([] (float t) { auto b = [t] (float c, float wd, float g) { return g * std::exp (-(t - c) * (t - c) / wd); };
                                                       return b (0.12f, 0.02f, 0.55f) + b (0.5f, 0.012f, -0.6f) + b (0.86f, 0.03f, 0.75f); }); break;      // trois bandes
        case zephyr::NFilter:   curve ([] (float t) { return 0.3f + 0.5f * std::exp (-(t - 0.55f) * (t - 0.55f) / 0.003f) - (t > 0.55f ? 9.f * (t - 0.55f) * (t - 0.55f) : 0.f); }, 0.f, 0.95f); break;   // passe-bas resonant
        case zephyr::NComp:     p.startNewSubPath (x0 + w * 0.2f, y0 + h);  p.lineTo (x0 + w * 0.55f, y0 + h * 0.35f);                                   // courbe de transfert avec coude
                                p.quadraticTo (x0 + w * 0.62f, y0 + h * 0.24f, x0 + w * 0.8f, y0 + h * 0.18f); p.lineTo (x0 + w, y0 + h * 0.08f);
                                p.startNewSubPath (x0 + w * 0.2f, y0 + h); p.lineTo (x0 + w * 0.2f, y0); break;
        case zephyr::NPhaser:   curve ([] (float t) { return 0.7f - 1.3f * std::pow (std::fabs (std::cos (t * 3.f * 3.14159f)), 14.f); }); break;   // creux du peigne
        case zephyr::NEnsemble: for (int k = 0; k < 3; ++k) curve ([k] (float t) { return 0.62f * std::sin (t * 12.566f + (float) k * 2.094f); }); break;  // trois lignes dephasees
        case zephyr::NStut:     for (int k = 0; k < 5; ++k) { const float bx = x0 + w * (0.02f + 0.2f * (float) k), bh = h * (k < 4 ? 0.8f : 0.4f);        // tranches repetees
                                                              p.addRoundedRectangle (bx, cy - bh * 0.5f, w * 0.15f, bh, 3.f); } break;
        case zephyr::NDelay:    for (int k = 0; k < 6; ++k) { const float bx = x0 + w * (0.04f + 0.16f * (float) k), bh = h * 0.95f * std::pow (0.72f, (float) k);   // echos qui s'eteignent
                                                              p.startNewSubPath (bx, cy - bh * 0.5f); p.lineTo (bx, cy + bh * 0.5f); } break;
        case zephyr::NVerb:     for (int k = 0; k < 5; ++k) { const float rr = h * (0.18f + 0.2f * (float) k);                                                        // ondes qui s'elargissent
                                                              p.addCentredArc (x0 + w * 0.12f, cy, rr, rr, 0.f, 0.5f, 2.64f, true); } break;
        case zephyr::NWide:     p.startNewSubPath (x0, cy); p.lineTo (x0 + w, cy);                                                                         // fleche stereo
                                p.startNewSubPath (x0 + w * 0.12f, cy - h * 0.3f); p.lineTo (x0, cy); p.lineTo (x0 + w * 0.12f, cy + h * 0.3f);
                                p.startNewSubPath (x0 + w * 0.88f, cy - h * 0.3f); p.lineTo (x0 + w, cy); p.lineTo (x0 + w * 0.88f, cy + h * 0.3f);
                                p.addEllipse (x0 + w * 0.5f - h * 0.22f, cy - h * 0.22f, h * 0.44f, h * 0.44f); break;
        default: break;
    }
    return p;
}

// potard : jupe moletee sombre, chapeau en aluminium tourne, repere lumineux ; angle < 0 : sans repere
void drawKnobBody (juce::Graphics& g, juce::Point<float> c, float kr, float angle, float alpha, bool pointer = true)
{
    const auto& th = look::T();
    for (int i = 0; i < 4; ++i)                                                       // ombre portee douce
    {
        const float e = (float) i * 1.2f;
        g.setColour (juce::Colours::black.withAlpha ((th.dark ? 0.16f : 0.08f) * alpha));
        g.fillEllipse (c.x - kr - e + 1.f, c.y - kr - e + 3.f + (float) i * 0.6f, (kr + e) * 2.f, (kr + e) * 2.f);
    }
    juce::ColourGradient sk (C (th.skirtHi), c.x - kr * 0.7f, c.y - kr * 0.9f, C (th.skirtLo), c.x + kr * 0.6f, c.y + kr, false);
    g.setGradientFill (sk);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
    for (int i = 0; i < 48; ++i)                                                      // moletage
    {
        const float a = (float) i / 48.f * juce::MathConstants<float>::twoPi, s = std::sin (a), co = std::cos (a);
        g.setColour (juce::Colours::white.withAlpha ((0.05f + 0.07f * std::max (0.f, -co * 0.6f - s * 0.4f)) * alpha));
        g.drawLine (c.x + kr * 0.80f * s, c.y - kr * 0.80f * co, c.x + kr * 0.98f * s, c.y - kr * 0.98f * co, 0.9f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.18f * alpha));
    juce::Path rim; rim.addCentredArc (c.x, c.y, kr - 0.6f, kr - 0.6f, 0.f, -2.4f, 0.5f, true);
    g.strokePath (rim, juce::PathStrokeType (1.f));
    const float cr = kr * 0.66f;
    g.setColour (juce::Colours::black.withAlpha (0.55f * alpha));
    g.fillEllipse (c.x - cr - 1.f, c.y - cr, cr * 2.f + 2.f, cr * 2.f + 2.f);
    juce::ColourGradient cap (C (th.capHi), c.x - cr * 0.7f, c.y - cr * 0.85f, C (th.capLo), c.x + cr * 0.8f, c.y + cr * 0.95f, false);
    cap.addColour (0.45, C (th.capMid));
    g.setGradientFill (cap);
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);
        g.reduceClipRegion (clip);
        for (int i = 1; i < 9; ++i)                                                   // tournage concentrique
        {
            const float r2 = cr * (float) i / 9.f;
            g.setColour ((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f));
            g.drawEllipse (c.x - r2, c.y - r2, r2 * 2.f, r2 * 2.f, 0.8f);
        }
        for (float a0 : { -1.2f, 1.94f })                                             // reflets anisotropes
        {
            juce::Path w; w.addPieSegment (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f, a0, a0 + 0.6f, 0.f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.02f), c.x, c.y, juce::Colours::white.withAlpha (0.5f * alpha), c.x + cr, c.y, true));
            g.fillPath (w);
        }
    }
    if (pointer)
    {
        const auto xf = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
        juce::Path p; p.addRoundedRectangle (-1.4f, -kr * 0.97f, 2.8f, kr * 0.36f, 1.2f);               // repere sur la jupe
        g.setColour (C (th.pointer).withAlpha (alpha));
        g.fillPath (p, xf);
        juce::Path d; d.addEllipse (-1.6f, -cr * 0.78f, 3.2f, 3.2f);                                     // point grave sur le chapeau
        g.setColour (C (th.accent).withAlpha (0.9f * alpha));
        g.fillPath (d, xf);
    }
}

// facade : aluminium brosse (ou laque) avec reflets, en-tete, cadre et vis si frame
void paintFacadeImpl (juce::Image& img, int W, int H, bool frame)
{
    const auto& th = look::T();
    img = juce::Image (juce::Image::RGB, juce::jmax (1, W), juce::jmax (1, H), false);
    juce::Graphics g (img);
    g.setGradientFill (juce::ColourGradient (C (th.panelTop), 0.f, 0.f, C (th.panelBot), 0.f, (float) H, false));
    g.fillAll();
    juce::Random rr (4242);
    for (int y = 0; y < H; ++y)                                                       // brossage horizontal fin
    {
        const bool light = rr.nextFloat() < 0.5f;
        g.setColour ((light ? juce::Colours::white : juce::Colours::black).withAlpha ((th.dark ? 0.012f : 0.03f) + (th.dark ? 0.015f : 0.035f) * rr.nextFloat()));
        g.drawHorizontalLine (y, 0.f, (float) W);
    }
    for (int k = 0; k < 3; ++k)                                                       // grandes bandes de lumiere obliques
    {
        const float x0 = (float) W * (0.15f + 0.33f * (float) k);
        juce::Path band;
        band.addQuadrilateral (x0, 0.f, x0 + 180.f, 0.f, x0 + 60.f, (float) H, x0 - 120.f, (float) H);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (th.dark ? 0.025f : 0.10f), x0, 0.f, juce::Colours::white.withAlpha (0.f), x0 + 180.f, 0.f, false));
        g.fillPath (band);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.45f, juce::Colours::black.withAlpha (th.dark ? 0.35f : 0.10f), 0.f, 0.f, true));
    g.fillAll();
    if (frame)
    {
        g.setColour (C (th.accent));                                                  // filet d'accent sous l'en-tete
        g.fillRect (14, kHeaderH - 3, W - 28, 2);
        g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.08f : 0.6f));
        g.fillRect (14, kHeaderH - 1, W - 28, 1);
        g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.6f : 0.25f));
        g.drawRect (0, 0, W, H, 1);
        int k = 0;
        for (auto p : { juce::Point<float> (14.f, 14.f), { (float) W - 14.f, 14.f }, { 14.f, (float) H - 14.f }, { (float) W - 14.f, (float) H - 14.f } })
            drawScrew (g, p, 5.f, k++ * 3);
    }
}

// pictogrammes d'ondes
bool drawGlyph (juce::Graphics& g, const juce::String& name, juce::Rectangle<float> r)
{
    juce::Path p;
    auto pt = [&r] (float x, float y) { return juce::Point<float> (r.getX() + x * r.getWidth(), r.getY() + y * r.getHeight()); };
    if (name == "SAW")                { p.startNewSubPath (pt (0.f, 1.f)); p.lineTo (pt (0.5f, 0.f)); p.lineTo (pt (0.5f, 1.f)); p.lineTo (pt (1.f, 0.f)); }
    else if (name == "PULSE" || name == "SQR") { p.startNewSubPath (pt (0.f, 1.f)); p.lineTo (pt (0.f, 0.f)); p.lineTo (pt (0.5f, 0.f)); p.lineTo (pt (0.5f, 1.f)); p.lineTo (pt (1.f, 1.f)); p.lineTo (pt (1.f, 0.f)); }
    else if (name == "TRI")           { p.startNewSubPath (pt (0.f, 0.5f)); p.lineTo (pt (0.25f, 0.f)); p.lineTo (pt (0.75f, 1.f)); p.lineTo (pt (1.f, 0.5f)); }
    else if (name == "SIN")
    {
        for (int i = 0; i <= 24; ++i)
        {
            const auto q = pt ((float) i / 24.f, 0.5f - 0.5f * std::sin ((float) i / 24.f * 6.2831853f));
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
    }
    else if (name == "S&H")
    {
        p.startNewSubPath (pt (0.f, 0.7f)); p.lineTo (pt (0.25f, 0.7f)); p.lineTo (pt (0.25f, 0.15f)); p.lineTo (pt (0.5f, 0.15f));
        p.lineTo (pt (0.5f, 0.9f)); p.lineTo (pt (0.75f, 0.9f)); p.lineTo (pt (0.75f, 0.4f)); p.lineTo (pt (1.f, 0.4f));
    }
    else return false;
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    return true;
}

juce::File themeFile() { return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Foudre Audio").getChildFile ("zephyr-theme.txt"); }
int loadThemePref() { const auto f = themeFile(); return f.existsAsFile() ? f.loadFileAsString().trim().getIntValue() : 0; }
void saveThemePref (int i) { const auto f = themeFile(); f.getParentDirectory().createDirectory(); f.replaceWithText (juce::String (i)); }

const int kTileNodes[] = { zephyr::NSat, zephyr::NColor, zephyr::NFilter, zephyr::NPhaser, zephyr::NComp, zephyr::NEnsemble, zephyr::NStut, zephyr::NDelay, zephyr::NVerb, zephyr::NWide };
const char* nodeName (int n)
{
    static const char* names[kNumNodes] { "INPUT", "STEREO WIDE", "SATURATION", "EQ", "FILTER", "PHASER", "COMP", "ENSEMBLE", "DELAY", "REVERB", "STUTTER", "OUTPUT" };
    return names[juce::jlimit (0, (int) kNumNodes - 1, n)];
}

const juce::Colour kCablePalette[6] { juce::Colour (0xff2f7fe0), juce::Colour (0xffe3a23a), juce::Colour (0xff47b8c8),
                                      juce::Colour (0xff6a5ad8), juce::Colour (0xffd8586a), juce::Colour (0xff58b06a) };

void drawCable (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour col, float alpha = 0.8f, float extra = 0.f)
{
    const float dx = juce::jmax (30.f, std::abs (b.x - a.x) * 0.4f), sag = 10.f + a.getDistanceFrom (b) * 0.08f;
    juce::Path p;
    p.startNewSubPath (a);
    p.cubicTo (a.x + dx, a.y + sag, b.x - dx, b.y + sag, b.x, b.y);
    g.setColour (juce::Colours::black.withAlpha (0.25f * alpha));
    g.strokePath (p, juce::PathStrokeType (4.4f + extra, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::translation (1.f, 3.f));
    g.setColour (col.darker (0.4f).withAlpha (alpha));
    g.strokePath (p, juce::PathStrokeType (4.f + extra, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col.withAlpha (alpha));
    g.strokePath (p, juce::PathStrokeType (2.6f + extra, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    for (auto c : { a, b })
    {
        g.setColour (juce::Colour (0xff141922).withAlpha (alpha));
        g.fillEllipse (c.x - 8.f, c.y - 8.f, 16.f, 16.f);
        g.setColour (col.withAlpha (alpha));
        g.drawEllipse (c.x - 6.f, c.y - 6.f, 12.f, 12.f, 1.8f);
    }
}

} // namespace

void paintFacade (juce::Image& img, int W, int H, bool frame) { paintFacadeImpl (img, W, H, frame); }

// ================================================================ look & feel
namespace { std::vector<ZephyrLookAndFeel*>& lafList() { static std::vector<ZephyrLookAndFeel*> v; return v; } }

ZephyrLookAndFeel::ZephyrLookAndFeel() { lafList().push_back (this); refresh(); }
ZephyrLookAndFeel::~ZephyrLookAndFeel() { auto& v = lafList(); v.erase (std::remove (v.begin(), v.end(), this), v.end()); }
void ZephyrLookAndFeel::refreshAll() { for (auto* l : lafList()) l->refresh(); }

void ZephyrLookAndFeel::refresh()
{
    const auto& th = look::T();
    const juce::Colour acc (th.accent), scr (th.screenBg), txt (th.screenText);
    setColour (juce::ComboBox::backgroundColourId, scr);
    setColour (juce::ComboBox::textColourId, txt);
    setColour (juce::ComboBox::outlineColourId, acc.withAlpha (0.55f));
    setColour (juce::ComboBox::arrowColourId, txt);
    setColour (juce::PopupMenu::backgroundColourId, scr);
    setColour (juce::PopupMenu::textColourId, txt);
    setColour (juce::PopupMenu::headerTextColourId, acc.brighter (0.5f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, acc);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, juce::Colour (th.keyLo));
    setColour (juce::TextButton::buttonOnColourId, acc);
    setColour (juce::TextButton::textColourOffId, look::print);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ScrollBar::thumbColourId, acc.withAlpha (0.6f));
    setColour (juce::AlertWindow::backgroundColourId, scr);
    setColour (juce::AlertWindow::textColourId, txt);
    setColour (juce::AlertWindow::outlineColourId, acc.withAlpha (0.6f));
    setColour (juce::TextEditor::backgroundColourId, scr);
    setColour (juce::TextEditor::textColourId, txt);
    setColour (juce::TextEditor::outlineColourId, acc.withAlpha (0.6f));
    setColour (juce::TextEditor::focusedOutlineColourId, acc);
    setColour (juce::Slider::thumbColourId, juce::Colour (th.capHi));
    setColour (juce::Slider::trackColourId, acc.withAlpha (0.8f));
    setColour (juce::Slider::backgroundColourId, scr);
    setColour (juce::BubbleComponent::backgroundColourId, scr);
    setColour (juce::BubbleComponent::outlineColourId, acc);
    setColour (juce::TooltipWindow::textColourId, txt);
}

void ZephyrLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    drawKey (g, b.getLocalBounds().toFloat().reduced (1.5f), down || b.getToggleState(), over);
    if (b.getProperties().contains ("modCol"))                                            // tuile de module : sa couleur du spectre
    {
        const juce::Colour mc ((juce::uint32) (juce::int64) b.getProperties()["modCol"]);
        const auto r = b.getLocalBounds().toFloat().reduced (1.5f);
        const juce::Rectangle<float> led (r.getX() + 6.f, r.getCentreY() - 3.5f, 7.f, 7.f);
        if (b.getToggleState()) { g.setColour (mc.withAlpha (0.45f)); g.fillEllipse (led.expanded (2.5f)); }
        g.setColour (b.getToggleState() ? mc.brighter (0.25f) : mc.withAlpha (0.35f)); g.fillEllipse (led);
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawEllipse (led, 0.7f);
    }
}

void ZephyrLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setFont (font (juce::jmin (12.f, (float) b.getHeight() * 0.5f + 2.f)));
    g.setColour (keyText (b.getToggleState() || down));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, down ? 1 : 0), juce::Justification::centred, false);
}

void ZephyrLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto& th = look::T();
    const auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (1.f);
    g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.06f : 0.6f));                  // lunette
    g.drawRoundedRectangle (r.translated (0.f, 1.f), 4.f, 1.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (th.screenBg), r.getX(), r.getY(), juce::Colour (th.screenBg).brighter (0.25f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (juce::Colour (th.accent).withAlpha (box.isEnabled() ? 0.55f : 0.25f));
    g.drawRoundedRectangle (r, 4.f, 1.f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), r.getX(), r.getY(), juce::Colours::white.withAlpha (0.f), r.getX(), r.getCentreY(), false));
    g.fillRoundedRectangle (r.reduced (1.f), 3.f);                                         // reflet sur le verre de l'afficheur
    juce::Path chev;
    const float cx = (float) w - 14.f, cy = (float) h * 0.5f;
    chev.startNewSubPath (cx - 4.f, cy - 2.f); chev.lineTo (cx, cy + 2.5f); chev.lineTo (cx + 4.f, cy - 2.f);
    g.setColour (juce::Colour (th.screenText).withAlpha (0.8f));
    g.strokePath (chev, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ZephyrLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const bool numbered = (bool) s.getProperties()["numbered"];
    const float r = (float) juce::jmin (w, h) * 0.5f, kr = r * (numbered ? 0.58f : 0.66f);
    const float angle = startAngle + pos * (endAngle - startAngle);
    if (s.isEnabled())                                                                     // arc lumineux de la valeur (depuis le cran)
    {
        const auto& th = look::T();
        const DetentSlider* ds = dynamic_cast<const DetentSlider*> (&s);
        const bool bip = ds != nullptr && ds->detent >= s.getMinimum() && ds->detent <= s.getMaximum();
        const float a0 = bip ? startAngle + (float) s.valueToProportionOfLength (ds->detent) * (endAngle - startAngle) : startAngle;
        if (std::abs (angle - a0) > 0.01f)
        {
            juce::Path arc; arc.addCentredArc (c.x, c.y, kr + 4.5f, kr + 4.5f, 0.f, juce::jmin (a0, angle), juce::jmax (a0, angle), true);
            g.setColour (juce::Colour (th.accent).withAlpha (0.85f));
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    drawKnobBody (g, c, kr, angle, s.isEnabled() ? 1.f : 0.5f);
}

// ================================================================ potard
KnobUnit::KnobUnit (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& t,
                    const juce::String& lo, const juce::String& hi, int cvIdx)
    : text (t), loText (lo), hiText (hi), cvIndex (cvIdx), attachment (apvts, id, slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setScrollWheelEnabled (false);                                   // la molette fait defiler le rack
    slider.getProperties().set ("seed", id.hashCode());
    if (auto* prm = apvts.getParameter (id))
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0) slider.detent = 0.0;           // cran au zero des reglages +/-
    if (slider.getMinimum() == 0.0 && slider.getMaximum() == 1.0)
        slider.textFromValueFunction = [] (double v) { return juce::String (v * 10.0, 1); };
    addAndMakeVisible (slider);
#if ZEPHYR_SYNTH
    paramId = id;
    proc = dynamic_cast<ZephyrAudioProcessor*> (&apvts.processor);
    if (proc != nullptr)
    {
        mapIdx = proc->mapIndex (id); modDst = ZephyrAudioProcessor::modDstOfParam (id);
        slider.addMouseListener (this, false); startTimerHz (6);
        if (modDst > 0) slider.intercept = [this] (const juce::MouseEvent& ev, int ph) { return modInterceptor (ev, ph); };
    }
#endif
}

void KnobUnit::timerCallback()
{
#if ZEPHYR_SYNTH
    if (proc == nullptr) return;
    proc->flushMidiMapIfDirty();
    if (modDst > 0)                                                            // anneaux : on suit la matrice et les sources vivantes (macros, molette)
    {
        ZephyrAudioProcessor::KnobMod km[2]; const int n = proc->knobMods (modDst, km);
        int hs = n * 977 + (int) (slider.getValue() * 1000.0);
        for (int k = 0; k < n; ++k) hs += km[k].src * 131 * (k + 1) + (int) (km[k].amt * 1000.f) + (int) (proc->modSourceLive (km[k].src) * 1000.f);
        if (hs != hash || n != nMods) { hash = hs; nMods = n; repaint(); }
        const int wantMs = n > 0 ? 40 : 160;
        if (getTimerInterval() != wantMs) startTimer (wantMs);
    }
    const int cc = proc->ccAt (mapIdx); const bool lr = proc->learningAt (mapIdx);
    if (cc != shownCc || lr != shownLearn || look::showMidi != shownShow) { shownCc = cc; shownLearn = lr; shownShow = look::showMidi; repaint(); }
#endif
}

void KnobUnit::mouseDown (const juce::MouseEvent& e)
{
#if ZEPHYR_SYNTH
    if (! e.mods.isPopupMenu() || proc == nullptr || (mapIdx < 0 && modDst <= 0)) return;
    const int cc = mapIdx >= 0 ? proc->ccAt (mapIdx) : -1, dflt = mapIdx >= 0 ? proc->defaultCcOf (paramId) : -1, mode = proc->takeoverMode.load();
    const bool learning = mapIdx >= 0 && proc->learningAt (mapIdx);
    juce::PopupMenu m;
    if (modDst > 0)                                                            // modulation : sources classees par categorie, 2 par potard
    {
        ZephyrAudioProcessor::KnobMod km[2]; const int n = proc->knobMods (modDst, km);
        auto has = [&] (int s) { for (int k = 0; k < n; ++k) if (km[k].src == s) return true; return false; };
        auto fam = [&] (const char* title, std::vector<int> srcs)
        {
            juce::PopupMenu sub;
            for (int s : srcs) sub.addItem (100 + s, kModSrcNames[s], n < 2 || has (s), has (s));
            m.addSubMenu (juce::String::fromUTF8 (title), sub);
        };
        m.addSectionHeader (juce::String::fromUTF8 ("Modulation de ce potard (2 sources max)"));
        fam ("Assigner \xc3\xa0  >  LFO", { MsLfo1, MsLfo2, MsLfo3, MsLfo4 });
        fam ("Assigner \xc3\xa0  >  MACROS", { MsMac1, MsMac2, MsMac3, MsMac4, MsMac5, MsMac6, MsMac7, MsMac8 });
        fam ("Assigner \xc3\xa0  >  ENVELOPPES", { MsEnv, MsAmpEnv });
        fam ("Assigner \xc3\xa0  >  JEU", { MsVel, MsWheel, MsChanAT, MsPolyAT, MsKey, MsRandom });
        fam ("Assigner \xc3\xa0  >  PISTES ARP", { MsArp1, MsArp2, MsArp3, MsArp4, MsArp5, MsArp6, MsArp7, MsArp8 });
        for (int k = 0; k < n; ++k)
            m.addItem (200 + km[k].slot, juce::String::fromUTF8 ("Supprimer : ") + kModSrcNames[km[k].src] + "  (" + juce::String (juce::roundToInt (km[k].amt * 100.f)) + " %)");
        if (n >= 2) m.addItem (999, juce::String::fromUTF8 ("2 sources d\xc3\xa9j\xc3\xa0 : supprime-en une pour en changer"), false);
        if (n > 0) m.addItem (998, juce::String::fromUTF8 ("Ctrl + clic gauche et glisser sur le potard : r\xc3\xa9gler l'anneau"), false);
        if (mapIdx >= 0) m.addSeparator();
    }
    if (mapIdx >= 0) {
    m.addItem (1, learning ? "Annuler l'apprentissage MIDI" : juce::String (cc >= 0 ? "R\xc3\xa9" "assigner MIDI" : "Assigner MIDI") + " (bouge un potard du clavier)" + (cc >= 0 ? "  [CC " + juce::String (cc) + "]" : juce::String()));
    m.addItem (2, juce::String ("Supprimer l'assignation MIDI") + (cc >= 0 ? "  [CC " + juce::String (cc) + "]" : juce::String()), cc >= 0);
    if (dflt >= 0) m.addItem (3, "Remettre le CC d'origine (" + juce::String (dflt) + ")", cc != dflt);
    m.addSeparator();
    m.addSectionHeader (juce::String::fromUTF8 ("Reprise de valeur (tous les potards)"));
    m.addItem (11, juce::String::fromUTF8 ("SCALE : rejoint le potard sans saut (conseill\xc3\xa9)"), true, mode == 0);
    m.addItem (12, juce::String::fromUTF8 ("PICKUP : attend que le potard croise la valeur"), true, mode == 1);
    m.addItem (13, juce::String::fromUTF8 ("DIRECT : saute \xc3\xa0 la position du potard"), true, mode == 2);
    }
    auto* pr = proc; const auto id = paramId; const int dst = modDst;
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [pr, id, learning, dst] (int r)
    {
        if (r >= 100 && r < 200) pr->assignKnobMod (dst, r - 100);
        else if (r >= 200 && r < 300) pr->clearKnobMod (r - 200);
        else if (r == 1) { if (learning) pr->cancelMidiLearn(); else pr->startMidiLearn (id); }
        else if (r == 2) pr->clearMidiAssign (id);
        else if (r == 3) pr->defaultMidiAssign (id);
        else if (r >= 11 && r <= 13) { pr->takeoverMode.store (r - 11); pr->markMidiMapDirty(); }
    });
#else
    juce::ignoreUnused (e);
#endif
}

void KnobUnit::resized()
{
    auto b = getLocalBounds().withTrimmedBottom (16);
    if (! choices.isEmpty()) b = b.withSizeKeepingCentre ((int) (juce::jmin (b.getWidth(), b.getHeight()) * 0.74f), (int) (juce::jmin (b.getWidth(), b.getHeight()) * 0.74f));   // potard cranté : place pour les libellés autour
    slider.setBounds (b);
}

void KnobUnit::setCvActive (bool a) { if (a != cvActive) { cvActive = a; repaint(); } }

#if ZEPHYR_SYNTH
static const juce::Colour kRingCol[2] { juce::Colour (0xff3d9bff), juce::Colour (0xff3fe0c0) };
// Ctrl + clic gauche + glisser : regle la quantite d'une source (anneau). Zone exterieure du potard = anneau 1, centre = anneau 2.
bool KnobUnit::modInterceptor (const juce::MouseEvent& e, int phase)
{
    if (proc == nullptr || modDst <= 0) return false;
    if (phase == 0)
    {
        if (! e.mods.isCtrlDown() || e.mods.isPopupMenu()) return false;
        ZephyrAudioProcessor::KnobMod km[2]; const int n = proc->knobMods (modDst, km);
        if (n == 0) return true;                                                   // rien a regler : on ne tourne pas le potard par accident
        int k = 0;
        if (n == 2)
        {
            const auto c = slider.getLocalBounds().toFloat().getCentre();
            const float r = juce::jmin ((float) slider.getWidth(), (float) slider.getHeight()) * 0.5f;
            k = e.position.getDistanceFrom (c) >= 0.62f * r ? 0 : 1;
        }
        ringSlot = km[k].slot; ringStartAmt = km[k].amt; dragStartY = e.position.y;
        dragText = juce::String (kModSrcNames[km[k].src]);
        return true;
    }
    if (ringSlot < 0) return true;
    if (phase == 1)
    {
        const float amt = juce::jlimit (-1.f, 1.f, ringStartAmt + (dragStartY - e.position.y) / 150.f);
        proc->setKnobModAmt (ringSlot, amt);
        dragText = dragText.upToFirstOccurrenceOf ("  ", false, false) + "  " + juce::String (juce::roundToInt (amt * 100.f)) + " %";
        repaint();
    }
    else { ringSlot = -1; dragText = {}; repaint(); }
    return true;
}

void KnobUnit::paintOverChildren (juce::Graphics& g)
{
    if (proc == nullptr || modDst <= 0) return;
    ZephyrAudioProcessor::KnobMod km[2]; const int n = proc->knobMods (modDst, km);
    if (n == 0) return;
    const auto sb = slider.getBounds().toFloat();
    const auto c = sb.getCentre();
    const float r = juce::jmin (sb.getWidth(), sb.getHeight()) * 0.5f;
    const float sa = juce::MathConstants<float>::pi * 1.2f, ea = juce::MathConstants<float>::pi * 2.8f;
    const float base = (float) slider.valueToProportionOfLength (slider.getValue());
    const bool numbered = (bool) slider.getProperties()["numbered"] && sb.getWidth() >= 58.f;
    const float sw = juce::jlimit (1.5f, 2.2f, r * 0.047f);
    for (int k = 0; k < n; ++k)
    {
        const float rad = r * (numbered ? (k == 0 ? 0.705f : 0.64f) : (k == 0 ? 0.82f : 0.74f));       // entre le corps du potard et les graduations : jamais sur les chiffres ni sur les bords
        const float sc = ZephyrAudioProcessor::modScaleOfDst (modDst), span = km[k].amt * sc;
        const float end = juce::jlimit (0.f, 1.f, base + span);
        const float a0 = sa + juce::jmin (base, end) * (ea - sa), a1 = sa + juce::jmax (base, end) * (ea - sa);
        juce::Path p; p.addCentredArc (c.x, c.y, rad, rad, 0.f, a0, juce::jmax (a1, a0 + 0.02f), true);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.strokePath (p, juce::PathStrokeType (sw + 1.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (kRingCol[k]); g.strokePath (p, juce::PathStrokeType (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float live = proc->modSourceLive (km[k].src);                        // point vivant : ou le reglage est reellement en ce moment
        if (live > -50.f)
        {
            const float pos = juce::jlimit (0.f, 1.f, base + span * live), a = sa + pos * (ea - sa);
            g.setColour (juce::Colours::white); g.fillEllipse (c.x + rad * std::sin (a) - 2.6f, c.y - rad * std::cos (a) - 2.6f, 5.2f, 5.2f);
            g.setColour (kRingCol[k]);           g.drawEllipse (c.x + rad * std::sin (a) - 2.6f, c.y - rad * std::cos (a) - 2.6f, 5.2f, 5.2f, 1.3f);
        }
    }
    if (dragText.isNotEmpty())
    {
        const auto box = juce::Rectangle<float> (c.x - 46.f, c.y - 9.f, 92.f, 18.f);
        g.setColour (juce::Colours::black.withAlpha (0.82f)); g.fillRoundedRectangle (box, 4.f);
        g.setColour (juce::Colours::white); g.setFont (font (10.5f)); g.drawText (dragText, box.toNearestInt(), juce::Justification::centred, false);
    }
}
#else
void KnobUnit::paintOverChildren (juce::Graphics&) {}
bool KnobUnit::modInterceptor (const juce::MouseEvent&, int) { return false; }
#endif

void KnobUnit::setChoices (const juce::StringArray& names)
{
    choices = names;
    resized();
    slider.getProperties().set ("numbered", false);
    slider.textFromValueFunction = [this] (double v) { return choices[juce::jlimit (0, juce::jmax (0, choices.size() - 1), juce::roundToInt (v))]; };
    slider.onValueChange = [this] { repaint(); };
    slider.setPopupDisplayEnabled (true, true, nullptr);
    repaint();
}

void KnobUnit::paint (juce::Graphics& g)
{
    const auto sb = slider.getBounds().toFloat();
    const auto c = sb.getCentre();
    const float r = juce::jmin (sb.getWidth(), sb.getHeight()) * 0.5f;
    const float sa = juce::MathConstants<float>::pi * 1.2f, ea = juce::MathConstants<float>::pi * 2.8f;
    const float alpha = slider.isEnabled() ? 1.f : 0.45f;
    const bool numbered = (bool) slider.getProperties()["numbered"] && sb.getWidth() >= 58.f;
    if (! choices.isEmpty())                                                       // potard cranté : graduations nommées, la valeur active en clair
    {
        const int n = choices.size(), cur = juce::jlimit (0, n - 1, juce::roundToInt (slider.getValue()));
        g.setFont (font (8.8f));
        for (int i = 0; i < n; ++i)
        {
            const float a = sa + (n > 1 ? (float) i / (float) (n - 1) : 0.5f) * (ea - sa);
            g.setColour ((i == cur ? look::cvRing : look::print).withAlpha ((i == cur ? 1.f : 0.85f) * alpha));
            g.fillEllipse (c.x + r * 0.74f * std::sin (a) - 1.8f, c.y - r * 0.74f * std::cos (a) - 1.8f, 3.6f, 3.6f);
            g.drawText (choices[i], (int) (c.x + r * (0.98f + 0.04f * std::abs (std::sin (a))) * std::sin (a)) - 24, (int) (c.y - r * 1.0f * std::cos (a)) - 6, 48, 12, juce::Justification::centred, false);
        }
    }
    else
    for (int i = 0; i <= 10; ++i)
    {
        const float a = sa + (float) i / 10.f * (ea - sa), rad = r * (numbered ? 0.755f : 0.93f), dot = (i % 5 == 0) ? 3.2f : 2.f;
        g.setColour (look::print.withAlpha (0.88f * alpha));
        g.fillEllipse (c.x + rad * std::sin (a) - dot * 0.5f, c.y - rad * std::cos (a) - dot * 0.5f, dot, dot);
        if (numbered)                                                              // chiffres 0..10 autour du bouton, comme le MS20
        {
            g.setFont (font (8.6f));                                               // chiffres un peu plus pres du potard : ils ne touchent plus ceux du potard voisin
            g.drawText (juce::String (i), (int) (c.x + r * 0.88f * std::sin (a)) - 7, (int) (c.y - r * 0.88f * std::cos (a)) - 5, 14, 10,
                        juce::Justification::centred, false);
        }
    }
    if (! numbered && choices.isEmpty() && ! (loText == "0" && hiText == "10"))
    {
        g.setFont (font (9.5f));
        g.setColour (look::print.withAlpha (0.8f * alpha));
        g.drawText (loText, (int) (c.x - r * 0.78f) - 14, (int) (c.y + r * 0.88f), 28, 11, juce::Justification::centred, false);
        g.drawText (hiText, (int) (c.x + r * 0.78f) - 14, (int) (c.y + r * 0.88f), 28, 11, juce::Justification::centred, false);
    }
    if (slider.detent >= slider.getMinimum() && slider.detent <= slider.getMaximum())               // repere orange du cran
    {
        const float a = sa + (float) slider.valueToProportionOfLength (slider.detent) * (ea - sa), rad = r * (numbered ? 0.74f : 0.93f);
        g.setColour (look::cvRing.withAlpha (0.95f * alpha));
        g.fillEllipse (c.x + rad * std::sin (a) - 2.f, c.y - rad * std::cos (a) - 2.f, 4.f, 4.f);
    }
    g.setFont (font (getWidth() < 80 ? 11.f : 12.f));
    g.setColour (look::print.withAlpha (0.95f * alpha));
    g.drawText (text, -4, getHeight() - 15, getWidth() + 8, 14, juce::Justification::centred, false);
#if ZEPHYR_SYNTH
    if (shownLearn || (look::showMidi && shownCc >= 0))                                              // assignation MIDI : petite etiquette orange dans le coin
    {
        const auto tag = shownLearn ? juce::String ("LEARN") : "CC" + juce::String (shownCc);
        g.setFont (font (8.5f));
        const int tw = (int) font (8.5f).getStringWidth (tag) + 8, tx = getWidth() - tw - 1;
        g.setColour (juce::Colour (0xffe0662a).withAlpha (shownLearn ? 0.95f : 0.8f));
        g.fillRoundedRectangle ((float) tx, 1.f, (float) tw, 10.f, 3.f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawText (tag, tx, 1, tw, 10, juce::Justification::centred, false);
    }
#endif
    if (cvActive)                                                            // un cable CV est branche sur ce reglage
    {
        g.setColour (look::cvRing.withAlpha (0.3f)); g.fillEllipse ((float) getWidth() - 17.f, 0.f, 15.f, 15.f);
        g.setColour (look::cvRing);                  g.fillEllipse ((float) getWidth() - 14.f, 3.f, 9.f, 9.f);
    }
}

// ================================================================ selecteur / interrupteurs
SegmentSelector::SegmentSelector (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, juce::StringArray names)
    : labels (std::move (names)),
      attachment (*apvts.getParameter (id), [this] (float v) { selected = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void SegmentSelector::paint (juce::Graphics& g)
{
    const int n = labels.size();
    const float w = (float) getWidth() / (float) juce::jmax (1, n);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (w * (float) i, 0.f, w, (float) getHeight()).reduced (2.f);
        const bool on = (i == selected);
        drawKey (g, r, on);
        drawJewel (g, juce::Rectangle<float> (6.f, 6.f).withCentre ({ r.getCentreX(), r.getY() + 6.5f }), on, juce::Colour (look::T().led));
        g.setColour (keyText (on));
        const auto glyphArea = r.withTrimmedTop (13.f).withTrimmedBottom (r.getHeight() > 36.f ? 11.f : 0.f).reduced (7.f, 2.f);
        if (r.getHeight() > 36.f && drawGlyph (g, labels[i], glyphArea))
        {
            g.setFont (font (8.f));
            g.drawText (labels[i], r.withTrimmedTop (r.getHeight() - 12.f), juce::Justification::centred, false);
        }
        else
        {
            g.setFont (font (9.5f));
            g.drawText (labels[i], r.withTrimmedTop (10.f), juce::Justification::centred, false);
        }
    }
}

void SegmentSelector::mouseDown (const juce::MouseEvent& e)
{
    const int n = labels.size();
    attachment.setValueAsCompleteGesture ((float) juce::jlimit (0, n - 1, (int) (e.position.x / ((float) getWidth() / (float) n))));
}

void LedToggle::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.f);
    const bool on = getToggleState();
    drawKey (g, r, on || down, over);
    drawJewel (g, juce::Rectangle<float> (7.f, 7.f).withCentre ({ r.getX() + 12.f, r.getCentreY() }), on, juce::Colour (look::T().led));
    g.setColour (keyText (on));
    g.setFont (font (10.f));
    g.drawText (getButtonText(), r.withTrimmedLeft (24.f), juce::Justification::centredLeft, false);
}

void FootSwitch::paintButton (juce::Graphics& g, bool, bool down)
{
    auto b = getLocalBounds().toFloat();
    const bool on = getToggleState();
    drawJewel (g, juce::Rectangle<float> (12.f, 12.f).withCentre ({ b.getCentreX(), b.getY() + 8.f }), on, juce::Colour (look::T().led));
    const float d = juce::jmin (b.getWidth() - 8.f, b.getHeight() - 22.f) * (down ? 0.95f : 1.f);
    const juce::Point<float> c (b.getCentreX(), b.getY() + 20.f + (b.getHeight() - 20.f) * 0.5f);
    drawKnobBody (g, c, d * 0.5f, 0.f, 1.f, false);
    g.setColour (juce::Colour (0xff1a1816));
    g.setFont (font (11.f));
    g.drawText ("ON", juce::Rectangle<float> (d, d).withCentre (c), juce::Justification::centred, false);
}

// ================================================================ vumeter a aiguille (patine, papier jauni)
void VuMeter::tick()
{
    shown += (target > shown ? 0.45f : 0.12f) * (target - shown);          // balistique : montee rapide, retombee lente
    if (std::fabs (shown - lastDrawn) > 0.03f) { lastDrawn = shown; repaint(); }
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.f);
    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip; clip.addRoundedRectangle (b, 4.f);
    g.reduceClipRegion (clip);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffddd3ab), b.getX(), b.getY(), juce::Colour (0xff9a8e66), b.getX(), b.getBottom(), false));
    g.fillRect (b);
    juce::Random rr (77);
    for (int k = 0; k < 70; ++k)                                                // taches de vieillissement
    {
        g.setColour (juce::Colour (0xff5a4a28).withAlpha (0.03f + 0.07f * rr.nextFloat()));
        const float s = 2.f + rr.nextFloat() * 7.f;
        g.fillEllipse (b.getX() + rr.nextFloat() * b.getWidth(), b.getY() + rr.nextFloat() * b.getHeight(), s, s);
    }

    const juce::Point<float> pivot (b.getCentreX(), b.getBottom() + 14.f);
    const float R = b.getHeight() * 0.98f + 6.f, a0 = -0.62f, a1 = 0.62f;
    auto dir = [] (float a) { return juce::Point<float> (std::sin (a), -std::cos (a)); };

    for (int i = 0; i <= 20; ++i)                                               // graduations 0..20 dB
    {
        const float a = a0 + (a1 - a0) * (float) i / 20.f, len = (i % 5 == 0) ? 9.f : 5.f;
        const auto p1 = pivot + dir (a) * (R - len), p2 = pivot + dir (a) * R;
        g.setColour (i >= 15 ? juce::Colour (0xffa81c10) : juce::Colour (0xff2a2418));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, (i % 5 == 0) ? 1.6f : 1.f);
        if (i % 5 == 0)
        {
            const auto t = pivot + dir (a) * (R - 19.f);
            g.setFont (font (9.f));
            g.drawText (juce::String (i), (int) t.x - 10, (int) t.y - 6, 20, 12, juce::Justification::centred, false);
        }
    }
    g.setColour (juce::Colour (0xff2a2418));
    g.setFont (font (8.5f));
    g.drawText ("GAIN REDUCTION", b.withTrimmedTop (3.f).withHeight (12.f), juce::Justification::centred, false);
    g.drawText ("dB", b.withTrimmedLeft (6.f).withTrimmedTop (b.getHeight() - 15.f), juce::Justification::centredLeft, false);

    const float v = juce::jlimit (0.f, 21.f, shown), a = a0 + (a1 - a0) * v / 20.f;
    const auto tip = pivot + dir (a) * (R - 3.f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));                        // ombre de l'aiguille
    g.drawLine (pivot.x + 2.f, pivot.y + 1.f, tip.x + 2.f, tip.y + 1.f, 1.6f);
    g.setColour (juce::Colour (0xff101010));
    g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 1.6f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.20f), b.getX(), b.getY(),      // reflet du verre
                                             juce::Colours::white.withAlpha (0.f), b.getX() + b.getWidth() * 0.6f, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (juce::Colour (0xff1b1b1d));
    g.drawRoundedRectangle (b, 4.f, 3.f);
}

void LedLamp::paint (juce::Graphics& g)
{
    const auto led = juce::Rectangle<float> (14.f, 14.f).withCentre ({ (float) getWidth() * 0.5f, 10.f });
    if (lit) { g.setColour (juce::Colour (0xff5be36b).withAlpha (0.35f)); g.fillEllipse (led.expanded (5.f)); }
    g.setColour (lit ? juce::Colour (0xff6bff7a) : juce::Colour (0xff10260f));
    g.fillEllipse (led);
    g.setColour (look::print.withAlpha (0.6f));
    g.drawEllipse (led, 1.f);
    g.setColour (look::print.withAlpha (0.9f));
    g.setFont (font (9.f));
    g.drawText (text, 0, 22, getWidth(), 12, juce::Justification::centred, false);
}

// ================================================================ prise jack
Jack::Jack (PatchBay& o, JackKind k, int i) : kind (k), index (i), owner (o) { setSize (28, 28); }

juce::Point<float> Jack::socketCentre() const { return owner.centreOf (*this); }

void Jack::paint (juce::Graphics& g)
{
    const juce::Rectangle<float> s (3.f, 3.f, 22.f, 22.f);
    const bool cv = kind == JackKind::CvIn || kind == JackKind::CvOut;
    g.setColour (cv ? look::cvRing : look::print.withAlpha (0.9f));                       // anneau serigraphie
    g.drawEllipse (s.expanded (2.5f), 1.6f);
    if (highlight) { g.setColour (juce::Colour (0xff7be37b).withAlpha (0.4f)); g.fillEllipse (s.expanded (5.f)); }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffbcbcc0), s.getX(), s.getY(), juce::Colour (0xff4a4a50), s.getRight(), s.getBottom(), false));
    g.fillEllipse (s.reduced (2.f));
    g.setColour (juce::Colour (0xff26262a));
    g.drawEllipse (s.reduced (2.f), 1.f);
    juce::Random rr (index * 17 + (int) kind * 101);
    for (int k = 0; k < 7; ++k)
    {
        g.setColour (juce::Colour (0xff2c2c30).withAlpha (0.5f));
        g.fillRect (s.getX() + 4.f + rr.nextFloat() * 14.f, s.getY() + 4.f + rr.nextFloat() * 14.f, 1.5f, 1.5f);
    }
    g.setColour (juce::Colour (0xff050506));
    g.fillEllipse (s.reduced (7.5f));
    g.setColour (juce::Colour (0xff9a9aa0).withAlpha (0.5f));
    g.drawEllipse (s.reduced (7.5f), 1.f);
    if (kind == JackKind::CvOut)                                                          // voyant d'activite
    {
        const float v = owner.proc.engine.modLed[index].load();
        g.setColour (juce::Colour (0xffff3a1a).withAlpha (0.12f + 0.88f * v));
        g.fillEllipse (1.f, 1.f, 6.f, 6.f);
    }
}

void Jack::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { owner.clearJack (*this); return; }
    owner.beginDrag (*this);
}
void Jack::mouseDrag (const juce::MouseEvent& e) { owner.dragMoved (e.getEventRelativeTo (&owner).getPosition()); }
void Jack::mouseUp (const juce::MouseEvent& e)   { owner.endDrag (e.getEventRelativeTo (&owner).getPosition()); }
void Jack::mouseDoubleClick (const juce::MouseEvent&) { owner.clearJack (*this); }
void Jack::mouseEnter (const juce::MouseEvent&) { owner.hover (this); }
void Jack::mouseExit (const juce::MouseEvent&)  { owner.hover (nullptr); }

// ================================================================ calque des cables
PatchBay::PatchBay (ZephyrAudioProcessor& p) : proc (p)
{
    setInterceptsMouseClicks (false, false);
}

Jack* PatchBay::makeJack (juce::Component& parent, JackKind k, int index)
{
    jacks.push_back (std::make_unique<Jack> (*this, k, index));
    auto* j = jacks.back().get();
    parent.addAndMakeVisible (*j);
    switch (k)
    {
        case JackKind::AudioOut: audioOutJ[index] = j; break;
        case JackKind::AudioIn:  audioInJ[index] = j; break;
        case JackKind::CvOut:    cvOutJ[index] = j; break;
        case JackKind::CvIn:     cvInJ[index] = j; break;
    }
    return j;
}

juce::Point<float> PatchBay::centreOf (const Jack& j) const
{
    return getLocalPoint (&j, j.getLocalBounds().getCentre()).toFloat();
}

bool PatchBay::usable (const Jack* j) const
{
    if (j == nullptr) return false;
    for (const juce::Component* p = j; p != nullptr; p = p->getParentComponent())          // visible = toute la chaine de parents l'est
        if (! p->isVisible()) return false;
    if (auto* vp = j->findParentComponentOfClass<juce::Viewport>())                       // prise defilee hors du rack : pas de cable
        if (! vp->getLocalBounds().contains (vp->getLocalPoint (j, j->getLocalBounds().getCentre()))) return false;
    return true;
}

uint32_t PatchBay::stateHash() const
{
    uint32_t h = cablesVisible ? 17u : 19u;
    for (auto& a : proc.engine.audioIn) h = h * 31u + a.load();
    for (auto& c : proc.engine.cvIn)    h = h * 31u + c.load();
    for (auto& j : jacks)
        if (usable (j.get()))
        {
            const auto p = centreOf (*j);
            h = h * 31u + (uint32_t) ((int) p.x * 4099 + (int) p.y);
        }
    h = h * 31u + (hovered != nullptr ? 1u : 0u);
    return h;
}

void PatchBay::repaintLeds() { for (auto* j : cvOutJ) if (usable (j)) j->repaint(); }

void PatchBay::beginDrag (Jack& j)
{
    dragFrom = &j;
    dragPos = centreOf (j);
    auto isSrc = [] (JackKind k) { return k == JackKind::AudioOut || k == JackKind::CvOut; };
    auto isAudio = [] (JackKind k) { return k == JackKind::AudioOut || k == JackKind::AudioIn; };
    for (auto& o : jacks)
    {
        o->highlight = usable (o.get()) && o.get() != &j && isSrc (o->kind) != isSrc (j.kind) && isAudio (o->kind) == isAudio (j.kind);
        o->repaint();
    }
}

void PatchBay::dragMoved (juce::Point<int> p) { dragPos = p.toFloat(); repaint(); }

void PatchBay::tryConnect (Jack& a, Jack& b)
{
    auto isSrc = [] (JackKind k) { return k == JackKind::AudioOut || k == JackKind::CvOut; };
    Jack* s = &a; Jack* d = &b;
    if (! isSrc (a.kind)) std::swap (s, d);
    if (s->kind == JackKind::AudioOut && d->kind == JackKind::AudioIn)      proc.engine.setAudioCable (s->index, d->index, true);
    else if (s->kind == JackKind::CvOut && d->kind == JackKind::CvIn)       proc.engine.setCvCable (s->index, d->index, true);
}

void PatchBay::endDrag (juce::Point<int> p)
{
    if (dragFrom != nullptr)
    {
        for (auto& o : jacks)
            if (o.get() != dragFrom && usable (o.get()) && centreOf (*o).getDistanceFrom (p.toFloat()) < 18.f) { tryConnect (*dragFrom, *o); break; }
        dragFrom = nullptr;
    }
    for (auto& o : jacks) if (o->highlight) { o->highlight = false; o->repaint(); }
    repaint();
}

void PatchBay::clearJack (Jack& j)
{
    auto& e = proc.engine;
    switch (j.kind)
    {
        case JackKind::AudioIn:  e.audioIn[j.index].store (0); break;
        case JackKind::CvIn:     e.cvIn[j.index].store (0); break;
        case JackKind::AudioOut: for (int d = kFirstMod; d < kNumNodes; ++d) e.setAudioCable (j.index, d, false); break;
        case JackKind::CvOut:    for (int d = 0; d < kNumCv; ++d) e.setCvCable (j.index, d, false); break;
    }
    dragFrom = nullptr;
    repaint();
}

void PatchBay::paint (juce::Graphics& g)
{
    if (! cablesVisible && dragFrom == nullptr) return;
    auto& e = proc.engine;

    std::vector<int> seq { (int) NIn };                                                  // INPUT, modules du rack, OUTPUT
    for (int n : e.getRack()) seq.push_back (n);
    seq.push_back ((int) NOut);
    uint32_t activeSrc = 0;
    for (int n : seq) if (n != NOut) activeSrc |= 1u << n;

    // cablage par defaut (normalise) : traits pointilles tant que l'entree n'est pas cablee
    const float dashes[2] { 4.f, 4.f };
    for (size_t i = 1; i < seq.size(); ++i)
    {
        const int nd = seq[i], prev = seq[i - 1];
        if ((e.audioIn[nd].load() & activeSrc) != 0 || ! usable (audioOutJ[prev]) || ! usable (audioInJ[nd])) continue;
        juce::Path line, dashed;
        line.startNewSubPath (centreOf (*audioOutJ[prev]));
        line.lineTo (centreOf (*audioInJ[nd]));
        juce::PathStrokeType (1.4f).createDashedStroke (dashed, line, dashes, 2);
        g.setColour (look::print.withAlpha (0.3f));
        g.fillPath (dashed);
    }

    auto involved = [this] (const Jack* a, const Jack* b) { return hovered != nullptr && (hovered == a || hovered == b); };
    auto drawIf = [&] (Jack* a, Jack* b, juce::Colour col)
    {
        if (! usable (a) || ! usable (b)) return;
        const bool hot = involved (a, b);
        drawCable (g, centreOf (*a), centreOf (*b), col, hot ? 1.f : 0.72f, hot ? 1.2f : 0.f);
    };
    for (int n : seq)
    {
        if (n == NIn) continue;
        const uint32_t m = e.audioIn[n].load() & activeSrc;
        for (int s = 0; s < NOut; ++s)
            if (m & (1u << s)) drawIf (audioOutJ[s], audioInJ[n], kCablePalette[(s * 3 + n) % 6]);
    }
    for (int d = 0; d < kNumCv; ++d)
    {
        const uint32_t m = e.cvIn[d].load();
        for (int s = 0; s < kNumSrc; ++s)
            if (m & (1u << s)) drawIf (cvOutJ[s], cvInJ[d], kCablePalette[(s + 2 * d + 1) % 6]);
    }
    if (dragFrom != nullptr)
        drawCable (g, centreOf (*dragFrom), dragPos, juce::Colour (0xffe6e4da), 0.9f);
}

// ================================================================ panneau patchbay
namespace
{
const char* cvName (int d)
{
    static const char* n[zephyr::kNumCv] { "SAT DRIVE", "WIDTH", "PH MIX", "HPF", "LPF", "PEAK", "PH RATE", "ENS DEPTH", "ENS MIX", "MIX", "S&H IN", "S&H CLK", "STUT GO", "STUT LEN", "STUT DEC", "STUT PIT", "STUT MIX" };
    return n[juce::jlimit (0, (int) zephyr::kNumCv - 1, d)];
}
int cvOwner (int d)                                         // module qui porte la destination (-1 = toujours disponible)
{
    switch (d)
    {
        case CvDrive:                          return NSat;
        case CvWidth:                          return NWide;
        case CvHpf: case CvLpf: case CvPeak:   return NFilter;
        case CvPhMix: case CvPhRate:           return NPhaser;
        case CvEnsDepth: case CvEnsMix:        return NEnsemble;
        case CvStutOn: case CvStutLen: case CvStutDecay: case CvStutPitch: case CvStutMix: return NStut;
        default:                               return -1;
    }
}
}

void PatchPanel::layoutJacks()
{
    labels.clear();
    auto& e = owner.proc.engine;
    std::vector<int> seq { (int) NIn };
    for (int n : e.getRack()) seq.push_back (n);
    seq.push_back ((int) NOut);
    for (int n = 0; n < kNumNodes; ++n)
    {
        if (audioOut[n] != nullptr) audioOut[n]->setVisible (false);
        if (audioIn[n] != nullptr)  audioIn[n]->setVisible (false);
    }
    const int x0 = 12, y0 = 58, pitch = 26, inX = 196, outX = 252;
    for (size_t i = 0; i < seq.size(); ++i)
    {
        const int n = seq[i], y = y0 + (int) i * pitch;
        labels.push_back ({ nodeName (n), { x0, y - 7, 150, 14 }, false, false });
        if (audioIn[n] != nullptr)  { audioIn[n]->setVisible (true);  audioIn[n]->setCentrePosition (inX, y); }
        if (audioOut[n] != nullptr) { audioOut[n]->setVisible (true); audioOut[n]->setCentrePosition (outX, y); }
    }
    const int ySrc = y0 + 13 * pitch + 36;                                                   // sorties de modulation
    static const char* srcNames[kNumSrc] { "LFO 1", "LFO 2", "S&H", "ENV" };
    for (int s = 0; s < kNumSrc; ++s)
    {
        const int cx = 12 + 36 + s * 70;
        cvOut[s]->setCentrePosition (cx, ySrc);
        labels.push_back ({ srcNames[s], { cx - 35, ySrc + 20, 70, 12 }, false, true });
    }
    const int yCv = ySrc + 78;                                                               // entrees CV
    for (int d = 0; d < kNumCv; ++d)
    {
        const int cx = 12 + 28 + (d % 5) * 56, cy = yCv + (d / 5) * 50;
        const int ow = cvOwner (d);
        const bool avail = ow < 0 || e.inRack (ow);
        cvIn[d]->setVisible (avail);
        cvIn[d]->setCentrePosition (cx, cy);
        labels.push_back ({ cvName (d), { cx - 28, cy + 20, 56, 12 }, ! avail, true });
    }
    repaint();
}

void PatchPanel::paint (juce::Graphics& g)
{
    drawPlate (g, getLocalBounds().toFloat());
    auto head = [&] (const juce::String& t, int y)
    {
        g.setColour (juce::Colour (look::T().accent));
        g.fillRoundedRectangle (14.f, (float) y + 3.f, 3.f, 11.f, 1.5f);
        g.setColour (look::print.withAlpha (0.95f));
        g.setFont (font (11.f).withExtraKerningFactor (0.06f));
        g.drawText (t, 22, y, getWidth() - 44, 16, juce::Justification::centredLeft, false);
        g.setColour (look::print.withAlpha (0.2f));
        g.fillRect (14, y + 19, getWidth() - 28, 1);
    };
    head ("PATCHBAY", 5);
    const int y0 = 58, pitch = 26;
    g.setColour (look::print.withAlpha (0.7f));
    g.setFont (font (9.f));
    g.drawText ("IN", 196 - 20, 36, 40, 11, juce::Justification::centred, false);
    g.drawText ("OUT", 252 - 20, 36, 40, 11, juce::Justification::centred, false);
    g.drawText ("AUDIO", 12, 36, 100, 11, juce::Justification::centredLeft, false);
    const int ySrc = y0 + 13 * pitch + 36;
    head ("MODULATION OUT", ySrc - 46);
    head ("CV IN", ySrc + 40);
    g.setColour (look::print.withAlpha (0.55f));
    g.setFont (font (8.5f, false));
    g.drawFittedText (fr ("Glisser d'une sortie vers une entr\xc3\xa9" "e.  Double-clic sur une prise : d\xc3\xa9" "brancher."),
                      14, getHeight() - 27, getWidth() - 28, 21, juce::Justification::centredLeft, 2);
}

void PatchPanel::paintOverChildren (juce::Graphics& g)
{
    g.setFont (font (9.5f));
    for (const auto& l : labels)
    {
        g.setColour ((l.cv ? look::cvRing : look::print).withAlpha (l.dim ? 0.3f : 0.95f));
        g.drawText (l.text, l.r, l.cv ? juce::Justification::centred : juce::Justification::centredLeft, false);
    }
    drawSpecks (g, getLocalBounds());
}

// ================================================================ module
ModuleUnit::ModuleUnit (RackView* rack, int n, const juce::String& t, int h, bool f)
    : node (n), title (t), fullHeight (h), fixed (f), rackView (rack) {}

ModuleUnit::Region ModuleUnit::regionAt (juce::Point<int> p) const
{
    if (p.y > 24) return None;
    if (fixed) return None;
    if (p.x > getWidth() - 40 && p.x < getWidth() - 14) return Remove;
    if (p.x > getWidth() - 68 && p.x <= getWidth() - 40) return Collapse;
    if (p.x > getWidth() - 98 && p.x <= getWidth() - 68 && isActive) return Power;
    return Handle;
}

void ModuleUnit::addJack (Jack* j, int col, int row, const juce::String& label)
{
    jackSlots.push_back ({ j, col, row, label });
    jackCols = juce::jmax (jackCols, col + 1);
}

void ModuleUnit::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    drawPlate (g, b);
    const auto& th = look::T();
    const bool fxModT = ! fixed && node > zephyr::NIn && node < zephyr::NOut;
    const int tx = fixed ? 20 : (fxModT ? 46 : 40);
    const int rightRes = fixed ? 20 : (isActive ? 110 : 80);
    const bool fxMod = ! fixed && node > zephyr::NIn && node < zephyr::NOut;
    const juce::Colour mc = fxMod ? moduleColour (node) : juce::Colour (th.accent);
    const bool lit = ! fxMod || ! isActive || isActive();
    if (fxMod && ! collapsed)
    {
        // grand pictogramme grave dans la plaque, a droite (sous les commandes : il reste discret)
        const float gh = juce::jmin ((float) getHeight() - 58.f, 92.f), gw = gh * 1.7f;
        const juce::Rectangle<float> gr ((float) getWidth() - (float) jackStripWidth() - gw - 34.f, 30.f + ((float) getHeight() - 30.f - gh) * 0.5f, gw, gh);
        if (gh > 30.f)
        {
            const auto glyph = moduleGlyph (node, gr);
            const juce::PathStrokeType st (6.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.30f : 0.10f)); g.strokePath (glyph, st, juce::AffineTransform::translation (0.f, 1.6f));
            g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.05f : 0.55f));  g.strokePath (glyph, st, juce::AffineTransform::translation (0.f, -1.f));
            g.setColour (mc.withAlpha (lit ? (th.dark ? 0.26f : 0.26f) : 0.06f));    g.strokePath (glyph, st);
        }
        g.setColour (look::print.withAlpha (0.32f));                                           // reference gravee du module
        g.setFont (font (9.f).withExtraKerningFactor (0.18f));
        g.drawText (moduleCode (node), getWidth() - jackStripWidth() - 160, getHeight() - 20, 136, 12, juce::Justification::centredRight, false);
    }
    if (fxMod)
    {
        // pastille de verre de la couleur du module devant le titre (allumee si le module est actif)
        const juce::Rectangle<float> jw ((float) tx - 11.f, 6.5f, 8.f, 13.f);
        if (lit) { g.setColour (mc.withAlpha (0.35f)); g.fillRoundedRectangle (jw.expanded (2.f), 4.f); }
        g.setGradientFill (juce::ColourGradient (lit ? mc.brighter (0.55f) : mc.withAlpha (0.3f), jw.getX(), jw.getY(), lit ? mc.darker (0.25f) : mc.withAlpha (0.2f), jw.getX(), jw.getBottom(), false));
        g.fillRoundedRectangle (jw, 3.f);
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.7f : 0.25f)); g.fillRoundedRectangle (jw.getX() + 1.5f, jw.getY() + 1.5f, 2.f, 5.f, 1.f);
    }
    else
    {
        g.setColour (juce::Colour (th.accent));                                          // petite barre d'accent devant le titre
        g.fillRoundedRectangle ((float) tx - 8.f, 7.f, 3.f, 12.f, 1.5f);
    }
    g.setColour (look::print.withAlpha (0.95f));
    g.setFont (font (12.f).withExtraKerningFactor (0.06f));
    g.drawText (title, tx, 4, getWidth() - tx - rightRes, 18, juce::Justification::centredLeft, false);
    if (fxMod)                                                                           // filet lumineux de la couleur du module sous le titre
    {
        const float lw = (float) (getWidth() - (tx - 8) - rightRes);
        g.setGradientFill (juce::ColourGradient (mc.withAlpha (lit ? 0.9f : 0.3f), (float) tx - 8.f, 0.f, mc.withAlpha (0.f), (float) tx - 8.f + lw, 0.f, false));
        g.fillRect ((float) tx - 8.f, 23.5f, lw, 2.f);
    }
    else
    {
        g.setColour (look::print.withAlpha (0.18f));
        g.fillRect (tx - 8, 24, getWidth() - (tx - 8) - rightRes, 1);
    }
    if (! fixed && isActive)                                                             // bouton d'activation (bypass) : LED bleue allumee = module actif
    {
        const bool on = isActive();
        const juce::Point<float> pc ((float) getWidth() - 83.f, 13.f);
        g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.5f : 0.22f)); g.fillEllipse (pc.x - 8.5f, pc.y - 7.5f, 17.f, 17.f);
        g.setGradientFill (juce::ColourGradient (on ? C (th.led).brighter (0.6f) : C (th.keyHi), pc.x, pc.y - 8.f, on ? C (th.led).darker (0.3f) : C (th.keyLo), pc.x, pc.y + 8.f, false));
        g.fillEllipse (pc.x - 8.f, pc.y - 8.f, 16.f, 16.f);
        if (on) { g.setColour (C (th.led).withAlpha (0.35f)); g.drawEllipse (pc.x - 10.f, pc.y - 10.f, 20.f, 20.f, 2.f); }
        g.setColour (look::print.withAlpha (0.55f)); g.drawEllipse (pc.x - 8.f, pc.y - 8.f, 16.f, 16.f, 0.9f);
        juce::Path pw; pw.addCentredArc (pc.x, pc.y + 0.5f, 4.2f, 4.2f, 0.f, 0.65f, 5.63f, true); pw.startNewSubPath (pc.x, pc.y - 5.f); pw.lineTo (pc.x, pc.y);
        g.setColour (on ? juce::Colours::white : look::print.withAlpha (0.7f)); g.strokePath (pw, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    if (! fixed)
    {
        g.setColour (look::print.withAlpha (0.45f));                                     // poignee de deplacement
        for (int cx = 0; cx < 2; ++cx) for (int cy = 0; cy < 3; ++cy)
            g.fillEllipse (17.f + (float) cx * 6.f, 7.f + (float) cy * 5.f, 2.6f, 2.6f);
        juce::Path tri;                                                                  // replier / deplier
        const float tx2 = (float) getWidth() - 59.f;
        if (collapsed) tri.addTriangle (tx2, 7.f, tx2, 19.f, tx2 + 10.f, 13.f);
        else           tri.addTriangle (tx2 - 1.f, 8.f, tx2 + 11.f, 8.f, tx2 + 5.f, 18.f);
        g.setColour (look::print.withAlpha (0.75f));
        g.fillPath (tri);
        const float xx = (float) getWidth() - 32.f;                                      // retirer du rack
        g.setColour (look::print.withAlpha (0.6f));
        g.drawLine (xx, 8.f, xx + 9.f, 17.f, 1.6f);
        g.drawLine (xx + 9.f, 8.f, xx, 17.f, 1.6f);
    }
    if (! collapsed && ! jackSlots.empty())                                              // etiquettes des prises + filet de separation
    {
        const int x0 = getWidth() - 14 - jackCols * 58;
        g.setColour (look::print.withAlpha (0.2f));
        g.fillRect (x0 - 8, 32, 1, getHeight() - 42);
        g.setFont (font (8.f));
        for (const auto& sl : jackSlots)
        {
            const bool cv = sl.jack->kind == JackKind::CvIn || sl.jack->kind == JackKind::CvOut;
            g.setColour (cv ? look::cvRing : look::print.withAlpha (0.9f));
            g.drawText (sl.label, x0 + sl.col * 58, 34 + sl.row * 38 + 28, 58, 10, juce::Justification::centred, false);
        }
    }
}

void ModuleUnit::paintOverChildren (juce::Graphics& g)
{
    if (isActive && ! isActive() && ! collapsed)                                          // module contourne : face assombrie, titre intact
    {
        g.setColour (juce::Colours::black.withAlpha (0.46f));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (3.f).withTrimmedTop (24.f), 4.f);
    }
}

void ModuleUnit::resized()
{
    for (auto* c : getChildren()) c->setVisible (! collapsed);
    if (collapsed) return;
    const int x0 = getWidth() - 14 - jackCols * 58;
    for (const auto& s : jackSlots)
        s.jack->setCentrePosition (x0 + s.col * 58 + 29, 34 + s.row * 38 + 14);
    if (layoutFn) layoutFn (getLocalBounds().withTrimmedTop (topTrim).withTrimmedBottom (8).withTrimmedLeft (16).withTrimmedRight (16 + jackStripWidth()));
}

void ModuleUnit::mouseDown (const juce::MouseEvent& e)
{
    pressed = regionAt (e.getPosition());
    if (pressed == Handle && rackView != nullptr) { grabOffset = e.y; rackView->dragStart (this, grabOffset); }
}

void ModuleUnit::mouseDrag (const juce::MouseEvent& e)
{
    if (pressed == Handle && rackView != nullptr) rackView->dragMove (this, e.getEventRelativeTo (rackView).y, grabOffset);
}

void ModuleUnit::mouseUp (const juce::MouseEvent& e)
{
    const auto region = regionAt (e.getPosition());
    if (pressed == Handle && rackView != nullptr) rackView->dragEnd (this);
    else if (pressed == Collapse && region == Collapse)
    {
        collapsed = ! collapsed;
        resized(); repaint();
        if (rackView != nullptr) rackView->relayout();
    }
    else if (pressed == Power && region == Power && onToggleActive) { onToggleActive(); repaint(); }
    else if (pressed == Remove && region == Remove && onRemove) onRemove();
    pressed = None;
}

// ================================================================ rack
void RackView::paint (juce::Graphics& g)
{
    if (order.empty())
    {
        g.setColour (look::print.withAlpha (0.7f));
        g.setFont (font (14.f));
        g.drawText (fr ("RACK VIDE - clique sur un module \xc3\xa0 gauche pour l'ajouter"), getLocalBounds().withHeight (80),
                    juce::Justification::centred, false);
    }
}

void RackView::setUnits (std::vector<ModuleUnit*> list)
{
    for (auto* c : getChildren()) c->setVisible (false);
    order = std::move (list);
    for (auto* u : order) u->setVisible (true);
    layoutUnits (order);
}

void RackView::layoutUnits (const std::vector<ModuleUnit*>& list, ModuleUnit* dragged, int draggedY)
{
    const int w = getWidth(), gap = 8;
    int y = 0;
    for (auto* u : list)
    {
        if (u != dragged) u->setBounds (0, y, w, u->currentHeight());
        y += u->currentHeight() + gap;
    }
    const int total = juce::jmax (y, 90);
    if (dragged != nullptr)
    {
        dragged->setBounds (0, juce::jlimit (0, juce::jmax (0, total - dragged->currentHeight()), draggedY), w, dragged->currentHeight());
        dragged->toFront (false);
    }
    if (getHeight() != total) setSize (w, total);
}

void RackView::dragStart (ModuleUnit* u, int grab) { temp = order; dragU = u; dragGrab = grab; startTimerHz (60); }

void RackView::dragMove (ModuleUnit* u, int pointerY, int grab)
{
    std::vector<ModuleUnit*> others;
    for (auto* o : order) if (o != u) others.push_back (o);
    const int centre = pointerY - grab + u->currentHeight() / 2;
    int idx = 0, y = 0;
    for (auto* o : others) { const int h = o->currentHeight(); if (centre > y + h / 2) ++idx; y += h + 8; }
    temp = others;
    temp.insert (temp.begin() + idx, u);
    lastPointerY = pointerY; dragGrab = grab;
    layoutUnits (temp, u, pointerY - grab);
}

// pendant un deplacement : le rack defile tout seul quand on approche du haut ou du bas de la fenetre
void RackView::timerCallback()
{
    auto* vp = findParentComponentOfClass<juce::Viewport>();
    if (vp == nullptr || dragU == nullptr) return;
    const int top = vp->getViewPositionY(), vh = vp->getViewHeight(), zone = 64;
    const int rel = lastPointerY - top;
    int dy = 0;
    if (rel < zone)           dy = -(int) (2.f + (float) juce::jmin (zone * 2, zone - rel) * 0.25f);
    else if (rel > vh - zone) dy = (int) (2.f + (float) juce::jmin (zone * 2, rel - (vh - zone)) * 0.25f);
    if (dy == 0) return;
    const int ny = juce::jlimit (0, juce::jmax (0, getHeight() - vh), top + dy);
    if (ny == top) return;
    vp->setViewPosition (vp->getViewPositionX(), ny);
    dragMove (dragU, lastPointerY + (ny - top), dragGrab);
}

void RackView::dragEnd (ModuleUnit*)
{
    stopTimer(); dragU = nullptr;
    if (! temp.empty()) order = temp;
    temp.clear();
    layoutUnits (order);
    std::vector<int> nodes;
    for (auto* o : order) nodes.push_back (o->node);
    owner.rackChangedByUser (nodes);
}

// ================================================================ panneau lateral
void PlatePanel::paint (juce::Graphics& g)
{
    drawPlate (g, getLocalBounds().toFloat());
    auto head = [&] (const juce::String& t, int y)
    {
        g.setColour (juce::Colour (look::T().accent));
        g.fillRoundedRectangle (14.f, (float) y + 3.f, 3.f, 11.f, 1.5f);
        g.setColour (look::print.withAlpha (0.95f));
        g.setFont (font (11.f).withExtraKerningFactor (0.06f));
        g.drawText (t, 22, y, getWidth() - 44, 16, juce::Justification::centredLeft, false);
        g.setColour (look::print.withAlpha (0.2f));
        g.fillRect (14, y + 19, getWidth() - 28, 1);
    };
    head (title, 5);
    for (const auto& s : subtitles) head (s.first, s.second);
}
void PlatePanel::paintOverChildren (juce::Graphics& g)
{
    g.setFont (font (8.f));
    g.setColour (look::print.withAlpha (0.9f));
    for (const auto& n : notes)
        g.drawText (std::get<0> (n), std::get<1> (n) - 30, std::get<2> (n), 60, 10, juce::Justification::centred, false);
    drawSpecks (g, getLocalBounds());
}

// ================================================================ boite de dialogue integree
ZephyrDialog::ZephyrDialog (juce::String t, juce::String m, bool askText, juce::String initialText, juce::String okText,
                          std::function<void (bool, const juce::String&)> callback)
    : titleText (std::move (t)), messageText (std::move (m)), hasText (askText), cb (std::move (callback))
{
    setWantsKeyboardFocus (true);
    okBtn.setButtonText (okText);
    cancelBtn.setButtonText ("ANNULER");
    okBtn.onClick = [this] { finish (true); };
    cancelBtn.onClick = [this] { finish (false); };
    addAndMakeVisible (okBtn);
    addAndMakeVisible (cancelBtn);
    if (hasText)
    {
        editor.setText (initialText, false);
        editor.setFont (font (16.f));
        editor.setSelectAllWhenFocused (true);
        editor.setIndents (8, 6);
        editor.onReturnKey = [this] { finish (true); };
        editor.onEscapeKey = [this] { finish (false); };
        addAndMakeVisible (editor);
    }
}

void ZephyrDialog::focus()
{
    if (hasText) editor.grabKeyboardFocus(); else grabKeyboardFocus();
}

bool ZephyrDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { finish (false); return true; }
    if (k == juce::KeyPress::returnKey) { finish (true); return true; }
    return true;                                                      // modal : rien ne passe a travers
}

void ZephyrDialog::finish (bool ok)
{
    if (done) return;
    done = true;
    const auto text = editor.getText();
    setVisible (false);
    if (cb) cb (ok, text);
    if (onFinished) onFinished();
}

void ZephyrDialog::resized()
{
    const int w = 520, h = hasText ? 230 : 200;
    box = juce::Rectangle<int> (w, h).withCentre (getLocalBounds().getCentre());
    if (hasText) editor.setBounds (box.getX() + 28, box.getY() + 96, w - 56, 34);
    cancelBtn.setBounds (box.getRight() - 28 - 140 - 10 - 140, box.getBottom() - 28 - 34, 140, 34);
    okBtn.setBounds (box.getRight() - 28 - 140, box.getBottom() - 28 - 34, 140, 34);
}

void ZephyrDialog::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.45f));
    const auto b = box.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (b.translated (3.f, 6.f), 7.f);
    drawPlate (g, b);
    g.setColour (look::print);
    g.setFont (font (16.f).withExtraKerningFactor (0.05f));
    g.drawText (titleText, box.getX() + 32, box.getY() + 22, box.getWidth() - 64, 22, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (look::T().accent));
    g.fillRect (box.getX() + 32, box.getY() + 48, 60, 3);
    g.setColour (look::print.withAlpha (0.9f));
    g.setFont (font (13.f, false));
    g.drawFittedText (messageText, box.getX() + 32, box.getY() + 60, box.getWidth() - 64, hasText ? 30 : 70, juce::Justification::topLeft, 3);
}

// ================================================================ editeur
#if ZEPHYR_SYNTH
// ---------------------------------------------------------------- panneau SETTINGS : reglages globaux (jamais dans les presets)
namespace
{
struct SettingsPanel : public juce::Component
{
    explicit SettingsPanel (ZephyrAudioProcessor& p) : proc (p)
    {
        struct Row { const char* title; const char* hint; juce::StringArray items; std::atomic<int>* v; };
        const Row rows[] {
            { "AUDIO QUALITY", "High Fidelity : oversampling x8 on filters, folder and drive. Heavier on CPU.",
              { "Performance  (filters x2)", "Quality  (filters x4)", "High Fidelity  (filters x8)" }, &p.quality },
            { "MIDI SMOOTHING", "Glides knob changes so a 7-bit controller does not zipper.",
              { "Off", "5 ms", "10 ms", "20 ms  (default)", "40 ms", "80 ms" }, &p.ccSmooth },
            { "AFTERTOUCH CURVE", "Shape of the pressure response.",
              { "Linear", "Exponential  (soft start)", "Logarithmic  (sensitive start)", "S-curve" }, &p.atCurve },
            { "AFTERTOUCH RESPONSE", "How fast the pressure follows your finger.",
              { "Instant", "Fast", "Medium  (default)", "Slow", "Slower", "Very slow" }, &p.atSpeed },
            { "MIDI TAKEOVER", "When a hardware knob does not match the plugin knob.",
              { "Scale  (converges smoothly)", "Pickup  (waits to cross)", "Direct  (jumps)" }, &p.takeoverMode } };
        for (const auto& r : rows)
        {
            auto lb = std::make_unique<juce::Label>(); lb->setText (r.title, juce::dontSendNotification);
            lb->setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold))); lb->setColour (juce::Label::textColourId, juce::Colour (look::T().screenText));
            auto hn = std::make_unique<juce::Label>(); hn->setText (r.hint, juce::dontSendNotification);
            hn->setFont (juce::Font (juce::FontOptions (9.5f))); hn->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
            auto cb = std::make_unique<juce::ComboBox>(); cb->addItemList (r.items, 1);
            cb->setSelectedId (r.v->load() + 1, juce::dontSendNotification);
            auto* a = r.v; auto* c = cb.get();
            cb->onChange = [this, a, c] { a->store (c->getSelectedId() - 1); proc.settingChanged(); };
            addAndMakeVisible (*lb); addAndMakeVisible (*hn); addAndMakeVisible (*cb);
            labels.push_back (std::move (lb)); hints.push_back (std::move (hn)); combos.push_back (std::move (cb));
        }
        setSize (420, 24 + (int) combos.size() * 58);
    }
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (look::T().screenBg)); }
    void resized() override
    {
        int y = 12;
        for (size_t k = 0; k < combos.size(); ++k)
        {
            labels[k]->setBounds (12, y, 150, 16); combos[k]->setBounds (170, y - 2, getWidth() - 182, 24);
            hints[k]->setBounds (12, y + 22, getWidth() - 24, 14); y += 58;
        }
    }
    ZephyrAudioProcessor& proc;
    std::vector<std::unique_ptr<juce::Label>> labels, hints;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
};
}
#endif

ZephyrEditor::ZephyrEditor (ZephyrAudioProcessor& p, juce::String title, juce::String subtitle)
    : AudioProcessorEditor (&p), proc (p), titleText (std::move (title)), subText (std::move (subtitle)), patchBay (p)
{
    setLookAndFeel (&laf);
    look::applyTheme (loadThemePref());
    ZephyrLookAndFeel::refreshAll();

    addAndMakeVisible (themeBtn);
    themeBtn.onClick = [this] { cycleTheme(); };
    addAndMakeVisible (prevBtn); addAndMakeVisible (nameBtn); addAndMakeVisible (nextBtn); addAndMakeVisible (saveBtn);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    nameBtn.onClick = [this] { presetMenu(); };
    saveBtn.onClick = [this] { saveDialog(); };
    nameBtn.setButtonText (proc.presetName);

    viewport.setViewedComponent (&rackView, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);
    addAndMakeVisible (side);
    addAndMakeVisible (patchPanel);

    buildUnits();

    addAndMakeVisible (cablesBtn);                                  // le calque des cables passe par-dessus tout
    cablesBtn.setButtonText ("CABLES");
    cablesBtn.setToggleState (true, juce::dontSendNotification);
    cablesBtn.onClick = [this] { patchBay.cablesVisible = cablesBtn.getToggleState(); patchBay.repaint(); };
#if ZEPHYR_SYNTH
    addAndMakeVisible (midiMon);                                    // moniteur : dernier message MIDI recu + position de la molette
    midiMon.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold))); midiMon.setColour (juce::Label::textColourId, juce::Colour (0xffe8d9a8)); midiMon.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (midiBtn);                                    // montre / cache les etiquettes CC des potards assignes
    midiBtn.setButtonText ("MIDI");
    {
        const auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Foudre Audio").getChildFile ("midishow.txt");
        look::showMidi = f.existsAsFile() && f.loadFileAsString().trim().getIntValue() != 0;
    }
    midiBtn.setToggleState (look::showMidi, juce::dontSendNotification);
    midiBtn.onClick = [this]
    {
        look::showMidi = midiBtn.getToggleState();
        const auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Foudre Audio").getChildFile ("midishow.txt");
        f.getParentDirectory().createDirectory(); f.replaceWithText (look::showMidi ? "1" : "0");
    };
    addAndMakeVisible (settingsBtn);
    settingsBtn.setButtonText ("SETTINGS");
    settingsBtn.onClick = [this]
    {
        juce::CallOutBox::launchAsynchronously (std::make_unique<SettingsPanel> (proc), settingsBtn.getScreenBounds(), getTopLevelComponent());
    };
#endif
    addAndMakeVisible (patchBay);

    setSize (kDesignW, kFxH);
    refreshRack();
    startTimerHz (30);
}

ZephyrEditor::~ZephyrEditor() { stopTimer(); setLookAndFeel (nullptr); }

KnobUnit* ZephyrEditor::mk (juce::Component& parent, const char* id, const juce::String& text,
                                         const char* lo, const char* hi, int cv)
{
    knobs.push_back (std::make_unique<KnobUnit> (proc.apvts, id, text, lo, hi, cv));
    auto* k = knobs.back().get();
    knobById[id] = k;
    parent.addAndMakeVisible (*k);
    return k;
}

ModuleUnit* ZephyrEditor::newUnit (int node, const juce::String& title, int height, bool fixed)
{
    if (fixed)
    {
        stripUnits.push_back (std::make_unique<ModuleUnit> (nullptr, node, title, height, true));
        stripUnits.back()->topTrim = 33;
        addAndMakeVisible (*stripUnits.back());
        return stripUnits.back().get();
    }
    auto u = std::make_unique<ModuleUnit> (&rackView, node, title, height, false);
    auto* raw = u.get();
    raw->topTrim = 33;
    u->onRemove = [this, node] { proc.engine.removeFromRack (node); refreshRack(); };
    u->isActive = [this, node] { return (proc.engine.bypassMask.load() & (1u << node)) == 0; };
    u->onToggleActive = [this, node, raw]
    {
        const bool wasBypassed = (proc.engine.bypassMask.load() & (1u << node)) != 0;
        proc.engine.bypassMask.fetch_xor (1u << node); lastBypass = proc.engine.bypassMask.load();
        if (wasBypassed)                                                      // le module s'allume : tous ses potards repartent de leur valeur neutre (jamais un potard reste a fond)
            for (auto* ch : raw->getChildren())
                if (auto* k = dynamic_cast<KnobUnit*> (ch))
                    if (auto* prm = proc.apvts.getParameter (k->getParamId())) prm->setValueNotifyingHost (prm->getDefaultValue());
    };
    rackView.addChildComponent (*raw);
    rackUnits[node] = std::move (u);
    return raw;
}

void ZephyrEditor::buildUnits()
{
    constexpr int kModH = 158, kCell = 108;
    auto addSelector = [this] (juce::Component& parent, const char* id, const juce::StringArray& names)
    {
        auto sel = std::make_unique<SegmentSelector> (proc.apvts, id, names);
        auto* r = sel.get();
        parent.addAndMakeVisible (*r);
        owned.push_back (std::move (sel));
        return r;
    };
    auto addSync = [this] (juce::Component& parent, const char* id, KnobUnit* rate)
    {
        auto cb = std::make_unique<juce::ComboBox>();
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
            cb->addItemList (choice->choices, 1);
        cb->setScrollWheelEnabled (false);
        parent.addAndMakeVisible (*cb);
        comboAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *cb));
        auto* r = cb.get();
        combos.push_back (std::move (cb));
        syncLinks.push_back ({ r, rate, true });
        return r;
    };
    auto addToggle = [this] (juce::Component& parent, const char* id, const char* label)
    {
        auto b = std::make_unique<LedToggle>();
        b->setButtonText (label);
        parent.addAndMakeVisible (*b);
        buttonAtt.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b));
        auto* r = b.get();
        owned.push_back (std::move (b));
        return r;
    };
    auto place = [] (juce::Rectangle<int> r, std::vector<KnobUnit*> ks, int x0, int cellW)
    {
        cellW = juce::jmin (cellW, (r.getWidth() - x0) / juce::jmax (1, (int) ks.size()));     // jamais plus large que le module
        int x = r.getX() + x0;
        const int kh = juce::jmin (r.getHeight(), cellW + 22), y = r.getY() + (r.getHeight() - kh) / 2;
        for (auto* k : ks) { k->setBounds (x, y, cellW, kh); x += cellW; }
    };

    // ---------------- modules du rack
    {   // STEREO WIDE
        auto* u = newUnit (NWide, "STEREO WIDE", kModH, false);
        auto* a = mk (*u, "wideWidth", "WIDTH", "0", "10", CvWidth);
        auto* b = mk (*u, "wideSpread", "SPREAD");
        auto* c = mk (*u, "wideBass", "BASS MONO");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b, c }, 0, kCell); };
    }
    {   // SATURATION
        auto* u = newUnit (NSat, "WARM SATURATION", kModH, false);
        auto* sel = addSelector (*u, "satMode", juce::StringArray { "TAPE", "CONSOLE", "TUBE" });
        auto* a = mk (*u, "satDrive", "DRIVE", "0", "10", CvDrive);
        auto* b = mk (*u, "satTone", "TONE", "DARK", "AIR");
        b->slider.detent = 0.5;
        u->layoutFn = [=] (juce::Rectangle<int> r) { sel->setBounds (r.getX(), r.getCentreY() - 18, 270, 36); place (r, { a, b }, 290, kCell); };
    }
    {   // EQ
        auto* u = newUnit (NColor, "EQ  LOW / MID / AIR", kModH, false);
        auto* a = mk (*u, "low", "LOW", "-12", "+12"); auto* b = mk (*u, "mid", "MID", "-12", "+12"); auto* c = mk (*u, "high", "AIR", "-12", "+12");
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b, c }, 0, kCell); };
    }
    {   // FILTER
        auto* u = newUnit (NFilter, "FILTER  HPF > LPF", kModH, false);
        auto* a = mk (*u, "hpf", "HPF CUTOFF", "0", "10", CvHpf);   auto* b = mk (*u, "hpfPeak", "PEAK", "0", "10", CvPeak);
        auto* c = mk (*u, "lpf", "LPF CUTOFF", "0", "10", CvLpf);   auto* d = mk (*u, "lpfPeak", "PEAK", "0", "10", CvPeak);
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b, c, d }, 0, kCell); };
    }
    {   // PHASER
        auto* u = newUnit (NPhaser, "PHASER", kModH, false);
        auto* a = mk (*u, "phMix", "MIX", "0", "10", CvPhMix);
        auto* b = mk (*u, "phRate", "RATE", "0", "10", CvPhRate);
        auto* c = mk (*u, "phDepth", "DEPTH");
        auto* d = mk (*u, "phFb", "FEEDBACK");
        auto* sync = addSync (*u, "phSync", b);
        auto* st = addSelector (*u, "phStages", juce::StringArray { "4", "6", "8", "12" });
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            place (r, { a, b, c, d }, 0, kCell - 8);
            sync->setBounds (r.getX() + 4 * (kCell - 8) + 20, r.getCentreY() - 40, 140, 26);
            st->setBounds (r.getX() + 4 * (kCell - 8) + 20, r.getCentreY() + 2, 200, 36);
        };
    }
    {   // COMP : type, ratio, threshold, attack, vumeter
        auto* u = newUnit (NComp, "COMPRESSOR", kModH, false);
        auto* sel = addSelector (*u, "compType", juce::StringArray { "OFF", "OPTO", "GLUE" });
        auto* a = mk (*u, "compAmt", "RATIO");
        auto* b = mk (*u, "compThresh", "THRESHOLD");
        auto* c = mk (*u, "compAtt", "ATTACK");
        auto meter = std::make_unique<VuMeter>();
        vu = meter.get();
        u->addAndMakeVisible (*meter);
        owned.push_back (std::move (meter));
        auto* mp = vu;
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            sel->setBounds (r.getX(), r.getCentreY() - 18, 230, 36);
            place (r, { a, b, c }, 244, 90);
            mp->setBounds (r.getX() + 530, r.getY() + 2, 170, r.getHeight() - 4);
        };
    }
    {   // ENSEMBLE
        auto* u = newUnit (NEnsemble, "STRING ENSEMBLE", kModH, false);
        auto* a = mk (*u, "ensMix", "MIX", "0", "10", CvEnsMix); auto* b = mk (*u, "ensDepth", "DEPTH", "0", "10", CvEnsDepth);
        auto* c = mk (*u, "ensSpeed", "SPEED", "SLOW", "FAST"); auto* d = mk (*u, "ensTone", "TONE", "WARM", "AIRY");
        c->slider.detent = 0.5;
        u->layoutFn = [=] (juce::Rectangle<int> r) { place (r, { a, b, c, d }, 0, kCell); };
    }
    {   // STUTTER : repetition rythmique de l'audio (tous les parametres sont pilotables depuis les pistes du sequenceur)
        auto* u = newUnit (NStut, "STUTTER / BEAT REPEAT", kModH, false);
        auto* mode = addSelector (*u, "stutMode", juce::StringArray { "BEAT REPEAT", "REVERSE", "GATE", "OFFBEAT" });
        auto* div  = addSelector (*u, "stutDiv", juce::StringArray { "1 BAR", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" });
        auto* feel = addSelector (*u, "stutFeel", juce::StringArray { "STRAIGHT", "TRIPLET", "DOTTED" });
        auto* len = mk (*u, "stutLen", "LENGTH", "0", "10", CvStutLen); auto* dec = mk (*u, "stutDecay", "DECAY", "0", "10", CvStutDecay); auto* pit = mk (*u, "stutPitch", "PITCH", "-12", "+12", CvStutPitch); auto* mx = mk (*u, "stutMix", "MIX", "0", "10", CvStutMix);
        auto* eng = addToggle (*u, "stutOn", "ENGAGE");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            const int sh = juce::jmin (30, (r.getHeight() - 8) / 3), gap = (r.getHeight() - 3 * sh) / 4;
            mode->setBounds (r.getX(), r.getY() + gap, 340, sh);
            div->setBounds  (r.getX(), r.getY() + 2 * gap + sh, 340, sh);
            feel->setBounds (r.getX(), r.getY() + 3 * gap + 2 * sh, 340, sh);
            place (r, { len, dec, pit, mx }, 356, 92);
            eng->setBounds (r.getX() + 356 + 4 * 92 + 14, r.getCentreY() - 17, 110, 34);
        };
    }

    {   // DELAY MULTIMODE
        auto* u = newUnit (NDelay, "DELAY  TAPE / ICE / PING-PONG / DIGITAL", kModH, false);
        auto* mode = addSelector (*u, "dlyMode", juce::StringArray { "TAPE", "ICE", "PING-PONG", "DIGITAL" });
        auto* a = mk (*u, "dlyTime", "TIME"); auto* b = mk (*u, "dlyFb", "FEEDBACK"); auto* c = mk (*u, "dlyTone", "TONE");
        auto* d = mk (*u, "dlyMod", "MOD"); auto* e = mk (*u, "dlyMix", "MIX");
        auto* sync = addSync (*u, "dlySync", a);
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            mode->setBounds (r.getX(), r.getY() + 4, 330, 36);
            sync->setBounds (r.getX(), r.getY() + 52, 150, 26);
            place (r, { a, b, c, d, e }, 350, kCell - 16);
        };
    }

    {   // PLATE REVERB
        auto* u = newUnit (NVerb, "PLATE REVERB + SHIMMER", kModH, false);
        auto* a = mk (*u, "revMix", "MIX"); auto* b = mk (*u, "revSize", "SIZE"); auto* c = mk (*u, "revDecay", "DECAY");
        auto* d = mk (*u, "revPre", "PRE-DELAY"); auto* e = mk (*u, "revDamp", "DAMPING"); auto* f = mk (*u, "revMod", "MOTION");
        auto* g2 = mk (*u, "revWidth", "WIDTH"); auto* sh = mk (*u, "revShimmer", "SHIMMER"); auto* lc = mk (*u, "revLowCut", "LOW CUT");
        auto* frz = addToggle (*u, "revFreeze", "FREEZE");
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            place (r, { a, b, c, d, e, f, g2, sh, lc }, 0, 76);
            frz->setBounds (r.getX() + 9 * 76 + 14, r.getCentreY() - 17, 110, 34);
        };
    }

    // ---------------- bandeau de modulation (fixe) : sources CV a droite de chaque module
    auto strip = [&] (const char* title, const char* prefix, int srcIdx, bool hasSync)
    {
        auto* u = newUnit (-1, title, 172, true);
        const juce::String pre (prefix);
        auto* rate  = mk (*u, (pre + "Rate").toRawUTF8(), "RATE");
        auto* shape = mk (*u, (pre + (pre == "sh" ? "Slew" : "Shape")).toRawUTF8(), pre == "sh" ? "SLEW" : "SHAPE", pre == "sh" ? "0" : "SIN", pre == "sh" ? "10" : "SQR");
        auto* level = mk (*u, (pre + "Level").toRawUTF8(), "LEVEL");
        juce::ComboBox* sync = hasSync ? addSync (*u, (pre + "Sync").toRawUTF8(), rate) : nullptr;
        u->layoutFn = [=] (juce::Rectangle<int> r)
        {
            if (sync != nullptr) sync->setBounds (r.getX(), r.getY() + 2, r.getWidth(), 24);
            auto k = r.withTrimmedTop (30);
            const int cw = k.getWidth() / 3;
            rate->setBounds (k.getX(), k.getY(), cw, k.getHeight());
            shape->setBounds (k.getX() + cw, k.getY(), cw, k.getHeight());
            level->setBounds (k.getX() + 2 * cw, k.getY(), cw, k.getHeight());
        };
    };
    strip ("LFO 1", "lfo1", SrcLfo1, true);
    strip ("LFO 2", "lfo2", SrcLfo2, true);
    strip ("SAMPLE & HOLD", "sh", SrcSh, true);
    {   // ENVELOPE FOLLOWER
        auto* u = newUnit (-1, "ENVELOPE FOLLOWER", 172, true);
        auto* a = mk (*u, "envRel", "RELEASE"); auto* b = mk (*u, "envLevel", "LEVEL");
        u->layoutFn = [=] (juce::Rectangle<int> r) { a->setBounds (r.getX(), r.getY() + 30, r.getWidth() / 2, r.getHeight() - 30); b->setBounds (r.getX() + r.getWidth() / 2, r.getY() + 30, r.getWidth() / 2, r.getHeight() - 30); };
    }

    // ---------------- panneau lateral : tuiles de modules + master + remises a zero + entree/sortie
    for (int n : kTileNodes)
    {
        auto b = std::make_unique<juce::TextButton> (nodeName (n));
        b->setClickingTogglesState (false);
        b->getProperties().set ("modCol", (juce::int64) moduleColour (n).getARGB());
        b->onClick = [this, n] { if (proc.engine.inRack (n)) proc.engine.removeFromRack (n); else proc.engine.addToRack (n); refreshRack(); };
        side.addAndMakeVisible (*b);
        tiles.push_back (std::move (b));
    }
    side.subtitles.push_back ({ "MASTER", 286 });
    side.subtitles.push_back ({ "RESET", 516 });
    for (int n = 0; n < kNumNodes; ++n)                                                   // toutes les prises vivent dans le panneau patchbay
    {
        if (n != NOut) patchPanel.audioOut[n] = patchBay.makeJack (patchPanel, JackKind::AudioOut, n);
        if (n != NIn)  patchPanel.audioIn[n]  = patchBay.makeJack (patchPanel, JackKind::AudioIn, n);
    }
    for (int s = 0; s < kNumSrc; ++s) patchPanel.cvOut[s] = patchBay.makeJack (patchPanel, JackKind::CvOut, s);
    for (int d = 0; d < kNumCv; ++d)  patchPanel.cvIn[d]  = patchBay.makeJack (patchPanel, JackKind::CvIn, d);
    mk (side, "mix", "DRY / WET", "0", "10", CvMix);
    mk (side, "output", "OUTPUT", "0", "10");
    side.addAndMakeVisible (foot);
    footAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "on", foot);

    clearRackBtn.setButtonText ("VIDER LE RACK");
    clearPatchBtn.setButtonText ("VIDER LE PATCHBAY");
    resetBtn.setButtonText (fr ("RESET R\xc3\x89" "GLAGES"));
    for (auto* b : { &clearRackBtn, &clearPatchBtn, &resetBtn }) side.addAndMakeVisible (*b);
    clearRackBtn.onClick  = [this] { confirm ("VIDER LE RACK", fr ("Retirer tous les modules du rack ?\nLes r\xc3\xa9" "glages sont conserv\xc3\xa9s."), [this] { proc.clearRack(); refreshRack(); }); };
    clearPatchBtn.onClick = [this] { confirm ("VIDER LE PATCHBAY", "Retirer tous les cables du patchbay ?", [this] { proc.clearPatchbay(); patchBay.repaint(); }); };
    resetBtn.onClick      = [this] { confirm (fr ("RESET DES R\xc3\x89" "GLAGES"), fr ("Remettre tous les r\xc3\xa9" "glages des modules \xc3\xa0 leur valeur par d\xc3\xa9" "faut ?\nLe rack et les c\xc3\xa2" "bles sont conserv\xc3\xa9s."), [this] { proc.resetModules(); }); };
}

// ---------------------------------------------------------------- rack <-> moteur
void ZephyrEditor::rackChangedByUser (const std::vector<int>& newOrder)
{
    proc.engine.setRack (newOrder);
    patchBay.repaint();
}

void ZephyrEditor::refreshRack()
{
    std::vector<ModuleUnit*> list;
    for (int n : proc.engine.getRack())
    {
        auto it = rackUnits.find (n);
        if (it != rackUnits.end()) list.push_back (it->second.get());
    }
    rackView.setUnits (list);
    patchPanel.layoutJacks();
    for (size_t i = 0; i < tiles.size(); ++i) tiles[i]->setToggleState (proc.engine.inRack (kTileNodes[i]), juce::dontSendNotification);
    patchBay.repaint();
    lastRackHash = 0;
    for (auto& s : proc.engine.rackSlot) lastRackHash = lastRackHash * 31u + (uint32_t) (s.load() + 2);
}

void ZephyrEditor::afterPreset()
{
    nameBtn.setButtonText (proc.presetName);
    refreshRack();
    patchBay.repaint();
}

// ---------------------------------------------------------------- presets
void ZephyrEditor::stepPreset (int dir)
{
    juce::StringArray all;
    const int nf = ZephyrAudioProcessor::getNumFactoryPresets();
    for (int i = 0; i < nf; ++i) all.add (ZephyrAudioProcessor::getFactoryPresetName (i));
    const auto user = proc.getUserPresetNames();
    all.addArray (user);
    int cur = all.indexOf (proc.presetName);
    cur = cur < 0 ? (dir > 0 ? -1 : 0) : cur;
    const int next = (cur + dir + all.size()) % juce::jmax (1, all.size());
    if (next < nf) proc.loadFactoryPreset (next); else proc.loadUserPreset (all[next]);
    afterPreset();
}

void ZephyrEditor::presetMenu()
{
    juce::PopupMenu m;
    const auto cats = ZephyrAudioProcessor::getCategories();
    const int nf = ZephyrAudioProcessor::getNumFactoryPresets();
    m.addSectionHeader ("FACTORY");
    m.addItem (100, "Init", true, proc.presetName == "Init");
    for (const auto& c : cats)
    {
        juce::PopupMenu sub;
        int cnt = 0;
        for (int i = 1; i < nf; ++i)
            if (ZephyrAudioProcessor::getFactoryPresetCategory (i) == c)
            {
                const auto nm = ZephyrAudioProcessor::getFactoryPresetName (i);
                sub.addItem (100 + i, nm, true, nm == proc.presetName); ++cnt;
            }
        if (cnt > 0) m.addSubMenu (c.toUpperCase() + "  (" + juce::String (cnt) + ")", sub, true, nullptr, proc.presetCategory == c);
    }
    const auto user = proc.getUserPresetNames();
    if (user.size() > 0)
    {
        m.addSeparator();
        m.addSectionHeader ("USER");
        juce::StringArray ucats;
        for (const auto& u : user) { const auto c = proc.getUserPresetCategory (u); if (! ucats.contains (c)) ucats.add (c); }
        ucats.sort (true);
        for (const auto& c : ucats)
        {
            juce::PopupMenu sub;
            for (int i = 0; i < user.size(); ++i)
                if (proc.getUserPresetCategory (user[i]) == c) sub.addItem (1000 + i, user[i], true, user[i] == proc.presetName);
            m.addSubMenu ((c.isEmpty() ? juce::String ("SANS CATEGORIE") : c.toUpperCase()), sub);
        }
    }
    m.addSeparator();
    m.addItem (1, "Save as...");
    if (user.contains (proc.presetName))
    {
        m.addItem (2, "Overwrite \"" + proc.presetName + "\"");
        m.addItem (3, "Delete \"" + proc.presetName + "\"");
    }
    m.addItem (4, "Open preset folder");

    juce::Component::SafePointer<ZephyrEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameBtn), [safe, user] (int id)
    {
        if (safe == nullptr || id == 0) return;
        auto& pr = safe->proc;
        if (id >= 1000)      { pr.loadUserPreset (user[id - 1000]); safe->afterPreset(); }
        else if (id >= 100)  { pr.loadFactoryPreset (id - 100); safe->afterPreset(); }
        else if (id == 1)    safe->saveDialog();
        else if (id == 2)    { pr.saveUserPreset (pr.presetName, pr.presetCategory); safe->afterPreset(); }
        else if (id == 3)
        {
            const auto name = pr.presetName;
            safe->confirm ("Supprimer le preset", "Supprimer definitivement \"" + name + "\" ?", [safe, name]
            {
                if (safe != nullptr) { safe->proc.deleteUserPreset (name); safe->proc.presetName = "Init"; safe->afterPreset(); }
            });
        }
        else if (id == 4)    { pr.getPresetDir().createDirectory(); pr.getPresetDir().revealToUser(); }
    });
}

void ZephyrEditor::showDialog (const juce::String& title, const juce::String& message, bool askText, const juce::String& initial,
                                            const juce::String& okText, std::function<void (bool, const juce::String&)> cb)
{
    juce::Component* host = dialogHost != nullptr ? dialogHost : this;
    dialog = std::make_unique<ZephyrDialog> (title, message, askText, initial, okText, std::move (cb));
    juce::Component::SafePointer<ZephyrEditor> safe (this);
    dialog->onFinished = [safe]
    {
        juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->dialog.reset(); });
    };
    host->addAndMakeVisible (*dialog);
    dialog->setBounds (host->getLocalBounds());
    dialog->toFront (true);
    dialog->focus();
}

void ZephyrEditor::saveDialog()
{
    juce::Component::SafePointer<ZephyrEditor> safe (this);
    juce::PopupMenu m;
    m.addSectionHeader ("CATEGORIE DU PRESET");
    const auto cats = ZephyrAudioProcessor::getCategories();
    for (int i = 0; i < cats.size(); ++i) m.addItem (10 + i, cats[i], true, proc.presetCategory == cats[i]);
    m.addSeparator();
    m.addItem (9, "Sans categorie");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameBtn), [safe, cats] (int id)
    {
        if (safe == nullptr || id == 0) return;
        const juce::String cat = id >= 10 ? cats[id - 10] : juce::String();
        safe->showDialog ("SAUVEGARDER LE PRESET", "Nom du preset" + (cat.isEmpty() ? juce::String() : " (" + cat + ")") + " :", true, safe->proc.presetName, "SAUVEGARDER",
                          [safe, cat] (bool ok, const juce::String& text)
        {
            if (ok && safe != nullptr) { safe->proc.saveUserPreset (text, cat); safe->afterPreset(); }
        });
    });
}

void ZephyrEditor::confirm (const juce::String& title, const juce::String& message, std::function<void()> action)
{
    showDialog (title, message, false, {}, "OK", [action] (bool ok, const juce::String&) { if (ok) action(); });
}

void ZephyrEditor::timerCallback()
{
    glowShown += 0.25f * (proc.engine.glow.load() - glowShown);
    brightShown += 0.2f * (proc.engine.bright.load() - brightShown);
    repaint (lampWindow().expanded (8.f).toNearestInt());
    if (const int ps = proc.presetSerial.load(); ps != lastPresetSerial) { lastPresetSerial = ps; afterPreset(); }
    patchBay.repaintLeds();
    {
        const int v = proc.lastMidiMsg.load(), kind = v >> 16, a = (v >> 8) & 255, b = v & 255;
        juce::String s = "IN: -";
        if (kind == 1) s = "IN: NOTE " + juce::String (a) + " v" + juce::String (b);
        else if (kind == 2) s = "IN: CC" + juce::String (a) + " = " + juce::String (b);
        else if (kind == 3) s = "IN: PITCH BEND";
        else if (kind == 4) s = "IN: AFTERTOUCH " + juce::String (b);
        s += "  |  WHEEL " + juce::String (proc.wheelShown.load());
        if (midiMon.getText() != s) midiMon.setText (s, juce::dontSendNotification);
        midiMon.setColour (juce::Label::textColourId, look::print.withAlpha (0.9f));
    }

    if (vu != nullptr) { vu->setValue (proc.engine.compGr.load()); vu->tick(); }

    const uint32_t h = patchBay.stateHash();                                  // cables : on ne redessine que si quelque chose a bouge
    if (h != lastPatchHash) { lastPatchHash = h; patchBay.repaint(); }
    for (auto& k : knobs)
        if (k->cvIndex >= 0) k->setCvActive (proc.engine.cvIn[k->cvIndex].load() != 0);

    if (const uint32_t bm = proc.engine.bypassMask.load(); bm != lastBypass) { lastBypass = bm; for (auto& kv : rackUnits) kv.second->repaint(); }
    uint32_t rh = 0;
    for (auto& s : proc.engine.rackSlot) rh = rh * 31u + (uint32_t) (s.load() + 2);
    if (rh != lastRackHash) refreshRack();

    for (auto& l : syncLinks)                                                     // le potard RATE / TIME est inutile quand la synchro est active
    {
        const bool free = l.box->getSelectedItemIndex() <= 0;
        if (free != l.lastFree)
        {
            l.lastFree = free;
            l.rate->slider.setEnabled (free);
            l.rate->setAlpha (free ? 1.f : 0.5f);
            l.rate->repaint();
        }
    }
}

// ---------------------------------------------------------------- mise en page
void ZephyrEditor::resized()
{
    const int W = getWidth(), H = getHeight();
    headerArea = { 0, 0, W, kHeaderH };

    // ---- en-tete : presets
    prevBtn.setBounds (W - 400, 36, 30, 28);
    nameBtn.setBounds (W - 364, 36, 220, 28);
    nextBtn.setBounds (W - 140, 36, 30, 28);
    saveBtn.setBounds (W - 104, 36, 70, 28);
    cablesBtn.setBounds (W - 540, 38, 108, 24);
    themeBtn.setBounds (W - 660, 38, 110, 24);
    themeBtn.setButtonText (look::T().name);
    midiBtn.setBounds (W - 660, 8, 110, 24);
    midiMon.setBounds (W - 540, 8, 130, 24);
    settingsBtn.setBounds (W - 364, 8, 110, 24);

    // ---- colonnes : panneau lateral | rack | (bandeau de modulation en bas)
    const int vpW = W - 170 - 310;
    const int top = kHeaderH + 8;
    patchPanel.setBounds (W - 310, top, 300, H - top - 10);
    side.setBounds (10, top, 150, H - top - 10);
    viewport.setBounds (170, top, vpW - 10, H - top - 8 - 172 - 8);
    rackView.setSize (viewport.getMaximumVisibleWidth(), juce::jmax (90, rackView.getHeight()));
    rackView.relayout();
    patchBay.setBounds (getLocalBounds());
    patchBay.toFront (false);

    // ---- bandeau de modulation
    {
        const int y = H - 8 - 172, gap = 8;
        const float weights[4] { 3.f, 3.f, 3.f, 2.6f };
        const int usable = vpW - 10 - 3 * gap;
        int x = 170;
        for (size_t i = 0; i < stripUnits.size() && i < 4; ++i)
        {
            const int w = juce::roundToInt ((float) usable * weights[i] / 11.6f);
            stripUnits[i]->setBounds (x, y, w, 172);
            x += w + gap;
        }
    }

    // ---- panneau lateral
    for (size_t i = 0; i < tiles.size(); ++i) tiles[i]->setBounds (12, 34 + (int) i * 25, 126, 22);
    knobById.at ("mix")->setBounds (8, 308, 67, 94);
    knobById.at ("output")->setBounds (75, 308, 67, 94);
    foot.setBounds (46, 414, 58, 90);
    clearRackBtn.setBounds (12, 544, 126, 28);
    clearPatchBtn.setBounds (12, 578, 126, 28);
    resetBtn.setBounds (12, 612, 126, 28);

    rebuildFacade();
}

void ZephyrEditor::rebuildFacade()
{
    const int W = getWidth(), H = getHeight();
    const auto& th = look::T();
    paintFacade (panelTex, W, H, true);
    juce::Graphics g (panelTex);
    const juce::Colour ink (th.print), acc (th.accent);
    // logotype : capitales larges, espacees, avec une ombre gravee
    g.setFont (font (44.f).withExtraKerningFactor (0.22f));
    g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.08f : 0.7f));
    g.drawText (titleText, 33, 4, 420, 50, juce::Justification::centredLeft, false);
    g.setColour (ink);
    g.drawText (titleText, 32, 3, 420, 50, juce::Justification::centredLeft, false);
    g.setGradientFill (juce::ColourGradient (acc, 34.f, 0.f, acc.withAlpha (0.f), 300.f, 0.f, false));
    g.fillRect (34, 54, 260, 2);
    g.setColour (ink.withAlpha (0.85f));
    g.setFont (font (10.5f).withExtraKerningFactor (0.12f));
    g.drawText (subText, 34, 59, 420, 13, juce::Justification::centredLeft, false);
    g.setFont (font (9.f, false).withExtraKerningFactor (0.1f));
    g.setColour (acc);
    g.drawText ("FOUDRE AUDIO  -  LIGHT, AIR AND VOLTAGE", 34, 71, 360, 11, juce::Justification::centredLeft, false);
    g.setColour (ink.withAlpha (0.8f));
    g.setFont (font (9.5f));
    g.drawText ("PRESET", W - 400, 26, 80, 13, juce::Justification::centredLeft, false);
    repaint();
}

void ZephyrEditor::cycleTheme()
{
    look::applyTheme ((look::themeIdx + 1) % look::kNumThemes);
    saveThemePref (look::themeIdx);
    ZephyrLookAndFeel::refreshAll();
    themeBtn.setButtonText (look::T().name);
    if (auto* top = getTopLevelComponent()) top->repaint();
    rebuildFacade();
    if (onThemeChanged) onThemeChanged();
}

void ZephyrEditor::mouseUp (const juce::MouseEvent& e)
{
    if (lampWindow().contains (e.position) && ! e.mouseWasDraggedSinceMouseDown())
    {
        proc.headerScope.store (proc.headerScope.load() == 0 ? 1 : 0);
        repaint (headerArea);
    }
}

// ---------------------------------------------------------------- le prisme : un faisceau blanc entre, un eventail de couleurs sort.
// L'intensite suit le niveau de sortie, l'ouverture de l'eventail et l'eclat suivent la brillance (part des aigus).
void ZephyrEditor::drawPrism (juce::Graphics& g, juce::Rectangle<float> r, float glow, float bright)
{
    const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101a2c), r.getCentreX(), r.getCentreY(), juce::Colour (0xff03060b), r.getX(), r.getY(), true));
    g.fillRect (r);
    const float L = 0.12f + 0.88f * glow;                                               // jamais tout a fait eteint : une veilleuse
    const float cx = r.getCentreX() - 6.f, cy = r.getCentreY() + 4.f, s = r.getHeight() * 0.42f;
    const juce::Point<float> top (cx, cy - s), bl (cx - s * 0.95f, cy + s * 0.7f), br (cx + s * 0.95f, cy + s * 0.7f);
    const auto entry = top + (bl - top) * 0.55f, exitP = top + (br - top) * 0.52f;

    // faisceau d'entree
    const juce::Point<float> src (r.getX() - 4.f, entry.y + 10.f);
    for (int k = 0; k < 3; ++k)
    {
        g.setColour (juce::Colours::white.withAlpha (L * (k == 0 ? 0.12f : k == 1 ? 0.25f : 0.85f)));
        g.drawLine ({ src, entry }, k == 0 ? 7.f : k == 1 ? 3.5f : 1.3f);
    }
    // eventail spectral
    static const juce::uint32 spec[7] { 0xffff3b3b, 0xffff8a2a, 0xffffe14a, 0xff52e06a, 0xff3fc4ff, 0xff4a6bff, 0xffa24aff };
    const float spread = 0.10f + 0.55f * bright;
    for (int i = 0; i < 7; ++i)
    {
        const float a = -0.20f + spread * ((float) i / 6.f - 0.15f);
        const float shimmer = 0.82f + 0.18f * (float) std::sin (t * (1.7 + 0.37 * i) + i * 1.3);
        const juce::Point<float> end (r.getRight() + 6.f, exitP.y + std::tan (a) * (r.getRight() - exitP.x) + 10.f);
        juce::Path ray;
        const float w0 = 0.8f, w1 = 2.5f + 5.f * bright;
        const auto dir = (end - exitP); const float len = dir.getDistanceFromOrigin();
        const juce::Point<float> nrm (-dir.y / len, dir.x / len);
        ray.startNewSubPath (exitP + nrm * w0); ray.lineTo (end + nrm * w1); ray.lineTo (end - nrm * w1); ray.lineTo (exitP - nrm * w0); ray.closeSubPath();
        const juce::Colour col (spec[i]);
        g.setGradientFill (juce::ColourGradient (col.withAlpha (L * 0.95f * shimmer), exitP.x, exitP.y, col.withAlpha (L * 0.35f * shimmer), end.x, end.y, false));
        g.fillPath (ray);
    }
    // halo a la sortie
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.35f * L), exitP.x, exitP.y, juce::Colours::white.withAlpha (0.f), exitP.x + 26.f, exitP.y, true));
    g.fillEllipse (juce::Rectangle<float> (52.f, 52.f).withCentre (exitP));
    // le verre du prisme
    juce::Path tri; tri.addTriangle (top, bl, br);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f + 0.15f * L), top.x, top.y, juce::Colour (0xff8fc7ff).withAlpha (0.08f + 0.1f * L), br.x, br.y, false));
    g.fillPath (tri);
    {
        juce::Graphics::ScopedSaveState ss (g);                                         // lumiere qui traverse le verre
        g.reduceClipRegion (tri);
        g.setColour (juce::Colours::white.withAlpha (0.35f * L));
        g.drawLine ({ entry, exitP }, 3.f);
        juce::Random rr (7);
        for (int k = 0; k < 6; ++k)                                                     // eclats internes
        {
            const float ph = (float) std::fmod (t * 0.6 + rr.nextFloat(), 1.0);
            const float sp = 0.5f + 0.5f * std::sin (ph * 6.2831853f);
            const juce::Point<float> q (bl.x + (br.x - bl.x) * rr.nextFloat(), top.y + (bl.y - top.y) * (0.4f + 0.6f * rr.nextFloat()));
            g.setColour (juce::Colours::white.withAlpha (sp * 0.5f * L * (0.3f + bright)));
            g.fillEllipse (juce::Rectangle<float> (2.f, 2.f).withCentre (q));
        }
    }
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.strokePath (tri, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawLine ({ top, top + (bl - top) * 0.25f }, 1.6f);                                // arete eclairee
}

void ZephyrEditor::drawScope (juce::Graphics& g, juce::Rectangle<float> scr)
{
    const auto& th = look::T();
    const juce::Colour ph (th.scope);
    g.setGradientFill (juce::ColourGradient (ph.withAlpha (0.10f), scr.getCentreX(), scr.getCentreY(), juce::Colour (0xff02040a), scr.getX(), scr.getY(), true));
    g.fillRect (scr);
    g.setColour (ph.withAlpha (0.10f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (scr.getY() + scr.getHeight() * (float) i / 4.f), scr.getX(), scr.getRight());
    for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (scr.getX() + scr.getWidth() * (float) i / 8.f), scr.getY(), scr.getBottom());
    g.setColour (ph.withAlpha (0.2f));
    g.drawHorizontalLine ((int) scr.getCentreY(), scr.getX(), scr.getRight());

    static float buf[4096];
    proc.copyScope (buf, 4096);
    // declenchement : passage par zero montant ; l'affichage couvre ~2,5 periodes
    int first = -1, last = -1, count = 0;
    for (int i = 40; i < 4096 - 8; ++i)
        if (buf[i - 1] < -0.004f && buf[i] >= 0.f) { if (first < 0) first = i; last = i; ++count; }
    int span = 1024, start = 4096 - 1024;
    if (count >= 2)
    {
        const float period = (float) (last - first) / (float) (count - 1);
        span = juce::jlimit (96, 2000, juce::roundToInt (period * 2.5f));
        start = first;
        for (int i = last; i >= first; --i) if (buf[i - 1] < -0.004f && buf[i] >= 0.f && i + span < 4096) { start = i; break; }
    }
    float pk = 0.f;
    for (int i = 0; i < span; ++i) pk = juce::jmax (pk, std::abs (buf[start + i]));
    const float target = pk > 0.003f ? juce::jmin (6.f, 0.85f / pk) : 1.f;
    scopeGain += (target - scopeGain) * 0.15f;
    juce::Path p;
    const int nx = (int) scr.getWidth();
    for (int x = 0; x <= nx; ++x)
    {
        const float pos = (float) x / (float) nx * (float) (span - 1);
        const int i0 = (int) pos; const float fr2 = pos - (float) i0;
        const float v = buf[start + i0] + (buf[start + juce::jmin (i0 + 1, span - 1)] - buf[start + i0]) * fr2;
        const float py = scr.getCentreY() - juce::jlimit (-1.f, 1.f, v * scopeGain) * scr.getHeight() * 0.44f;
        if (x == 0) p.startNewSubPath (scr.getX(), py); else p.lineTo (scr.getX() + (float) x, py);
    }
    g.setColour (ph.withAlpha (0.16f)); g.strokePath (p, juce::PathStrokeType (5.f, juce::PathStrokeType::curved));
    g.setColour (ph.withAlpha (0.35f)); g.strokePath (p, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved));
    g.setColour (ph.brighter (0.5f));   g.strokePath (p, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved));
    g.setColour (ph.withAlpha (0.6f));
    g.setFont (font (8.5f));
    g.drawText ("OSC", scr.withTrimmedLeft (5.f).withHeight (12.f).translated (0.f, scr.getHeight() - 13.f), juce::Justification::centredLeft, false);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), scr.getX(), scr.getY(), juce::Colours::transparentWhite, scr.getX(), scr.getCentreY(), false));
    g.fillRect (scr);
}

void ZephyrEditor::paint (juce::Graphics& g)
{
    const auto& th = look::T();
    g.drawImageAt (panelTex, 0, 0);

    const auto win = lampWindow();                       // une seule fenetre : prisme (lumiere du son) ou oscilloscope (clic)
    g.setColour (juce::Colours::black.withAlpha (th.dark ? 0.55f : 0.25f));
    g.fillRoundedRectangle (win.expanded (4.f).translated (0.f, 2.f), 9.f);
    juce::ColourGradient bez (juce::Colour (0xfff4f6f9), win.getX(), win.getY(), juce::Colour (0xff6d7580), win.getRight(), win.getBottom(), false);
    bez.addColour (0.5, juce::Colour (0xffb8c0ca));
    g.setGradientFill (bez);
    g.fillRoundedRectangle (win.expanded (4.f), 8.f);                                 // lunette en aluminium tourne
    g.setColour (juce::Colour (0xff03050a));
    g.fillRoundedRectangle (win, 6.f);
    const bool scope = proc.headerScope.load() == 1;
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (win.reduced (2.f), 5.f);
        g.reduceClipRegion (clip);
        if (scope) drawScope (g, win.reduced (2.f));
        else       drawPrism (g, win.reduced (2.f), juce::jlimit (0.f, 1.f, glowShown), juce::jlimit (0.f, 1.f, brightShown));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), win.getX(), win.getY(), juce::Colours::transparentWhite, win.getX() + win.getWidth() * 0.4f, win.getCentreY(), false));
        g.fillRect (win);                                                              // reflet du verre
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (win.reduced (0.5f), 6.f, 1.2f);
    g.setColour (look::print.withAlpha (0.9f));
    g.setFont (font (10.f).withExtraKerningFactor (0.1f));
    g.drawText (scope ? "SCOPE" : "PRISM", (int) win.getRight() + 12, (int) win.getCentreY() - 14, 70, 14, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (th.accent));
    g.setFont (font (8.5f, false));
    g.drawText ("click to switch", (int) win.getRight() + 12, (int) win.getCentreY(), 80, 12, juce::Justification::centredLeft, false);
}
