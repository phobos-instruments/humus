// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <algorithm>

#include "gui/style/Colours.h"
#include "gui/host/EngineHostClips.h"
#include "gui/pianoroll/NoteColourMenu.h"
#include "gui/pianoroll/PianoRollEditor.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
struct NamedCC { int cc; const char* key; const char* name; };
constexpr NamedCC kCommonCCs[] = {
    {1, "piano-roll-lane.cc-mod-wheel", "Mod Wheel"},
    {2, "piano-roll-lane.cc-breath", "Breath"},
    {7, "piano-roll-lane.cc-volume", "Volume"},
    {10, "piano-roll-lane.cc-pan", "Pan"},
    {11, "piano-roll-lane.cc-expression", "Expression"},
    {64, "piano-roll-lane.cc-sustain", "Sustain"},
    {71, "piano-roll-lane.cc-resonance", "Resonance"},
    {74, "piano-roll-lane.cc-cutoff", "Cutoff"},
};
juce::String ccLabel(int cc) {
    if (cc == kBendController) return tr("piano-roll-lane.bend", "Pitch bend");
    for (const auto& c : kCommonCCs)
        if (c.cc == cc) return "CC" + juce::String(cc) + " " + tr(c.key, c.name);
    return "CC" + juce::String(cc);
}
}

void PianoRollEditor::showLaneMenu() {
    enum { kVelocity = 1, kClear, kBend, kDeletePoints };
    juce::PopupMenu m;
    if (model_.laneCC >= 0 && !model_.ccSelection.empty()) {
        m.addSubMenu(tr("tracks-pane-menu.color", "Color"), notecolour::menu(selectedCCsColour()));
        m.addItem(kDeletePoints, tr("piano-roll-lane.delete-points", "Delete points"));
        m.addSeparator();
    }
    m.addItem(kVelocity, tr("piano-roll-lane.velocity", "Velocity"), true, model_.laneCC < 0);
    m.addItem(kBend, tr("piano-roll-lane.bend", "Pitch bend"), true, model_.laneCC == kBendController);
    m.addSeparator();
    std::vector<int> offered;
    for (const auto& c : kCommonCCs) offered.push_back(c.cc);
    for (const auto& e : model_.ccs())
        if (e.controller != kBendController
            && std::find(offered.begin(), offered.end(), e.controller) == offered.end())
            offered.push_back(e.controller);
    for (int cc : offered) m.addItem(100 + cc, ccLabel(cc), true, model_.laneCC == cc);
    if (model_.laneCC >= 0) {
        m.addSeparator();
        m.addItem(kClear, tr("piano-roll-lane.clear", "Clear ") + ccLabel(model_.laneCC) + tr("piano-roll-lane.events", " events"));
    }
    m.showMenuAsync({}, [this](int r) {
        if (r == 0) return;
        if (const auto c = notecolour::choiceFor(r); c.pick == notecolour::Pick::Colour) {
            colourSelectedCCs(c.colour);
            return;
        } else if (c.pick == notecolour::Pick::Custom) {
            openColourPicker(true);
            return;
        }
        if (r == kDeletePoints) { deleteSelectedCCs(); return; }
        model_.ccSelection.clear();
        if (r == kVelocity) { model_.laneCC = -1; repaint(); return; }
        if (r == kBend) { model_.laneCC = kBendController; repaint(); return; }
        if (r == kClear && model_.laneCC >= 0) {
            model_.clearLane();
            repaint();
            return;
        }
        if (r >= 100 && r <= 227) { model_.laneCC = r - 100; repaint(); }
    });
}

void PianoRollEditor::paintCCLane(juce::Graphics& g, int top, int h) {
    const auto geo = geometry();
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText(model_.laneCC == kBendController ? tr("piano-roll-lane.bend-short", "bend")
                                          : tr("piano-roll-lane.cc", "CC") + juce::String(model_.laneCC),
               4, top + 3, kKeyW - 8, 10, juce::Justification::centredLeft, false);

    const bool drawing = model_.gesture() == Gesture::CCLane;
    const auto ccs = drawing ? model_.gestureCCs() : model_.ccs();
    std::vector<int> lane;
    for (int i = 0; i < (int) ccs.size(); ++i)
        if (ccs[(size_t) i].controller == model_.laneCC) lane.push_back(i);
    std::stable_sort(lane.begin(), lane.end(), [&ccs](int a, int b) {
        return ccs[(size_t) a].tick < ccs[(size_t) b].tick;
    });

    if (model_.laneCC == kBendController) {
        g.setColour(Palette::border);
        g.drawHorizontalLine((int) geo.ccY(kBendCentre, model_.laneCC), (float) gridLeft(), (float) getWidth());
    }
    for (size_t k = 0; k < lane.size(); ++k) {
        const auto& c = ccs[(size_t) lane[k]];
        const float x = geo.tickToX(c.tick);
        const float xn = k + 1 < lane.size() ? geo.tickToX(ccs[(size_t) lane[k + 1]].tick)
                                             : geo.tickToX(durationTicks());
        const float y = geo.ccY(c.value, model_.laneCC);
        const auto col = notecolour::fill(c.colour, Palette::accent);
        g.setColour(col.withAlpha(alpha::strong));
        if (xn > (float) gridLeft() && x < (float) getWidth())
            g.drawHorizontalLine((int) y, juce::jmax(x, (float) gridLeft()), xn);
        if (x < (float) gridLeft() || x > (float) getWidth()) continue;
        const bool sel = !drawing && model_.ccSelection.count(lane[k]) != 0;
        const float r = sel ? 3.5f : 2.5f;
        g.setColour(col);
        g.fillEllipse(x - r, y - r, 2.0f * r, 2.0f * r);
        if (sel) {
            g.setColour(Palette::text);
            g.drawEllipse(x - r - 1.0f, y - r - 1.0f, 2.0f * r + 2.0f, 2.0f * r + 2.0f, 1.2f);
        }
    }
    if (const auto marquee = marqueeRect(); model_.gesture() == Gesture::CCMarquee && !marquee.isEmpty()) {
        g.setColour(Palette::accent.withAlpha(alpha::mist));
        g.fillRect(marquee);
        g.setColour(Palette::accent.withAlpha(alpha::strong));
        g.drawRect(marquee, 1);
    }
}

}
