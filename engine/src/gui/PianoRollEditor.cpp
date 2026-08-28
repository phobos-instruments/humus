#include "gui/PianoRollEditor.h"

#include <cmath>

#include "gui/LookAndFeel.h"

namespace hum {

PianoRollEditor::PianoRollEditor(EngineHost& host, std::string organism)
    : host_(host), name_(std::move(organism)), keyboard_(host, name_) {
    host_.patterns().ensureNote(name_);
    setOpaque(true);
    buildToolbar();
    setWantsKeyboardFocus(true);
    setTool(Tool::Pointer);
    startTimerHz(30);
}

PianoRollEditor::~PianoRollEditor() {
    releaseKey();
    if (host_.midi().isRecordTarget(name_)) host_.midi().setRecordTarget(name_, false);
}

void PianoRollEditor::loopTap() {
    const bool armed = host_.midi().isRecordTarget(name_);
    if (armed && loopTakeOpen_) {
        host_.midi().setRecordTarget(name_, false);
        loopTakeOpen_ = false;
        const int bars = looperCloseBars(host_.positionBeats(), 4);
        const int L = bars * 4 * Pattern::kTicksPerBeat;
        host_.clips().setNotes(name_, clip_, wrapNotesIntoLoop(notes(), L), L);
        reloadValues();
    } else if (armed) {
        host_.midi().setRecordTarget(name_, false);
    } else if (notes().empty()) {
        host_.ensureAudio();
        host_.pushUndo();
        host_.clips().setNotes(name_, clip_, {}, 64 * 4 * Pattern::kTicksPerBeat);
        host_.setPositionBeats(0.0);
        if (!host_.isPlaying()) host_.play();
        host_.midi().setRecordTarget(name_, true,
            quantize_.getToggleState() ? snapTicks() : 0, clip_, true);
        loopTakeOpen_ = true;
    } else {
        host_.ensureAudio();
        if (!host_.isPlaying()) host_.play();
        host_.midi().setRecordTarget(name_, true,
            quantize_.getToggleState() ? snapTicks() : 0, clip_, true);
    }
    updateLoopButton();
}

void PianoRollEditor::loopClear() {
    juce::PopupMenu m;
    m.addItem(1, "Clear loop");
    m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
        if (r != 1) return;
        if (host_.midi().isRecordTarget(name_)) host_.midi().setRecordTarget(name_, false);
        loopTakeOpen_ = false;
        host_.pushUndo();
        host_.clips().setNotes(name_, clip_, {}, 4 * 4 * Pattern::kTicksPerBeat);
        reloadValues();
        updateLoopButton();
    });
}

void PianoRollEditor::updateLoopButton() {
    const auto st = loopState();
    const char* text = "Loop";
    juce::Colour on = Palette::accentDim;
    bool lit = true;
    switch (st) {
        case LooperState::Empty: text = "Loop"; lit = false; break;
        case LooperState::Rec:   text = "Rec";  on = juce::Colour(0xffb04040); break;
        case LooperState::Play:  text = "Play"; on = Palette::accentDim; break;
        case LooperState::Dub:   text = "Dub";  on = juce::Colour(0xffb07030); break;
    }
    juce::String t(text);
    if (st == LooperState::Rec)
        t << " " << juce::jmax(1, (int) std::lround(host_.positionBeats() / 4.0));
    loop_.setButtonText(t);
    loop_.setColour(juce::TextButton::buttonOnColourId, on);
    loop_.setToggleState(lit, juce::dontSendNotification);
}

void PianoRollEditor::buildToolbar() {
    for (auto* l : {&barsLabel_, &snapLabel_}) {
        l->setColour(juce::Label::textColourId, Palette::textDim);
        l->setFont(juce::Font(juce::FontOptions(11.0f)));
        addAndMakeVisible(*l);
    }
    for (int b : {1, 2, 4, 8, 16, 32, 64}) bars_.addItem(juce::String(b), b);
    bars_.onChange = [this] {
        const int bars = bars_.getSelectedId();
        if (bars <= 0) return;
        host_.pushUndo();
        host_.setParam(name_, "Bars", bars);
        host_.clips().setNotes(name_, clip_, notes(), bars * 4 * Pattern::kTicksPerBeat);
        repaint();
    };
    addAndMakeVisible(bars_);

    int id = 1;
    for (const char* s : {"1/4", "1/8", "1/16", "1/32"}) snap_.addItem(s, id++);
    snap_.setSelectedId(3, juce::dontSendNotification);
    snap_.onChange = [this] { repaint(); };
    addAndMakeVisible(snap_);

    loop_.setClickingTogglesState(false);
    loop_.setTooltip("Loop recorder: tap to record, tap to close the loop, "
                     "tap to overdub (R). Right-click: clear.");
    loop_.onClick = [this] { loopTap(); };
    loop_.onRightClick = [this] { loopClear(); };
    addAndMakeVisible(loop_);
    updateLoopButton();

    record_.setClickingTogglesState(true);
    record_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffb04040));
    record_.setTooltip("Record incoming MIDI into this clip");
    record_.onClick = [this] {
        const bool on = record_.getToggleState();
        if (on) {
            host_.ensureAudio();
            if (!host_.isPlaying()) host_.play();
        }
        host_.midi().setRecordTarget(name_, on,
                                  quantize_.getToggleState() ? snapTicks() : 0, clip_);
    };
    addAndMakeVisible(record_);

    quantize_.setClickingTogglesState(true);
    quantize_.setToggleState(true, juce::dontSendNotification);
    quantize_.setTooltip("Quantize recorded notes to the snap grid");
    addAndMakeVisible(quantize_);

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
    reloadValues();
    fitPitch();
}

