#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/PolledBrick.h"
#include "hum/Capabilities.h"
#include "hum/PixelField.h"
#include "gui/Localisation.h"

namespace hum {

class PictureFieldBrick : public PolledBrick, public juce::FileDragAndDropTarget {
public:
    PictureFieldBrick(EngineHost& host, std::string organism, std::string param)
        : PolledBrick(host, std::move(organism), 2), param_(std::move(param)) {
        reloadValues();
    }

    int preferredContentWidth() const override { return 584; }
    int preferredContentHeight(int) const override { return 150; }

    void reloadValues() override {
        const auto path = host_.liveParamText(name_, param_);
        if (path == path_) return;
        path_ = path;
        field_ = {};
        image_ = {};
        loadPixelField(path_, field_, 512, 256);
        if (!field_.empty()) {
            image_ = juce::Image(juce::Image::RGB, field_.width, field_.height, false);
            juce::Image::BitmapData bd(image_, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < field_.height; ++y)
                for (int x = 0; x < field_.width; ++x) {
                    const auto v = (juce::uint8) juce::jlimit(0, 255,
                                       (int) std::lround(field_.at(x, y) * 255.0f));
                    bd.setPixelColour(x, y, juce::Colour(v, v, v));
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
            g.setColour(Palette::accent.withAlpha(0.18f));
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
        g.setColour(Palette::background.withAlpha(0.62f));
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
        g.setColour(Palette::accentDim.withAlpha(0.7f));
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
        auto p = [&](const char* n) { return host_.liveParamValue(name_, n); };
        const int x = area.getX() + (int) (p("X") * area.getWidth());
        const int w = std::max(2, (int) (p("Span") * area.getWidth()));
        const double y = p("Y"), h = p("Height");
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
        const double h = host_.liveParamValue(name_, "Height");
        host_.setParam(name_, "X", fx);
        host_.setParam(name_, "Y", juce::jlimit(0.0, 1.0 - h, fy - h * 0.5));
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

    std::string param_, path_;
    PixelField field_;
    juce::Image image_;
    std::unique_ptr<juce::FileChooser> chooser_;
    float scan_ = 0.0f, level_ = 0.0f;
    bool dropHot_ = false;
};

}
