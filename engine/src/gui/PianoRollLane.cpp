#include <algorithm>

#include "gui/PianoRollEditor.h"
#include "gui/LookAndFeel.h"

namespace hum {

namespace {
struct NamedCC { int cc; const char* name; };
constexpr NamedCC kCommonCCs[] = {
    {1, "Mod Wheel"}, {2, "Breath"}, {7, "Volume"}, {10, "Pan"},
    {11, "Expression"}, {64, "Sustain"}, {71, "Resonance"}, {74, "Cutoff"},
};
juce::String ccLabel(int cc) {
    for (const auto& c : kCommonCCs)
        if (c.cc == cc) return "CC" + juce::String(cc) + " " + c.name;
    return "CC" + juce::String(cc);
}
}

void PianoRollEditor::applyVelocityLane(juce::Point<int> p) {
    const int vel = juce::jlimit(1, 127, juce::roundToInt(
        127.0 * ((gridBottom() + kVelH - 3) - p.y) / (double) (kVelH - 6)));
    bool changed = false;
    for (size_t i = 0; i < gestureNotes_.size(); ++i) {
        if (!selection_.empty()) {
            if (!selection_.count((int) i)) continue;
        } else if (std::abs((float) p.x - tickToX(gestureNotes_[i].tick)) > 4.0f) {
            continue;
        }
        gestureNotes_[i].velocity = vel;
        changed = true;
    }
    if (changed) repaint();
}

void PianoRollEditor::applyCCLane(juce::Point<int> p) {
    const int value = juce::jlimit(0, 127, juce::roundToInt(
        127.0 * ((gridBottom() + kVelH - 3) - p.y) / (double) (kVelH - 6)));
    const int tick = snapTick(juce::jlimit(0, durationTicks() - 1, xToTick((float) p.x)));
    bool placed = false;
    for (auto& c : gestureCCs_)
        if (c.controller == laneCC_ && c.tick == tick) { c.value = value; placed = true; }
    if (!placed) gestureCCs_.push_back({tick, laneCC_, value});
    repaint();
}

void PianoRollEditor::showLaneMenu() {
    juce::PopupMenu m;
    m.addItem(1, "Velocity", true, laneCC_ < 0);
    m.addSeparator();
    std::vector<int> offered;
    for (const auto& c : kCommonCCs) offered.push_back(c.cc);
    for (const auto& e : host_.clips().ccs(name_, clip_))
        if (std::find(offered.begin(), offered.end(), e.controller) == offered.end())
            offered.push_back(e.controller);
    for (int cc : offered) m.addItem(100 + cc, ccLabel(cc), true, laneCC_ == cc);
    if (laneCC_ >= 0) {
        m.addSeparator();
        m.addItem(2, "Clear " + ccLabel(laneCC_) + " events");
    }
    m.showMenuAsync({}, [this](int r) {
        if (r == 0) return;
        if (r == 1) { laneCC_ = -1; repaint(); return; }
        if (r == 2 && laneCC_ >= 0) {
            host_.pushUndo();
            auto ccs = host_.clips().ccs(name_, clip_);
            ccs.erase(std::remove_if(ccs.begin(), ccs.end(),
                                     [this](const CCEvent& c) { return c.controller == laneCC_; }),
                      ccs.end());
            host_.clips().setCCs(name_, clip_, ccs);
            repaint();
            return;
        }
        if (r >= 100 && r <= 227) { laneCC_ = r - 100; repaint(); }
    });
}

void PianoRollEditor::paintCCLane(juce::Graphics& g, int top, int h) {
    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("CC" + juce::String(laneCC_), 4, top + 3, kKeyW - 8, 10,
               juce::Justification::centredLeft, false);

    const auto ccs = gesture_ == Gesture::CCLane ? gestureCCs_ : host_.clips().ccs(name_, clip_);
    std::vector<const CCEvent*> lane;
    for (const auto& c : ccs) if (c.controller == laneCC_) lane.push_back(&c);
    std::sort(lane.begin(), lane.end(),
              [](const CCEvent* a, const CCEvent* b) { return a->tick < b->tick; });

    const auto yFor = [&](int v) { return (float) (top + h - 3) - (float) (h - 6) * v / 127.0f; };
    g.setColour(Palette::accent.withAlpha(0.7f));
    for (size_t i = 0; i < lane.size(); ++i) {
        const float x = tickToX(lane[i]->tick);
        const float xn = i + 1 < lane.size() ? tickToX(lane[i + 1]->tick)
                                             : tickToX(durationTicks());
        const float y = yFor(lane[i]->value);
        if (xn > (float) gridLeft() && x < (float) getWidth())
            g.drawHorizontalLine((int) y, juce::jmax(x, (float) gridLeft()), xn);
        if (x >= (float) gridLeft() && x <= (float) getWidth())
            g.fillEllipse(x - 2.5f, y - 2.5f, 5.0f, 5.0f);
    }
}

}
