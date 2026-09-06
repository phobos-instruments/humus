#pragma once
#include <cmath>
#include <map>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Categories.h"
#include "gui/EngineHost.h"
#include "gui/HelpDocs.h"
#include "gui/LookAndFeel.h"
#include "gui/SigilView.h"

namespace hum {

namespace collar {

constexpr int kHeight = 30;

inline std::string speciesNameFor(const std::string& displayClass) {
    const auto names = helpCandidateNames(displayClass);
    return names.empty() ? displayClass : names.back();
}

inline const juce::Image& substrateFor(Family f, int w, int h) {
    static std::map<juce::int64, juce::Image> cache;
    const auto fam = Palette::familyAccent(f);
    const juce::int64 key = ((juce::int64) (int) f << 56) ^ ((juce::int64) w << 36)
                          ^ ((juce::int64) h << 16) ^ (juce::int64) (juce::uint32) fam.getARGB();
    if (auto it = cache.find(key); it != cache.end()) return it->second;

    juce::Image img(juce::Image::ARGB, juce::jmax(1, w), juce::jmax(1, h), true);
    juce::Graphics ig(img);
    juce::Random rng(0x501Fu + (juce::int64) (int) f * 977);
    const float fw = (float) w, fh = (float) h;
    ig.setColour(fam.withAlpha(0.05f));
    switch (f) {
        case Family::Voice: {
            const int roots = juce::jmax(4, w / 46);
            for (int i = 0; i < roots; ++i) {
                juce::Path p;
                float x = fw * (0.5f + i) / (float) roots + rng.nextFloat() * 14.0f - 7.0f;
                float y = fh;
                p.startNewSubPath(x, y);
                const float rise = fh * (0.35f + rng.nextFloat() * 0.45f);
                while (y > fh - rise) {
                    y -= 9.0f + rng.nextFloat() * 8.0f;
                    x += rng.nextFloat() * 8.0f - 4.0f;
                    p.lineTo(x, y);
                }
                ig.strokePath(p, juce::PathStrokeType(1.0f));
            }
            break;
        }
        case Family::Time: {
            const float cx = fw * 1.08f, cy = -fh * 0.15f;
            const float reach = std::sqrt(fw * fw + fh * fh) * 1.15f;
            for (float rad = 26.0f; rad < reach; rad += 26.0f + rng.nextFloat() * 8.0f) {
                juce::Path ring;
                ring.addCentredArc(cx, cy, rad, rad, 0.0f, 2.4f, 4.6f, true);
                ig.strokePath(ring, juce::PathStrokeType(1.0f));
            }
            break;
        }
        case Family::Motion: {
            const int strands = juce::jmax(3, h / 60);
            for (int i = 0; i < strands; ++i) {
                float x = -4.0f, y = fh * rng.nextFloat();
                juce::Path p;
                p.startNewSubPath(x, y);
                while (x < fw) {
                    x += 14.0f + rng.nextFloat() * 12.0f;
                    y += rng.nextFloat() * 18.0f - 9.0f;
                    p.lineTo(x, y);
                    if (rng.nextFloat() < 0.25f) {
                        juce::Path b;
                        b.startNewSubPath(x, y);
                        b.lineTo(x + 10.0f + rng.nextFloat() * 10.0f,
                                 y + (rng.nextBool() ? 12.0f : -12.0f));
                        ig.strokePath(b, juce::PathStrokeType(1.0f));
                    }
                }
                ig.strokePath(p, juce::PathStrokeType(1.0f));
            }
            break;
        }
        case Family::Sense: {
            for (float y = 14.0f; y < fh; y += 30.0f)
                for (float x = 12.0f; x < fw; x += 34.0f) {
                    const float jx = x + rng.nextFloat() * 16.0f - 8.0f;
                    const float jy = y + rng.nextFloat() * 12.0f - 6.0f;
                    const float r = 0.9f + rng.nextFloat() * 1.1f;
                    ig.fillEllipse(jx - r, jy - r, r * 2.0f, r * 2.0f);
                }
            break;
        }
        case Family::Utility: break;
    }
    return cache.emplace(key, std::move(img)).first->second;
}

inline const juce::Image& soilFor(Family f, int w, int h) {
    static std::map<juce::int64, juce::Image> cache;
    const auto fam = f == Family::Utility ? Palette::border.brighter(0.25f)
                                          : Palette::familyAccent(f);
    const juce::int64 key = ((juce::int64) (int) f << 56) ^ ((juce::int64) w << 36)
                          ^ ((juce::int64) h << 16)
                          ^ (juce::int64) (juce::uint32) fam.getARGB()
                          ^ ((juce::int64) (juce::uint32) Palette::panel.getARGB() << 8);
    if (auto it = cache.find(key); it != cache.end()) return it->second;

    juce::Image img(juce::Image::RGB, juce::jmax(1, w), juce::jmax(1, h), false);
    juce::Graphics ig(img);
    const auto r = juce::Rectangle<float>(0.0f, 0.0f, (float) w, (float) h);
    juce::ColourGradient soil(Palette::panel.brighter(0.07f),
                              r.getWidth() * 0.30f, r.getHeight() * 0.12f,
                              Palette::panel.darker(0.22f),
                              r.getRight(), r.getBottom(), true);
    soil.addColour(0.55, Palette::panel.darker(0.04f));
    ig.setGradientFill(soil);
    ig.fillRect(r);
    if (f != Family::Utility) ig.drawImageAt(substrateFor(f, w, h), 0, 0);
    juce::ColourGradient hairline(fam.withAlpha(0.85f), 0.0f, 0.0f,
                                  fam.withAlpha(0.0f), r.getRight(), 0.0f, false);
    ig.setGradientFill(hairline);
    ig.fillRect(0.0f, 0.0f, r.getWidth(), 1.0f);
    return cache.emplace(key, std::move(img)).first->second;
}

inline void paintSoil(juce::Graphics& g, juce::Rectangle<float> r, Family f) {
    g.drawImageAt(soilFor(f, (int) r.getWidth(), (int) r.getHeight()),
                  (int) r.getX(), (int) r.getY());
}

inline const juce::Image& grownMark(const std::string& species, Family f) {
    static std::map<juce::int64, juce::Image> cache;
    juce::int64 seed = 1469598103934665603LL;
    for (const char ch : species) seed = (seed ^ (juce::int64) (unsigned char) ch) * 1099511628211LL;
    const auto fam = f == Family::Utility ? Palette::textDim : Palette::familyAccent(f);
    const juce::int64 key = seed ^ (juce::int64) (juce::uint32) fam.getARGB();
    if (auto it = cache.find(key); it != cache.end()) return it->second;

    const int G = SigilView::kGrid;
    juce::Image img(juce::Image::ARGB, G, G, true);
    juce::Graphics ig(img);
    juce::Random rng(seed);
    const float s = (float) G;

    const float lean = (rng.nextFloat() - 0.5f) * 0.34f;
    const float curl = (rng.nextFloat() - 0.5f) * 0.5f;
    const int stemPts = 7;
    float sx[stemPts], sy[stemPts];
    juce::Path stem;
    for (int i = 0; i < stemPts; ++i) {
        const float t = (float) i / (float) (stemPts - 1);
        sx[i] = (0.5f + lean * t + curl * t * t) * s;
        sy[i] = (0.92f - 0.74f * t) * s;
        if (i == 0) stem.startNewSubPath(sx[i], sy[i]);
        else stem.lineTo(sx[i], sy[i]);
    }
    ig.setColour(Palette::text.withAlpha(0.8f));
    ig.strokePath(stem, juce::PathStrokeType(s * 0.05f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    const int branches = 2 + rng.nextInt(3);
    for (int b = 0; b < branches; ++b) {
        const int at = 2 + rng.nextInt(stemPts - 3);
        const float side = (b % 2 == 0) ? 1.0f : -1.0f;
        const float len = s * (0.16f + rng.nextFloat() * 0.14f);
        const float tipX = sx[at] + side * len;
        const float tipY = sy[at] - len * (0.2f + rng.nextFloat() * 0.5f);
        ig.setColour(Palette::text.withAlpha(0.55f));
        ig.drawLine(sx[at], sy[at], tipX, tipY, s * 0.04f);
        const float dr = s * (0.045f + rng.nextFloat() * 0.02f);
        ig.setColour(Palette::textDim.withAlpha(0.9f));
        ig.fillEllipse(tipX - dr, tipY - dr, dr * 2.0f, dr * 2.0f);
    }
    const float tipR = s * 0.075f;
    ig.setColour(fam);
    ig.fillEllipse(sx[stemPts - 1] - tipR, sy[stemPts - 1] - tipR, tipR * 2.0f, tipR * 2.0f);

    return cache.emplace(key, std::move(img)).first->second;
}

inline void paintFace(juce::Graphics& g, EngineHost& host, const std::string& organism,
                      const std::string& species, Family f, juce::Rectangle<int> face) {
    const auto fam = Palette::familyAccent(f);
    if (auto* src = dynamic_cast<SigilSource*>(host.liveOrganism(organism))) {
        std::array<SigilSource::Prim, 48> prims;
        const int n = src->sigil(prims.data(), (int) prims.size(), 0.0);
        if (n > 0) {
            juce::Image img(juce::Image::ARGB, SigilView::kGrid, SigilView::kGrid, true);
            {
                juce::Graphics ig(img);
                for (int i = 0; i < n; ++i) SigilView::drawPrim(ig, prims[(size_t) i], fam);
            }
            g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
            g.drawImage(img, face.toFloat());
            return;
        }
    }
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
    g.drawImage(grownMark(species, f), face.toFloat());
}

inline bool speciesIsStylised(const juce::String& s) {
    if (s.isEmpty() || !juce::CharacterFunctions::isLowerCase(s[0])) return false;
    for (int i = 1; i < s.length(); ++i)
        if (juce::CharacterFunctions::isUpperCase(s[i])) return true;
    return false;
}

inline juce::Rectangle<int> drawSpeciesCaps(juce::Graphics& g, const std::string& species,
                                            juce::Rectangle<int> area) {
    juce::Font nameFont(juce::FontOptions(11.5f).withStyle("Bold"));
    nameFont.setExtraKerningFactor(0.09f);
    g.setFont(nameFont);
    g.setColour(Palette::text);
    const auto raw = juce::String::fromUTF8(species.c_str());
    const auto caps = speciesIsStylised(raw) ? raw : raw.toUpperCase();
    const int nameW = juce::jmin(area.getWidth(),
                                 (int) std::ceil(juce::GlyphArrangement::getStringWidth(nameFont, caps)) + 2);
    g.drawText(caps, area.removeFromLeft(nameW), juce::Justification::centredLeft, false);
    return area;
}

inline void paint(juce::Graphics& g, EngineHost& host, const std::string& organism,
                  juce::Rectangle<int> r) {
    const auto* cm = host.model().byName(organism);
    if (!cm) return;
    const auto species = speciesNameFor(cm->displayClass);
    const auto f = familyOf(cm->displayClass);

    auto area = r.reduced(8, 0);
    const int cell = kHeight - 8;
    paintFace(g, host, organism, species,
              f, area.removeFromLeft(cell).withSizeKeepingCentre(cell, cell));
    area.removeFromLeft(7);
    drawSpeciesCaps(g, species, area);
}

inline void paintBand(juce::Graphics& g, EngineHost& host, const std::string& organism,
                      juce::Rectangle<int> r, int leftInset, int rightInset) {
    const auto* cm = host.model().byName(organism);
    if (!cm) {
        g.setColour(Palette::text);
        g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        g.drawText(juce::String(organism), r.reduced(leftInset, 0).withTrimmedRight(rightInset),
                   juce::Justification::centredLeft, true);
        return;
    }
    const auto species = speciesNameFor(cm->displayClass);
    const auto f = familyOf(cm->displayClass);
    const auto fam = f == Family::Utility ? Palette::border.brighter(0.25f)
                                          : Palette::familyAccent(f);

    juce::ColourGradient wash(fam.withAlpha(0.05f), 0.0f, (float) r.getY(),
                              fam.withAlpha(0.0f), 0.0f, (float) r.getBottom(), false);
    g.setGradientFill(wash);
    g.fillRect(r);
    juce::ColourGradient hairline(fam.withAlpha(0.85f), (float) r.getX(), 0.0f,
                                  fam.withAlpha(0.0f), (float) r.getRight(), 0.0f, false);
    g.setGradientFill(hairline);
    g.fillRect((float) r.getX(), (float) r.getY(), (float) r.getWidth(), 1.0f);

    auto area = r.withTrimmedLeft(leftInset).withTrimmedRight(rightInset);
    const int cell = kHeight - 8;
    paintFace(g, host, organism, species,
              f, area.removeFromLeft(cell).withSizeKeepingCentre(cell, cell));
    area.removeFromLeft(7);
    area = drawSpeciesCaps(g, species, area);

    const juce::String inst = juce::String::fromUTF8(organism.c_str());
    const juce::String display = juce::String::fromUTF8(cm->displayClass.c_str());
    juce::String token;
    bool renamed = false;
    if (!inst.equalsIgnoreCase(juce::String::fromUTF8(species.c_str()))
        && !inst.equalsIgnoreCase(display)) {
        const juce::String pre = display + "_";
        if (inst.startsWith(pre) && inst.substring(pre.length()).containsOnly("0123456789")) {
            int siblings = 0;
            for (const auto& o : host.model().organisms)
                if (o.displayClass == cm->displayClass) ++siblings;
            if (siblings > 1) token = inst.substring(pre.length());
        } else {
            token = inst;
            renamed = true;
        }
    }
    if (token.isNotEmpty() && area.getWidth() > 24) {
        area.removeFromLeft(6);
        if (renamed) {
            g.setColour(Palette::textDim);
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
            g.drawText(juce::String::fromUTF8("\xc2\xb7"), area.removeFromLeft(7),
                       juce::Justification::centred, false);
            area.removeFromLeft(5);
            g.setColour(Palette::text);
            g.setFont(juce::Font(juce::FontOptions(11.5f)));
        } else {
            g.setColour(Palette::textDim);
            g.setFont(juce::Font(juce::FontOptions(11.0f)));
        }
        g.drawText(token, area, juce::Justification::centredLeft, true);
    }
}

}
}
