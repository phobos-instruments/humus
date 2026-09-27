// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/editor/juce/JuceGeometry.h"
#include "gui/editor/BrickBindings.h"
#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/editor/files/PictureFieldModel.h"
#include "hum/PixelFieldImage.h"
#include "gui/editor/juce/JuceFilePicker.h"
#include "gui/editor/juce/JuceMediaLibrary.h"
#include "gui/common/Localisation.h"

namespace hum {

class PictureFieldBrick : public PolledBrick, public juce::FileDragAndDropTarget {
public:
    PictureFieldBrick(BrickHost& host, std::string organism, std::string param, const Bindings& bound)
        : PolledBrick(host, organism, 2),
          media_(host),
          field_(host, media_, organism,
                 {std::move(param), bound(bind::kBlur), bound(bind::kGate), bound(bind::kTilt), bound(bind::kLevel),
                  bound(bind::kLowest), bound(bind::kHighest), bound(bind::kX), bound(bind::kY), bound(bind::kWidth),
                  bound(bind::kHeight)}) {
        reloadValues();
    }

    int preferredContentWidth() const override { return 584; }
    int windowRepaintsForTest() const { return windowRepaints_; }
    int preferredContentHeight(int) const override { return 150; }

    void reloadValues() override {
        const auto result = field_.reload();
        if (result == files::PictureFieldModel::Reload::Live) poll();
        if (result == files::PictureFieldModel::Reload::Loaded) rebuildImage();
    }

    void rebuildImage() {
        image_ = {};
        const auto shade = field_.shade();
        if (!shade.grey.empty()) {
            image_ = juce::Image(juce::Image::RGB, shade.width, shade.height, false);
            juce::Image::BitmapData bd(image_, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < shade.height; ++y)
                for (int x = 0; x < shade.width; ++x) {
                    const auto v = shade.grey[(size_t) y * (size_t) shade.width + (size_t) x];
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

        const int hx = area.getX() + (int) (field_.scan() * (float) area.getWidth());
        g.setColour(Palette::accent.withAlpha(0.35f + 0.6f * field_.level()));
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
        if (field_.pullLive()) rebuildImage();
        if (field_.lookMoved()) rebuildImage();
        const auto win = windowRect(getLocalBounds().reduced(3));
        if (win != window_) {
            window_ = win;
            ++windowRepaints_;
            repaint();
        }
        if (field_.followScan(getWidth())) repaint();
    }

private:
    juce::Rectangle<int> windowRect(juce::Rectangle<int> area) {
        return toJuce(field_.window(rectOf(area)));
    }

    void scrub(const juce::MouseEvent& e) {
        const auto area = getLocalBounds().reduced(3);
        if (area.getWidth() <= 0 || area.getHeight() <= 0) return;
        field_.scrub(e.x, e.y, rectOf(area));
        host_.notePanelEdit(name_);
        repaint();
    }

    void browse() {
        files::FilePick request;
        request.title = tr("picture-field.pick-a-picture-or-any", "Pick a picture - or any file at all").toStdString();
        juce::Component::SafePointer<PictureFieldBrick> safe(this);
        picker_.pick(request, [safe](const std::vector<std::string>& paths) {
            if (safe == nullptr || paths.empty()) return;
            const juce::File f(juce::String::fromUTF8(paths.front().c_str()));
            if (f.existsAsFile()) safe->setPicture(f.getFullPathName());
        });
    }

    void setPicture(const juce::String& file) {
        field_.setPicture(file.toStdString());
        reloadValues();
    }

    JuceMediaLibrary media_;
    files::PictureFieldModel field_;
    juce::Image image_;
    juce::Rectangle<int> window_;
    int windowRepaints_ = 0;
    JuceFilePicker picker_;
    bool dropHot_ = false;
};

}
