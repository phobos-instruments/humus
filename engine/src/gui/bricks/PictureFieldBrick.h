// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "hum/caps/Graph.h"
#include "hum/caps/Video.h"
#include "hum/PixelField.h"
#include "gui/common/Localisation.h"

namespace hum {

class PictureFieldBrick : public PolledBrick, public juce::FileDragAndDropTarget {
public:
    PictureFieldBrick(BrickHost& host, std::string organism, std::string param, const Bindings& bound)
        : PolledBrick(host, std::move(organism), 2), param_(std::move(param)),
          blurParam_(bound(bind::kBlur)), gateParam_(bound(bind::kGate)), tiltParam_(bound(bind::kTilt)),
          levelParam_(bound(bind::kLevel)), lowestParam_(bound(bind::kLowest)), highestParam_(bound(bind::kHighest)),
          xParam_(bound(bind::kX)), yParam_(bound(bind::kY)), widthParam_(bound(bind::kWidth)), heightParam_(bound(bind::kHeight)) {
        reloadValues();
    }

    int preferredContentWidth() const override { return 584; }
    int windowRepaintsForTest() const { return windowRepaints_; }
    int preferredContentHeight(int) const override { return 150; }

    void reloadValues() override {
        const auto path = host_.liveParamText(name_, param_);
        if (path == path_ && shownGen_ != kNoGeneration) return;
        path_ = path;
        if (const auto* src = live<PixelFieldSource>()) {
            shownGen_ = kNoGeneration;
            (void) src;
            poll();
            return;
        }
        field_ = {};
        loadPixelField(path_, field_, 512, 256);
        rebuildImage();
    }

    struct Look {
        int blur = 0;
        float gate = 0.0f, tilt = 0.0f, level = 1.0f;
        double lowest = 55.0, highest = 8000.0, y = 0.0, height = 1.0;
        bool operator==(const Look& o) const {
            return blur == o.blur && gate == o.gate && tilt == o.tilt && level == o.level
                && lowest == o.lowest && highest == o.highest && y == o.y && height == o.height;
        }
        bool operator!=(const Look& o) const { return !(*this == o); }
    };

    Look lookNow() const {
        auto p = [&](const std::string& n, double) { return host_.liveParamValue(name_, n); };
        Look k;
        k.blur = pixelfield::blurRadiusFor(p(blurParam_, 0.2));
        k.gate = (float) juce::jlimit(0.0, 1.0, p(gateParam_, 0.1));
        k.tilt = (float) juce::jlimit(-1.0, 1.0, p(tiltParam_, 0.0));
        k.level = (float) juce::jlimit(0.0, 2.0, p(levelParam_, 1.0));
        k.lowest = juce::jlimit(20.0, 2000.0, p(lowestParam_, 55.0));
        k.highest = std::max(k.lowest * 1.25, juce::jlimit(200.0, 16000.0, p(highestParam_, 8000.0)));
        k.y = juce::jlimit(0.0, 1.0, p(yParam_, 0.0));
        k.height = juce::jlimit(0.02, 1.0, p(heightParam_, 1.0));
        return k;
    }

    float tiltGain(const Look& k, int row, int height) const {
        if (k.tilt == 0.0f || height <= 1) return 1.0f;
        const double rowTop = (1.0 - k.y - k.height) * (double) (height - 1);
        const double rowSpan = k.height * (double) (height - 1);
        if (rowSpan <= 0.0) return 1.0f;
        const double up = juce::jlimit(0.0, 1.0, 1.0 - ((double) row - rowTop) / rowSpan);
        const double hz = k.lowest * std::pow(k.highest / k.lowest, up);
        const double top = std::pow(k.highest / k.lowest, k.tilt * 0.5);
        const double g = std::pow(hz / k.lowest, k.tilt * 0.5);
        return (float) (k.tilt > 0.0f ? g / top : g);
    }

