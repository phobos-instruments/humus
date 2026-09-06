#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/FpsMeter.h"
#include "gui/PolledBrick.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "hum/Capabilities.h"

namespace hum {

class CamPreview : public PolledBrick {
public:
    CamPreview(EngineHost& host, std::string organism)
        : PolledBrick(host, std::move(organism)) {
        setOpaque(true);
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 312; }
    int preferredContentHeight(int) const override { return 234; }

    void mouseUp(const juce::MouseEvent& e) override {
        if (FpsMeter::clickToggles(e.getPosition(), getLocalBounds())) repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
        fetchIfStale(false);
        auto* src = dynamic_cast<CamPreviewSource*>(host_.liveOrganism(name_));
        const juce::String blocked =
            src != nullptr ? juce::String(src->camUnavailable().c_str()) : juce::String();
        if (!src || blocked.isNotEmpty() || !src->camActive() || stalled()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText(src == nullptr ? juce::String("No live instance")
                       : blocked.isNotEmpty() ? blocked
                       : stalled()
                           ? juce::String("no picture arriving - is the source on?")
                           : juce::String("Camera off - turn on Enabled below"),
                       getLocalBounds().reduced(8), juce::Justification::centred, true);
            g.setColour(Palette::border);
            g.drawRect(getLocalBounds());
            return;
        }
        auto img = getLocalBounds().toFloat();
        if (frame_.isValid()) {
            img = frame_.getBounds().toFloat().transformedBy(
                juce::RectanglePlacement(juce::RectanglePlacement::centred)
                    .getTransformToFit(frame_.getBounds().toFloat(), img));
            g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
            g.drawImage(frame_, img, juce::RectanglePlacement::stretchToFit);
        }

        if (const auto sk = src->camSkeleton(); sk.supported) {
            const float w = img.getWidth(), h = img.getHeight();
            const float ox = img.getX(), oy = img.getY();
            if (sk.points > 0) {
                const int group = sk.groupSize > 0 ? sk.groupSize : sk.points;
                g.setColour(juce::Colour(0xff21c063));
                for (int base = 0; base + group <= sk.points; base += group)
                    for (int b = 0; b < sk.boneCount; ++b) {
                        const int i = base + sk.bones[b][0], j = base + sk.bones[b][1];
                        if (i >= sk.points || j >= sk.points) continue;
                        g.drawLine(ox + sk.pt[(size_t) i][0] * w, oy + sk.pt[(size_t) i][1] * h,
                                   ox + sk.pt[(size_t) j][0] * w, oy + sk.pt[(size_t) j][1] * h,
                                   2.5f);
                    }
                g.setColour(juce::Colour(0xffe23d2e));
                for (int i = 0; i < sk.points; ++i)
                    g.fillEllipse(ox + sk.pt[(size_t) i][0] * w - 3.5f,
                                  oy + sk.pt[(size_t) i][1] * h - 3.5f, 7.0f, 7.0f);
            } else {
                g.setColour(juce::Colours::white.withAlpha(0.75f));
                g.setFont(juce::FontOptions(12.0f));
                g.drawText("show a hand to the camera",
                           getLocalBounds().reduced(6).removeFromBottom(16),
                           juce::Justification::centredLeft, false);
            }
            paintRate(g);
            FpsMeter::paintButton(g, getLocalBounds());
            g.setColour(Palette::border);
            g.drawRect(getLocalBounds());
            return;
        }

        if (auto* vn = dynamic_cast<VideoNode*>(host_.liveOrganism(name_));
            vn != nullptr && vn->numVideoOutputs() > 0) {
            FpsMeter::paintButton(g, getLocalBounds());
            g.setColour(Palette::border);
            g.drawRect(getLocalBounds());
            return;
        }

        float x = 0.5f, y = 0.5f, motion = 0.0f, bright = 0.0f;
        src->camSignals(x, y, motion, bright);
        const float cx = img.getX() + x * img.getWidth();
        const float cy = img.getY() + (1.0f - y) * img.getHeight();
        const float r = 8.0f + motion * 26.0f;
        g.setColour(Palette::accent.withAlpha(0.85f));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 2.0f);
        g.drawLine(cx - 5.0f, cy, cx + 5.0f, cy, 1.4f);
        g.drawLine(cx, cy - 5.0f, cx, cy + 5.0f, 1.4f);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("motion " + juce::String(motion, 2) + "   bright " + juce::String(bright, 2),
                   getLocalBounds().reduced(6).removeFromBottom(14),
                   juce::Justification::centredLeft, false);
        paintRate(g);
        FpsMeter::paintButton(g, getLocalBounds());
        g.setColour(Palette::border);
        g.drawRect(getLocalBounds());
    }

private:
    void poll() override { fetchIfStale(true); }

    void paintRate(juce::Graphics& g) { meter_.paint(g, getLocalBounds()); }

    bool stalled() const { return meter_.stalled(); }

    void fetchIfStale(bool repaintOnChange) {
        auto* src = dynamic_cast<CamPreviewSource*>(host_.liveOrganism(name_));
        const unsigned gen = src ? src->camGeneration() : 0;
        const bool active = src && src->camActive();
        if (active) meter_.note(gen);
        else meter_.reset();
        if (src != nullptr && src->camSourceHeld()) meter_.keepFresh();
        const bool nowStalled = active && stalled();
        if (gen != lastGen_ || active != lastActive_ || nowStalled != wasStalled_) {
            wasStalled_ = nowStalled;
            lastGen_ = gen;
            lastActive_ = active;
            if (src && active) {
                const auto f = src->camFrame();
                if (f.width > 0 && f.height > 0) {
                    frame_ = juce::Image(juce::Image::ARGB, f.width, f.height, false);
                    juce::Image::BitmapData bd(frame_, juce::Image::BitmapData::writeOnly);
                    for (int yy = 0; yy < f.height; ++yy) {
                        auto* line = (juce::PixelARGB*) bd.getLinePointer(yy);
                        const auto* p = f.rgba.data() + (size_t) yy * (size_t) f.width * 4;
                        for (int xx = 0; xx < f.width; ++xx, p += 4)
                            line[xx].setARGB(255, p[0], p[1], p[2]);
                    }
                }
            }
            if (repaintOnChange) repaint();
        }
    }

    juce::Image frame_;
    unsigned lastGen_ = ~0u;
    bool lastActive_ = false;
    FpsMeter meter_;
    bool wasStalled_ = false;
};

}
