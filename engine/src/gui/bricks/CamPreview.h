// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/video/FpsMeter.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/host/BrickHost.h"
#include "hum/Organism.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/video/VideoModels.h"
#include "gui/common/Localisation.h"

namespace hum {

class CamPreview : public PolledBrick {
public:
    CamPreview(BrickHost& host, std::string organism)
        : PolledBrick(host, organism), cam_(host, organism) {
        setOpaque(true);
        meter_.watches(name_);
    }

    void reloadValues() override { repaint(); }
    void refreshAutomatedValues() override {}
    int preferredContentWidth() const override { return 312; }
    int preferredContentHeight(int) const override { return 234; }

    void mouseUp(const juce::MouseEvent& e) override {
        if (meter_.clickToggles(e.getPosition(), getLocalBounds())) repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
        fetchIfStale(false);
        auto* src = cam_.source();
        if (const auto message = cam_.message(stalled()); !message.empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText(juce::String::fromUTF8(message.c_str()), getLocalBounds().reduced(8),
                       juce::Justification::centred, true);
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
                g.setColour(ink::skeleton::bone);
                for (int base = 0; base + group <= sk.points; base += group)
                    for (int b = 0; b < sk.boneCount; ++b) {
                        const int i = base + sk.bones[b][0], j = base + sk.bones[b][1];
                        if (i >= sk.points || j >= sk.points) continue;
                        g.drawLine(ox + sk.pt[(size_t) i][0] * w, oy + sk.pt[(size_t) i][1] * h,
                                   ox + sk.pt[(size_t) j][0] * w, oy + sk.pt[(size_t) j][1] * h,
                                   2.5f);
                    }
                g.setColour(ink::skeleton::joint);
                for (int i = 0; i < sk.points; ++i)
                    g.fillEllipse(ox + sk.pt[(size_t) i][0] * w - 3.5f,
                                  oy + sk.pt[(size_t) i][1] * h - 3.5f, 7.0f, 7.0f);
            } else {
                g.setColour(juce::Colours::white.withAlpha(alpha::strong));
                g.setFont(juce::FontOptions(12.0f));
                g.drawText(tr("cam-preview.show-a-hand-to-the", "show a hand to the camera"),
                           getLocalBounds().reduced(6).removeFromBottom(16),
                           juce::Justification::centredLeft, false);
            }
            paintRate(g);
            meter_.paintButton(g, getLocalBounds());
            g.setColour(Palette::border);
            g.drawRect(getLocalBounds());
            return;
        }

        if (cam_.producesVideo()) {
            meter_.paintButton(g, getLocalBounds());
            g.setColour(Palette::border);
            g.drawRect(getLocalBounds());
            return;
        }

        const auto sig = cam_.signals();
        const float cx = img.getX() + sig.x * img.getWidth();
        const float cy = img.getY() + (1.0f - sig.y) * img.getHeight();
        const float r = sig.radius();
        g.setColour(Palette::accent.withAlpha(alpha::heavy));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 2.0f);
        g.drawLine(cx - 5.0f, cy, cx + 5.0f, cy, 1.4f);
        g.drawLine(cx, cy - 5.0f, cx, cy + 5.0f, 1.4f);

        g.setColour(juce::Colours::white.withAlpha(alpha::heavy));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String::fromUTF8(cam_.signalText(sig).c_str()),
                   getLocalBounds().reduced(6).removeFromBottom(14),
                   juce::Justification::centredLeft, false);
        paintRate(g);
        meter_.paintButton(g, getLocalBounds());
        g.setColour(Palette::border);
        g.drawRect(getLocalBounds());
    }

private:
    void poll() override { fetchIfStale(true); }

    void paintRate(juce::Graphics& g) { meter_.paint(g, getLocalBounds()); }

    bool stalled() const { return meter_.stalled(); }

    void fetchIfStale(bool repaintOnChange) {
        auto* src = cam_.source();
        const unsigned gen = src ? src->camGeneration() : 0;
        const bool active = cam_.active();
        if (active) meter_.note(gen);
        else meter_.reset();
        if (src != nullptr && src->camSourceHeld()) meter_.keepFresh();
        if (cam_.frameChanged(stalled())) {
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

    video::CamPreviewModel cam_;
    juce::Image frame_;
    FpsMeter meter_;
};

}
