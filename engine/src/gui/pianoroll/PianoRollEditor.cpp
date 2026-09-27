// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/pianoroll/PianoRollEditor.h"
#include "gui/host/EngineHostPattern.h"
#include "gui/host/EngineHostClips.h"
#include "gui/host/EngineHostMidiControl.h"
#include "gui/host/LiveMidi.h"
#include "gui/style/Colours.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

#include "hum/dsp/DspMath.h"

namespace hum {

PianoRollEditor::PianoRollEditor(BrickHost& host, std::string organism, Params params)
    : host_(host), name_(std::move(organism)), params_(std::move(params)), keyboard_(host, name_) {
    host_.patterns().ensureNote(name_);
    setOpaque(true);
    buildToolbar();
    reloadValues();
    setWantsKeyboardFocus(true);
    setTool(Tool::Pointer);
    startTimerHz(30);
}

PianoRollEditor::~PianoRollEditor() { releaseKey(); }

void PianoRollEditor::buildToolbar() {
    for (auto* l : {&barsLabel_, &snapLabel_}) {
        l->setColour(juce::Label::textColourId, Palette::textDim);
        l->setFont(juce::Font(juce::FontOptions(11.0f)));
        addAndMakeVisible(*l);
    }
    bars_.onChange = [this] {
        const int bars = bars_.getSelectedId();
        if (bars <= 0) return;
        host_.pushUndo();
        host_.setParam(name_, params_.bars, bars);
        host_.clips().setNotes(name_, clip_, model_.notes(), bars * 4 * Pattern::kTicksPerBeat);
        repaint();
    };
    addAndMakeVisible(bars_);

    snap_.addItem(tr("tracks-pane.snap-free", "Free"), kSnapFreeId);
    int id = 1;
    for (const char* s : {"1/4", "1/8", "1/16", "1/32"}) snap_.addItem(s, id++);
    snap_.setSelectedId(3, juce::dontSendNotification);
    snap_.onChange = [this] {
        host_.midi().setRecordGrid(name_, snapTicks());
        repaint();
    };
    addAndMakeVisible(snap_);

    buildRecordButtons();

    struct { ToolButton* b; Tool t; const char* tip; } tools[] = {
        {&toolP_, Tool::Pointer, "Pointer: select, marquee, move (1)"},
        {&toolD_, Tool::Draw, "Draw: drag to create notes (2)"},
        {&toolS_, Tool::Scissors, "Scissors: click a note to split it (3)"},
        {&toolE_, Tool::Eraser, "Eraser: sweep to delete notes (4)"},
    };
    for (auto& t : tools) {
        t.b->setClickingTogglesState(false);
        t.b->setTooltip(t.tip);
        const Tool tool = t.t;
        t.b->onClick = [this, tool] { setTool(tool); };
        addAndMakeVisible(*t.b);
    }
}

void PianoRollEditor::setClip(int clip) {
    clip_ = juce::jmax(0, clip);
    model_.ccSelection.clear();
    reloadValues();
    fitPitch();
}

void PianoRollEditor::fitPitch() {
    model_.fitPitch(geometry().rows());
    repaint();
}

void PianoRollEditor::reloadValues() {
    const int count = (int) host_.clips().list(name_).size();
    if (count > 0 && clip_ >= count) clip_ = count - 1;
    showBars(juce::jlimit(1, 64,
        (durationTicks() + 2 * Pattern::kTicksPerBeat) / (4 * Pattern::kTicksPerBeat)));
    repaint();
}

void PianoRollEditor::showBars(int bars) {
    std::vector<int> lengths{1, 2, 4, 8, 16, 32, 64};
    if (std::find(lengths.begin(), lengths.end(), bars) == lengths.end()) {
        lengths.push_back(bars);
        std::sort(lengths.begin(), lengths.end());
    }
    bool listed = bars_.getNumItems() == (int) lengths.size();
    for (int i = 0; listed && i < (int) lengths.size(); ++i)
        listed = bars_.getItemId(i) == lengths[(size_t) i];
    if (!listed) {
        bars_.clear(juce::dontSendNotification);
        for (const int b : lengths) bars_.addItem(juce::String(b), b);
    }
    bars_.setSelectedId(bars, juce::dontSendNotification);
}

int PianoRollEditor::preferredContentHeight(int) const {
    return kToolbarH + kRulerH + kVisibleRows * kRowH + kVelH + kKbH;
}

void PianoRollEditor::soundKey(int pitch) {
    pitch = juce::jlimit(0, kMidiMax, pitch);
    if (pitch == keyNote_) return;
    releaseKey();
    keyNote_ = pitch;
    host_.injectLiveMidiToNode(name_, noteOnEvent(1, pitch, 100));
    repaintKeys();
}

void PianoRollEditor::releaseKey() {
    if (keyNote_ < 0) return;
    host_.injectLiveMidiToNode(name_, noteOffEvent(1, keyNote_));
    keyNote_ = -1;
    repaintKeys();
}

void PianoRollEditor::resized() {
    auto r = getLocalBounds().removeFromTop(kToolbarH).reduced(4, 4);
    loop_.setBounds(r.removeFromLeft(58));
    r.removeFromLeft(8);
    barsLabel_.setBounds(r.removeFromLeft(30));
    bars_.setBounds(r.removeFromLeft(58));
    r.removeFromLeft(8);
    snapLabel_.setBounds(r.removeFromLeft(32));
    snap_.setBounds(r.removeFromLeft(64));
    r.removeFromLeft(10);
    for (auto* b : {&toolP_, &toolD_, &toolS_, &toolE_})
        b->setBounds(r.removeFromLeft(24));
    quantize_.setBounds(r.removeFromRight(28));
    r.removeFromRight(4);
    record_.setBounds(r.removeFromRight(40));
}

std::vector<roll::ClipSpan> PianoRollEditor::clipSpans() const {
    std::vector<roll::ClipSpan> spans;
    for (const auto& c : host_.clips().list(name_)) spans.push_back({c.startTick, c.lengthTicks, c.looped});
    return spans;
}

int PianoRollEditor::durationTicks() const {
    const auto* cm = host_.model().byName(name_);
    const int patternTicks = cm != nullptr && cm->pattern.present ? cm->pattern.duration : 0;
    return roll::durationOf(clipSpans(), clip_, patternTicks);
}

double PianoRollEditor::playheadClipTick() const { return roll::clipTick(clipSpans(), clip_, host_.positionBeats()); }

PianoRollEditor::Playhead PianoRollEditor::playhead() const {
    return roll::playheadAt(clipSpans(), clip_, host_.positionBeats(), durationTicks(), host_.isPlaying());
}

int PianoRollEditor::snapTicks() const {
    switch (snap_.getSelectedId()) {
        case 1: return Pattern::kTicksPerBeat;
        case 2: return Pattern::kTicksPerBeat / 2;
        case 4: return Pattern::kTicksPerBeat / 8;
        default: return Pattern::kTicksPerBeat / 4;
    }
}

void PianoRollEditor::timerCallback() {
    syncRecordButtons();
    if (++loopTick_ % 8 == 0) updateLoopButton();
    if (host_.midi().isRecordTarget(name_)) {
        model_.followRecording(geometry().rows());
    }
    const double tick = playhead().tick;
    if (std::abs(tick - lastPlayheadTick_) > 0.5 || host_.midi().isRecordTarget(name_)) {
        lastPlayheadTick_ = tick;
        repaint();
    }
}

}
