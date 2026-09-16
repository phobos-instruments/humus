// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/TracksPane.h"

#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"
#include "gui/tracks/TimelineChips.h"

namespace hum {

void TracksPane::paintBackCrumb(juce::Graphics& g) {
    const auto back = crumbBackBox();
    g.setFont(juce::FontOptions(timelinechrome::kCrumbFont));
    g.setColour(Palette::panelLight);
    g.fillRoundedRectangle(back.toFloat(), 3.0f);
    g.setColour(Palette::textDim);
    g.drawText(juce::String::fromUTF8("\xe2\x86\x90 Back to Timeline"), back,
               juce::Justification::centred, false);
}

void TracksPane::paintNamedCrumb(juce::Graphics& g, const std::string& node, const juce::String& label,
                                 float fontH) {
    paintBackCrumb(g);
    const juce::Rectangle<int> name(crumbBackBox().getRight() + 10, 1, 300, kTopH - 2);
    g.setFont(juce::FontOptions(fontH));
    g.setColour(timelinechrome::laneAccent(host_.model(), node));
    g.fillEllipse((float) name.getX(), (float) name.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour(Palette::text);
    g.drawText(label, name.withTrimmedLeft(10), juce::Justification::centredLeft, true);
}

void TracksPane::paintCrumb(juce::Graphics& g) {
    switch (mode()) {
        case Mode::Track: {
            const auto& node = rollView_.node();
            g.setColour(timelinechrome::laneAccent(host_.model(), node).withAlpha(alpha::mid));
            g.fillRect(kStripW, tracksgeo::headerH() - 1, getWidth() - kStripW, 1);
            paintNamedCrumb(g, node, juce::String(node) + juce::String::fromUTF8("  \xe2\x96\xbe"),
                            timelinechrome::kCrumbFont);
            break;
        }
        case Mode::Box:
            paintNamedCrumb(g, song_.boxNode(),
                            juce::String(song_.boxNode()) + juce::String::fromUTF8(" \xe2\x80\xba Automation"),
                            timelinechrome::kCrumbFont);
            break;
        case Mode::Clip: {
            ClipEditor::ClipInfo ci;
            if (!clipView_.clipInfo(ci)) break;
            const auto clipName = ci.name.empty() ? juce::File(ci.audioFile).getFileNameWithoutExtension()
                                                  : juce::String(ci.name);
            paintNamedCrumb(g, clipView_.node(),
                            juce::String(clipView_.node()) + juce::String::fromUTF8(" \xe2\x80\xba ") + clipName,
                            10.0f);
            break;
        }
        case Mode::Song: break;
    }
}

bool TracksPane::crumbDown(const juce::MouseEvent& e, juce::Point<int> p) {
    if (mode_ == Mode::Track) {
        if (crumbBackBox().contains(p)) { leaveTrackMode(); return true; }
        if (crumbNameBox().contains(p)) {
            juce::PopupMenu m;
            const auto rows = song_.noteRows();
            for (int i = 0; i < (int) rows.size(); ++i)
                m.addItem(i + 1, juce::String(rows[(size_t) i]), true,
                          rows[(size_t) i] == rollView_.node());
            const auto sp = e.getScreenPosition();
            m.showMenuAsync(juce::PopupMenu::Options()
                                .withTargetScreenArea({sp.x, sp.y, 1, 1}),
                            [this, rows](int r) {
                if (r > 0 && r <= (int) rows.size()) enterTrackMode(rows[(size_t) (r - 1)]);
            });
            return true;
        }
        return false;
    }
    if (!crumbBackBox().contains(p)) return false;
    if (mode_ == Mode::Clip) { leaveClipMode(); return true; }
    if (mode() == Mode::Box) { leaveBoxMode(); return true; }
    return false;
}

juce::String TracksPane::crumbTooltip(juce::Point<int> p) {
    if (mode_ != Mode::Track || p.y >= kTopH) return {};
    if (crumbBackBox().contains(p)) return tr("tracks-pane-roll.back-to-the-timeline-esc", "Back to the timeline (Esc)");
    if (crumbNameBox().contains(p)) return "Choose a track (Alt+Up / Alt+Down)";
    return {};
}

}
