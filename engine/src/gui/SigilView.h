#pragma once
#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/OrganismEditor.h"
#include "gui/UiTicker.h"
#include "hum/Capabilities.h"

namespace hum {

class SigilView : public OrganismEditor, private juce::Timer {
public:
    SigilView(EngineHost& host, std::string name)
        : host_(host), name_(std::move(name)) {
        tickerId_ = UiTicker::instance().add([this] { timerCallback(); });
    }
    ~SigilView() override { UiTicker::instance().remove(tickerId_); }

    void reloadValues() override {}
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 60; }
    int preferredContentHeight(int) const override { return 84; }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds();
        g.setColour(Palette::background.darker(0.35f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.0f, 1.0f);

        auto* src = dynamic_cast<SigilSource*>(host_.liveOrganism(name_));
        if (src == nullptr) return;
        const double t = frozenClock() >= 0.0
                             ? frozenClock()
                             : juce::Time::getMillisecondCounterHiRes() * 0.001;
        const int n = src->sigil(prims_.data(), (int) prims_.size(), t);
        if (n <= 0) return;

        juce::Colour fam = Palette::accent;
        if (const auto* cm = host_.model().byName(name_))
            fam = Palette::familyAccent(familyOf(cm->displayClass));
        juce::Image img(juce::Image::ARGB, kGrid, kGrid, true);
        {
            juce::Graphics ig(img);
            for (int i = 0; i < n; ++i) drawPrim(ig, prims_[(size_t) i], fam);
        }
        const auto cell = r.reduced(3).toFloat();
        const float side = juce::jmin(cell.getWidth(), cell.getHeight());
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.drawImage(img, juce::Rectangle<float>(side, side).withCentre(cell.getCentre()));
    }

    static constexpr int kGrid = 40;

    static double& frozenClock() {
        static double t = -1.0;
        return t;
    }

    static juce::Colour roleColour(unsigned char role, juce::Colour accent) {
        switch (role) {
            case 0: return Palette::textDim.withAlpha(0.55f);
            case 2: return accent;
            case 3: return accent.brighter(0.6f);
            default: return Palette::text.withAlpha(0.85f);
        }
    }

    static void drawPrim(juce::Graphics& g, const SigilSource::Prim& p, juce::Colour accent) {
        const float a = juce::jlimit(0.0f, 1.0f, p.alpha);
        if (a <= 0.004f) return;
        g.setColour(roleColour(p.role, accent).withMultipliedAlpha(a));
        const float s = (float) kGrid;
        if (p.kind == 1) {
            const float rad = juce::jmax(0.6f, p.size * s);
            g.fillEllipse(p.pt[0][0] * s - rad, p.pt[0][1] * s - rad, rad * 2.0f, rad * 2.0f);
            return;
        }
        const int pts = juce::jlimit(0, SigilSource::kMaxPoints, p.points);
        if (pts < 2) return;
        juce::Path path;
        path.startNewSubPath(p.pt[0][0] * s, p.pt[0][1] * s);
        for (int i = 1; i < pts; ++i) path.lineTo(p.pt[i][0] * s, p.pt[i][1] * s);
        if (p.closed) path.closeSubPath();
        if (p.filled) g.fillPath(path);
        else g.strokePath(path, juce::PathStrokeType(juce::jmax(0.8f, p.size * s)));
    }

private:
    void timerCallback() override {
        if (dynamic_cast<SigilSource*>(host_.liveOrganism(name_)) != nullptr) repaint();
    }

    EngineHost& host_;
    int tickerId_ = 0;
    std::string name_;
    std::array<SigilSource::Prim, 48> prims_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SigilView)
};

}