    void rebuildImage() {
        image_ = {};
        look_ = lookNow();
        if (!field_.empty()) {
            PixelField shown;
            pixelfield::boxBlur(field_, look_.blur, shown);
            image_ = juce::Image(juce::Image::RGB, shown.width, shown.height, false);
            juce::Image::BitmapData bd(image_, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < shown.height; ++y) {
                const float rowGain = tiltGain(look_, y, shown.height) * look_.level;
                for (int x = 0; x < shown.width; ++x) {
                    const float lit = pixelfield::gated(shown.at(x, y), look_.gate) * rowGain;
                    const auto v = (juce::uint8) juce::jlimit(0, 255, (int) std::lround(lit * 255.0f));
                    bd.setPixelColour(x, y, juce::Colour(v, v, v));
                }
            }
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(Palette::background.darker(0.15f));
        g.fillRoundedRectangle(r, 5.0f);
        const auto area = getLocalBounds().reduced(3);

        if (image_.isValid()) {
            g.setOpacity(1.0f);
            g.drawImage(image_, area.toFloat(), juce::RectanglePlacement::stretchToFit);
            g.setColour(Palette::accent.withAlpha(alpha::veil));
            g.fillRect(area);
        } else {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(dropHot_ ? juce::String(tr("picture-field.let-go", "Let go"))
                                : juce::String(tr("picture-field.drop-a-picture-here-or", "Drop a picture here, or any other file")),
                       getLocalBounds(), juce::Justification::centred);
            g.setColour(dropHot_ ? Palette::accent : Palette::border);
            g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, dropHot_ ? 2.0f : 1.0f);
            return;
        }

        const auto win = windowRect(area);
        g.setColour(Palette::background.withAlpha(alpha::mid));
        for (auto& outside : { juce::Rectangle<int>(area.getX(), area.getY(),
                                                    win.getX() - area.getX(), area.getHeight()),
                               juce::Rectangle<int>(win.getRight(), area.getY(),
                                                    area.getRight() - win.getRight(),
                                                    area.getHeight()),
                               juce::Rectangle<int>(win.getX(), area.getY(), win.getWidth(),
                                                    win.getY() - area.getY()),
                               juce::Rectangle<int>(win.getX(), win.getBottom(), win.getWidth(),
                                                    area.getBottom() - win.getBottom()) })
            if (!outside.isEmpty()) g.fillRect(outside);
        g.setColour(Palette::accentDim.withAlpha(alpha::strong));
        g.drawRect(win, 1);

        const int hx = area.getX() + (int) (scan_ * (float) area.getWidth());
        g.setColour(Palette::accent.withAlpha(0.35f + 0.6f * level_));
        g.drawVerticalLine(hx, (float) win.getY(), (float) win.getBottom());
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    }

    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void fileDragEnter(const juce::StringArray&, int, int) override { dropHot_ = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { dropHot_ = false; repaint(); }
    void filesDropped(const juce::StringArray& files, int, int) override {
        dropHot_ = false;
        if (!files.isEmpty()) setPicture(files[0]);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!image_.isValid() || e.mods.isPopupMenu()) { browse(); return; }
        scrub(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override { if (image_.isValid()) scrub(e); }
    void mouseDoubleClick(const juce::MouseEvent&) override { browse(); }

protected:
    void poll() override {
        if (const auto* src = live<PixelFieldSource>()) {
            const unsigned gen = src->pixelFieldGeneration();
            if (gen != shownGen_) {
                shownGen_ = gen;
                if (!src->copyPixelField(field_)) field_ = {};
                rebuildImage();
            }
        }
        if (!field_.empty() && lookNow() != look_) rebuildImage();
        const auto win = windowRect(getLocalBounds().reduced(3));
        if (win != window_) {
            window_ = win;
            ++windowRepaints_;
            repaint();
        }
        float s = scan_, l = level_;
        if (auto* cs = live<ControlSource>()) {
            ControlSource::ControlVal vals[4];
            const int n = cs->controlValues(vals, 4);
            for (int i = 0; i < n; ++i) {
                if (juce::String(vals[i].name) == "scan") s = vals[i].value;
                if (juce::String(vals[i].name) == "level") l = vals[i].value;
            }
        }
        const int w = juce::jmax(1, getWidth());
        if ((int) (s * w) == (int) (scan_ * w) && std::abs(l - level_) < 0.02f) return;
        scan_ = s;
        level_ = l;
        repaint();
    }

private:
    juce::Rectangle<int> windowRect(juce::Rectangle<int> area) {
        auto p = [&](const std::string& n) { return host_.liveParamValue(name_, n); };
        const int x = area.getX() + (int) (p(xParam_) * area.getWidth());
        const int w = std::max(2, (int) (p(widthParam_) * area.getWidth()));
        const double y = p(yParam_), h = p(heightParam_);
        const int top = area.getY() + (int) ((1.0 - y - h) * area.getHeight());
        return { x, top, std::min(w, area.getRight() - x),
                 std::max(2, (int) (h * area.getHeight())) };
    }

    void scrub(const juce::MouseEvent& e) {
        const auto area = getLocalBounds().reduced(3);
        if (area.getWidth() <= 0 || area.getHeight() <= 0) return;
        const double fx = juce::jlimit(0.0, 1.0,
                                       (double) (e.x - area.getX()) / area.getWidth());
        const double fy = juce::jlimit(0.0, 1.0,
                                       1.0 - (double) (e.y - area.getY()) / area.getHeight());
        const double h = host_.liveParamValue(name_, heightParam_);
        host_.setParam(name_, xParam_, fx);
        host_.setParam(name_, yParam_, juce::jlimit(0.0, 1.0 - h, fy - h * 0.5));
        host_.notePanelEdit(name_);
        repaint();
    }

    void browse() {
        chooser_ = std::make_unique<juce::FileChooser>(tr("picture-field.pick-a-picture-or-any", "Pick a picture - or any file at all"));
        juce::Component::SafePointer<PictureFieldBrick> safe(this);
        chooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [safe](const juce::FileChooser& fc) {
            const auto f = fc.getResult();
            if (safe != nullptr && f.existsAsFile()) safe->setPicture(f.getFullPathName());
        });
    }

    void setPicture(const juce::String& file) {
        host_.setParamText(name_, param_, file.toStdString());
        reloadValues();
    }

    static constexpr unsigned kNoGeneration = ~0u;
    std::string param_, path_;
    std::string blurParam_, gateParam_, tiltParam_, levelParam_, lowestParam_, highestParam_, xParam_, yParam_,
        widthParam_, heightParam_;
    PixelField field_;
    juce::Image image_;
    unsigned shownGen_ = kNoGeneration;
    Look look_;
    juce::Rectangle<int> window_;
    int windowRepaints_ = 0;
    std::unique_ptr<juce::FileChooser> chooser_;
    float scan_ = 0.0f, level_ = 0.0f;
    bool dropHot_ = false;
};

}