void PianoRollEditor::fitPitch() {
    if (pitchScrolled_) return;
    const auto n = notes();
    if (n.empty()) return;
    int lo = 127, hi = 0;
    for (const auto& e : n) { lo = std::min(lo, e.pitch); hi = std::max(hi, e.pitch); }
    const int rows = juce::jmax(1, (gridBottom() - gridTop()) / kRowH);
    const int want = hi + (rows - (hi - lo + 1)) / 2;
    topPitch_ = juce::jlimit(juce::jmax(0, rows - 1), 127,
                             hi - lo + 1 <= rows ? want : hi + 1);
    repaint();
}

void PianoRollEditor::reloadValues() {
    const int count = (int) host_.clips().list(name_).size();
    if (count > 0 && clip_ >= count) clip_ = count - 1;
    const int bars = juce::jlimit(1, 64,
        (durationTicks() + 2 * Pattern::kTicksPerBeat) / (4 * Pattern::kTicksPerBeat));
    bars_.setSelectedId(bars, juce::dontSendNotification);
    repaint();
}

int PianoRollEditor::preferredContentHeight(int) const {
    return kToolbarH + kRulerH + kVisibleRows * kRowH + kVelH + kKbH;
}

void PianoRollEditor::soundKey(int pitch) {
    pitch = juce::jlimit(0, 127, pitch);
    if (pitch == keyNote_) return;
    releaseKey();
    keyNote_ = pitch;
    host_.injectLiveMidiToNode(name_, juce::MidiMessage::noteOn(1, pitch, (juce::uint8) 100));
    repaintKeys();
}

void PianoRollEditor::releaseKey() {
    if (keyNote_ < 0) return;
    host_.injectLiveMidiToNode(name_, juce::MidiMessage::noteOff(1, keyNote_));
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

int PianoRollEditor::durationTicks() const {
    const auto cs = host_.clips().list(name_);
    if (clip_ >= 0 && clip_ < (int) cs.size()) return cs[(size_t) clip_].lengthTicks;
    if (auto* cm = host_.model().byName(name_))
        if (cm->pattern.present && cm->pattern.duration > 0) return cm->pattern.duration;
    return 4 * 4 * Pattern::kTicksPerBeat;
}

double PianoRollEditor::playheadClipTick() const {
    const auto cs = host_.clips().list(name_);
    if (clip_ < 0 || clip_ >= (int) cs.size()) return -1.0;
    const auto& ci = cs[(size_t) clip_];
    double rel = host_.positionBeats() * Pattern::kTicksPerBeat - ci.startTick;
    if (rel < 0.0) return -1.0;
    if (ci.looped) return std::fmod(rel, (double) ci.lengthTicks);
    return rel < (double) ci.lengthTicks ? rel : -1.0;
}

double PianoRollEditor::ppt() const {
    const int w = juce::jmax(120, getWidth() - kKeyW);
    return (double) w / (double) juce::jmax(1, durationTicks());
}

int PianoRollEditor::snapTicks() const {
    switch (snap_.getSelectedId()) {
        case 1: return Pattern::kTicksPerBeat;
        case 2: return Pattern::kTicksPerBeat / 2;
        case 4: return Pattern::kTicksPerBeat / 8;
        default: return Pattern::kTicksPerBeat / 4;
    }
}

int PianoRollEditor::snapTick(int tick) const {
    const int s = snapTicks();
    return juce::jlimit(0, durationTicks() - 1, (tick / s) * s);
}

void PianoRollEditor::commit(const std::vector<NoteEvent>& n) {
    host_.clips().setNotes(name_, clip_, n, durationTicks());
    repaint();
}

int PianoRollEditor::noteAt(int tick, int pitch, noteedit::Grab& grab, int x) const {
    const auto n = notes();
    grab = noteedit::Grab::Miss;
    for (int i = (int) n.size() - 1; i >= 0; --i) {
        const auto& e = n[(size_t) i];
        if (e.pitch != pitch) continue;
        if (tick < e.tick || tick >= e.tick + e.lengthTicks) continue;
        const auto b = noteBounds(e);
        grab = noteedit::grabAt(b, {(float) x, b.getCentreY()});
        return i;
    }
    return -1;
}

void PianoRollEditor::timerCallback() {
    if (++loopTick_ % 8 == 0) updateLoopButton();
    if (host_.midi().isRecordTarget(name_)) {
        const auto n = notes();
        if (!n.empty()) {
            const int rows = juce::jmax(1, (gridBottom() - gridTop()) / kRowH);
            const int p = n.back().pitch;
            if (p > topPitch_)
                topPitch_ = juce::jlimit(rows - 1, 127, p + 4);
            else if (p < topPitch_ - rows + 1)
                topPitch_ = juce::jlimit(rows - 1, 127, p + rows / 2);
        }
    }
    const double tick = playheadClipTick();
    if (std::abs(tick - lastPlayheadTick_) > 0.5 || host_.midi().isRecordTarget(name_)) {
        lastPlayheadTick_ = tick;
        repaint();
    }
}

}
