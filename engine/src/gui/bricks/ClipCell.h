// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/timeline/ClipDrag.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/style/Colours.h"
#include "gui/editor/BrickBindings.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostClips.h"
#include "gui/video/FrameImage.h"
#include "gui/style/IconGlyph.h"
#include "gui/style/LookAndFeel.h"
#include "gui/properties/PadBar.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/bricks/PolledBrick.h"
#include "gui/video/VideoDeckPool.h"
#include "gui/common/Localisation.h"

namespace hum {

class CellButton : public juce::Button {
public:
    CellButton(IconGlyph glyph, const juce::String& text)
        : juce::Button(text.isEmpty() ? juce::String(kIconGlyphNames[(size_t) glyph]) : text),
          glyph_(glyph), text_(text) {}

    void setGlyph(IconGlyph g) {
        if (g == glyph_) return;
        glyph_ = g;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel.brighter(down ? 0.20f : over ? 0.12f : 0.05f));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(getToggleState() ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
        const auto tint = !isEnabled() ? Palette::textDim.withAlpha(alpha::dim)
                          : getToggleState() ? Palette::accent
                                             : Palette::text;
        if (text_.isNotEmpty()) {
            g.setColour(tint);
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText(text_, getLocalBounds(), juce::Justification::centred, false);
            return;
        }
        drawIconGlyph(g, glyph_, r.reduced(r.getHeight() * 0.28f), tint, isEnabled(),
                      getToggleState());
    }

private:
    IconGlyph glyph_;
    juce::String text_;
};

class ClipCell : public juce::Component,
                 public juce::FileDragAndDropTarget,
                 public juce::DragAndDropTarget {
public:
    static constexpr int kWidth = 138, kThumbH = 78, kHeight = 118;

    std::function<void()> onLaunch, onPlayPause, onSetIn, onSetOut, onLoop, onOpen;
    std::function<void(double)> onScrub;
    std::function<void(const juce::File&)> onDrop;
    std::function<void(juce::Point<int>)> onMenu;
    std::function<void(int, bool)> onDropPad;
    std::function<void(double)> onDragIn, onDragOut;
    std::function<void(const std::string&, int)> onDropClip;
    std::string node;

    explicit ClipCell(int row);

    void setState(const juce::String& fileName, bool active, bool outgoing, bool paused, double pos, double len, double in, double out, bool loop);

    void setThumbnail(juce::Image img);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseMove(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray&, int, int) override;
    void fileDragExit(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    bool isInterestedInDragSource(const SourceDetails& d) override;
    void itemDragEnter(const SourceDetails&) override;
    void itemDragExit(const SourceDetails&) override;
    void itemDropped(const SourceDetails& d) override;

    static bool accepts(const juce::String& path);

private:
    static juce::String clock(double seconds);

    padbar::Span span() const { return padbar::clipSpan(inSec_, outSec_, len_); }

    juce::String rangeText() const;

    int row_;
    juce::String fileName_;
    juce::Image thumb_;
    bool active_ = false, outgoing_ = false, dragOver_ = false, dragged_ = false;
    bool badgeDrag_ = false, lifted_ = false;
    padbar::Grip grip_ = padbar::Grip::None;
    double pos_ = 0.0, len_ = 0.0, inSec_ = 0.0, outSec_ = 0.0;
    CellButton open_{IconGlyph::Open, {}}, play_{IconGlyph::Play, {}}, in_{IconGlyph::Play, "In"},
        out_{IconGlyph::Play, "Out"}, loop_{IconGlyph::Loop, {}};
};

}
