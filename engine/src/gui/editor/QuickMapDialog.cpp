// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/QuickMapDialog.h"

#include "core/params/ParamUnit.h"
#include "gui/common/Localisation.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/QuickMapWindow.h"
#include "gui/properties/ControlModeText.h"
#include "gui/style/ControlGlyph.h"

namespace hum {

namespace {

constexpr int kSlotH = 136, kHeadH = 56, kModeH = 30, kFootH = 46, kDialogW = 500;

std::unique_ptr<juce::DocumentWindow>& dialogWindow() {
    static std::unique_ptr<juce::DocumentWindow> w;
    return w;
}

class DialogWindow : public juce::DocumentWindow {
public:
    explicit DialogWindow(QuickMapDialog* owned)
        : juce::DocumentWindow(tr("quick-map.title", "Quick-Map MIDI Control"), Palette::panel,
                               juce::DocumentWindow::closeButton) {
        setUsingNativeTitleBar(true);
        setContentOwned(owned, true);
        setAlwaysOnTop(true);
        centreAroundComponent(nullptr, getWidth(), getHeight());
        setVisible(true);
        toFront(true);
    }
    void closeButtonPressed() override {
        if (auto* d = dynamic_cast<QuickMapDialog*>(getContentComponent())) d->cancelForTest();
        QuickMapDialog::close();
    }
};

}

void QuickMapDialog::show(BrickHost& host, const std::string& organism, const std::string& param) {
    if (MidiLearner::instance().armed()) MidiLearner::instance().cancel();
    dialogWindow() = std::make_unique<DialogWindow>(new QuickMapDialog(host, organism, param));
}

void QuickMapDialog::close() {
    if (MidiLearner::instance().armed()) MidiLearner::instance().cancel();
    auto* leaving = dialogWindow().get();
    juce::MessageManager::callAsync([leaving] {
        if (dialogWindow().get() == leaving) dialogWindow().reset();
    });
}

bool QuickMapDialog::isOpen() { return dialogWindow() != nullptr; }

QuickMapDialog::QuickMapDialog(BrickHost& host, std::string organism, std::string param)
    : host_(host), organism_(std::move(organism)), param_(rangeBaseOf(param)) {
    paramLine_.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    paramLine_.setText(juce::String(organism_) + " / " + juce::String(param_),
                       juce::dontSendNotification);
    addAndMakeVisible(paramLine_);

    const auto [lo, hi] = bounds();
    const auto unit = paramUnit(host_, organism_, param_);
    rangeLine_.setFont(juce::FontOptions(11.5f));
    rangeLine_.setColour(juce::Label::textColourId, Palette::textDim);
    rangeLine_.setText(tr("quick-map.total-range", "Total range: ") + unitPlain(unit, lo, lo, hi)
                           + " to " + unitPlain(unit, hi, lo, hi) + " " + unitSuffix(unit),
                       juce::dontSendNotification);
    addAndMakeVisible(rangeLine_);

    devices_ = host_.midi().inputDevices();
    ranged_ = paramIsRange(host_, organism_, param_);

    modeLabel_.setText(tr("quick-map.range-mode", "Range Mode"), juce::dontSendNotification);
    modeLabel_.setFont(juce::FontOptions(11.5f));
    modeLabel_.setColour(juce::Label::textColourId, Palette::textDim);
    mode_.addItem(tr("quick-map.value-spread", "Value / Spread"), 1);
    mode_.addItem(tr("quick-map.min-max", "Minimum / Maximum"), 2);
    mode_.addItem(tr("quick-map.single", "Single"), 3);
    mode_.setSelectedId(idOfMode(mode()), juce::dontSendNotification);
    mode_.onChange = [this] {
        host_.setRangeMode(organism_, param_, modeOfId(mode_.getSelectedId()));
        buildSlots();
        fitToSlots();
        resized();
        refresh();
    };
    modeLabel_.setVisible(ranged_);
    mode_.setVisible(ranged_);
    addChildComponent(modeLabel_);
    addChildComponent(mode_);

    buildSlots();

    for (const auto end : {RangeEnd::Whole, RangeEnd::Spread, RangeEnd::Low, RangeEnd::High}) {
        const auto target = rangeEndParam(param_, end);
        if (const auto* e = entryFor(target))
            before_.push_back({e->source(), target, e->min, e->max, e->shape});
    }

    okBtn_.onClick = [this] { commit(); };
    cancelBtn_.onClick = [this] { revert(); };
    addAndMakeVisible(okBtn_);
    addAndMakeVisible(cancelBtn_);

    refresh();
    fitToSlots();
    startTimerHz(10);
}

void QuickMapDialog::fitToSlots() {
    setSize(kDialogW, kHeadH + (ranged_ ? kModeH : 0) + (int) slots_.size() * kSlotH + kFootH);
}

QuickMapDialog::~QuickMapDialog() {
    if (MidiLearner::instance().armed()) MidiLearner::instance().cancel();
}

RangeMode QuickMapDialog::mode() const { return host_.rangeMode(organism_, param_); }

RangeMode QuickMapDialog::modeOfId(int id) {
    return id == 2 ? RangeMode::MinMax : id == 3 ? RangeMode::Single : RangeMode::ValueSpread;
}

int QuickMapDialog::idOfMode(RangeMode m) {
    return m == RangeMode::MinMax ? 2 : m == RangeMode::Single ? 3 : 1;
}

void QuickMapDialog::buildSlots() {
    slots_.clear();
    const std::vector<RangeEnd> ends =
        !ranged_                       ? std::vector<RangeEnd>{RangeEnd::Whole}
        : mode() == RangeMode::MinMax  ? std::vector<RangeEnd>{RangeEnd::Low, RangeEnd::High}
        : mode() == RangeMode::Single  ? std::vector<RangeEnd>{RangeEnd::Whole}
                                       : std::vector<RangeEnd>{RangeEnd::Whole, RangeEnd::Spread};
    for (const auto end : ends) {
        auto slot = std::make_unique<Slot>();
        buildSlot(*slot, end);
        slots_.push_back(std::move(slot));
    }
}

std::pair<double, double> QuickMapDialog::bounds() const {
    return paramRange(host_, organism_, param_);
}

void QuickMapDialog::buildSlot(Slot& s, RangeEnd end) {
    s.end = end;
    s.param = rangeEndParam(param_, end);
    const auto which = end == RangeEnd::Low      ? tr("quick-map.minimum", " (Minimum)")
                       : end == RangeEnd::High   ? tr("quick-map.maximum", " (Maximum)")
                       : end == RangeEnd::Spread ? tr("quick-map.spread", " (Spread)")
                       : ranged_ && mode() != RangeMode::Single
                                                 ? tr("quick-map.value", " (Value)")
                                                 : juce::String();
    s.head.setText(tr("quick-map.midi-control-source", "MIDI control source") + which,
                   juce::dontSendNotification);
    s.head.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
    s.head.setColour(juce::Label::textColourId, Palette::textDim);
    addAndMakeVisible(s.head);

    s.sourceLabel.setFont(juce::FontOptions(12.5f));
    addAndMakeVisible(s.sourceLabel);

    s.learn.setButtonText(tr("quick-map.learn", "Learn"));
    s.learn.onClick = [this, slot = &s] { armSlot(*slot); };
    addAndMakeVisible(s.learn);

    s.clear.setButtonText(tr("quick-map.clear", "Clear"));
    s.clear.onClick = [this, slot = &s] {
        if (const auto* e = entryFor(slot->param)) {
            host_.midi().clearCC(e->source(), organism_, slot->param);
            refresh();
        }
    };
    addAndMakeVisible(s.clear);

    s.deviceLabel.setText(tr("quick-map.device", "Device"), juce::dontSendNotification);
    s.deviceLabel.setFont(juce::FontOptions(11.5f));
    s.deviceLabel.setColour(juce::Label::textColourId, Palette::textDim);
    addAndMakeVisible(s.deviceLabel);
    s.device.addItem(tr("quick-map.any-device", "Any"), 1);
    for (size_t i = 0; i < devices_.size(); ++i)
        s.device.addItem(juce::String(devices_[i]), (int) i + 2);
    s.device.setSelectedId(1, juce::dontSendNotification);
    s.device.onChange = [this, slot = &s] { applySlot(*slot); };
    addAndMakeVisible(s.device);

    s.numberLabel.setText(tr("quick-map.control-number", "Control number"), juce::dontSendNotification);
    s.numberLabel.setFont(juce::FontOptions(11.5f));
    s.numberLabel.setColour(juce::Label::textColourId, Palette::textDim);
    addAndMakeVisible(s.numberLabel);
    s.number.setInputRestrictions(3, "0123456789");
    s.number.setJustification(juce::Justification::centred);
    s.number.onReturnKey = [this, slot = &s] { applySlot(*slot); };
    s.number.onFocusLost = [this, slot = &s] { applySlot(*slot); };
    addAndMakeVisible(s.number);

    s.howLabel.setText(tr("quick-map.behaves-as", "Behaves as"), juce::dontSendNotification);
    s.howLabel.setFont(juce::FontOptions(11.5f));
    s.howLabel.setColour(juce::Label::textColourId, Palette::textDim);
    addAndMakeVisible(s.howLabel);
    s.type.setOptions(controlTypesFor(ControlFamily::Midi), [](ControlType t) { return modeText(t); },
                      [](juce::Graphics& g, ControlType t, juce::Rectangle<float> r, juce::Colour c) {
                          paintControlGlyph(g, t, r, c);
                      });
    s.type.onPick = [this, slot = &s](ControlType t) { pickType(*slot, t); };
    addAndMakeVisible(s.type);
    s.how.setTooltip(tr("quick-map.how-tooltip", "What the control does to the parameter"));
    s.how.onChange = [this, slot = &s] { pickHow(*slot); };
    addAndMakeVisible(s.how);
}

void QuickMapDialog::showBehaviour(Slot& s, const ControlShape& shape) {
    s.type.setSelected(shape.type);
    s.how.clear(juce::dontSendNotification);
    int selected = 1;
    auto add = [&](auto value, const juce::String& text, bool on) {
        const int id = s.how.getNumItems() + 1;
        s.how.addItem(text, id);
        if (on) selected = id;
        (void) value;
    };
    if (shape.isButton())
        for (auto m : buttonModesFor(ControlFamily::Midi)) add(m, modeText(m), m == shape.button);
    else if (shape.isFader())
        for (auto m : faderModesFor(ControlFamily::Midi)) add(m, modeText(m), m == shape.fader);
    else
        for (const auto& row : ModeTable<EncoderFormat>::rows)
            add(row.value, modeText(row.value), row.value == shape.encoder);
    s.how.setSelectedId(selected, juce::dontSendNotification);
    s.how.setTooltip(behaviourHint(shape));
}

void QuickMapDialog::pickType(Slot& s, ControlType t) {
    const auto* e = entryFor(s.param);
    if (e == nullptr) return;
    auto shape = e->shape;
    if (shape.type == t) return;
    shape.type = t;
    if (t != ControlType::Fader) shape.logScale = false;
    host_.midi().setShape(e->source(), organism_, s.param, shape);
    refresh();
}

void QuickMapDialog::pickHow(Slot& s) {
    const auto* e = entryFor(s.param);
    if (e == nullptr) return;
    auto shape = e->shape;
    const int index = s.how.getSelectedId() - 1;
    if (index < 0) return;
    if (shape.isButton()) shape.button = buttonModesFor(ControlFamily::Midi)[(size_t) index];
    else if (shape.isFader()) shape.fader = faderModesFor(ControlFamily::Midi)[(size_t) index];
    else shape.encoder = ModeTable<EncoderFormat>::rows[(size_t) index].value;
    host_.midi().setShape(e->source(), organism_, s.param, shape);
    refresh();
}

const MidiMapEntry* QuickMapDialog::entryFor(const std::string& param) const {
    for (const auto& e : host_.midi().map().entries())
        if (e.organism == organism_ && e.param == param) return &e;
    return nullptr;
}

void QuickMapDialog::refresh() {
    for (const auto& s : slots_) {
        const auto* e = entryFor(s->param);
        const bool waiting = MidiLearner::instance().armed()
                             && MidiLearner::instance().armedOrganism() == organism_
                             && MidiLearner::instance().armedParam() == s->param;
        s->sourceLabel.setColour(juce::Label::textColourId,
                                 waiting ? Palette::accent : Palette::text);
        s->sourceLabel.setText(waiting ? tr("quick-map.move-a-control", "move a control...")
                                             + " (" + juce::String(MidiLearner::instance().secondsLeft()) + "s)"
                               : e == nullptr ? tr("quick-map.none", "(none)")
                                              : juce::String(midiSourceLabel(e->source())),
                               juce::dontSendNotification);
        s->clear.setEnabled(e != nullptr);
        s->number.setEnabled(e != nullptr);
        s->device.setEnabled(e != nullptr);
        s->type.setEnabled(e != nullptr);
        s->how.setEnabled(e != nullptr);
        if (e != nullptr) showBehaviour(*s, e->shape);
        if (e == nullptr) {
            s->number.setText({}, juce::dontSendNotification);
            s->device.setSelectedId(1, juce::dontSendNotification);
            continue;
        }
        const auto src = e->source();
        s->number.setText(juce::String(sourceNumber(src.cc)), juce::dontSendNotification);
        int id = 1;
        for (size_t i = 0; i < devices_.size(); ++i)
            if (!src.anyPort() && host_.midi().portForDevice(devices_[i]) == src.port) id = (int) i + 2;
        s->device.setSelectedId(id, juce::dontSendNotification);
    }
}

void QuickMapDialog::armSlot(Slot& s) {
    const auto [lo, hi] = endBounds(host_, organism_, param_, s.end);
    MidiLearner::instance().arm(host_, organism_, s.param, lo, hi, false);
    MidiLearner::instance().onCaptured = [this] { refresh(); };
    refresh();
}

void QuickMapDialog::applySlot(Slot& s) {
    const auto* e = entryFor(s.param);
    if (e == nullptr) return;
    const auto was = e->source();
    const double mn = e->min, mx = e->max;
    const auto shape = e->shape;
    const int id = s.device.getSelectedId();
    const int port = id <= 1 ? kAnyMidiPort
                             : host_.midi().portForDevice(devices_[(size_t) id - 2]);
    const int number = juce::jlimit(0, kMidiMax, s.number.getText().getIntValue());
    const int source = isCcSource(was.cc) || isNoteSource(was.cc)
                           ? sourceFromMessage(messageTypeOf(was.cc), number) : was.cc;
    const MidiSource next(source, was.held, port, was.channel);
    if (next.cc == was.cc && midiPortRow(next.port) == midiPortRow(was.port)) return;
    host_.midi().clearCC(was, organism_, s.param);
    host_.midi().mapCC(next, organism_, s.param, mn, mx, false);
    if (!shape.isDefault()) host_.midi().setShape(next, organism_, s.param, shape);
    refresh();
}

void QuickMapDialog::commit() { close(); }

void QuickMapDialog::revert() {
    if (MidiLearner::instance().armed()) MidiLearner::instance().cancel();
    for (const auto& s : slots_)
        if (const auto* e = entryFor(s->param)) host_.midi().clearCC(e->source(), organism_, s->param);
    for (const auto& r : before_) {
        host_.midi().mapCC(r.source, organism_, r.param, r.min, r.max, false);
        if (!r.shape.isDefault()) host_.midi().setShape(r.source, organism_, r.param, r.shape);
    }
    refresh();
    close();
}

void QuickMapDialog::timerCallback() {
    const bool armed = MidiLearner::instance().armed();
    if (armed || armed != wasArmed_) refresh();
    wasArmed_ = armed;
}

void quickMapMidi(BrickHost& host, const std::string& organism, const std::string& param) {
    QuickMapDialog::show(host, organism, param);
}

std::string QuickMapDialog::slotNameForTest(int slot) const {
    if (slot < 0 || slot >= (int) slots_.size()) return {};
    return slots_[(size_t) slot]->param;
}

void QuickMapDialog::setModeForTest(RangeMode want) {
    mode_.setSelectedId(idOfMode(want), juce::sendNotificationSync);
}

std::string QuickMapDialog::slotSourceForTest(int slot) const {
    if (slot < 0 || slot >= (int) slots_.size()) return {};
    return slots_[(size_t) slot]->sourceLabel.getText().toStdString();
}

std::string QuickMapDialog::slotBehaviourForTest(int slot) const {
    if (slot < 0 || slot >= (int) slots_.size()) return {};
    const auto* e = entryFor(slots_[(size_t) slot]->param);
    if (e == nullptr) return {};
    return std::string(modeWord(e->shape.type)) + " " + slots_[(size_t) slot]->how.getText().toStdString();
}

void QuickMapDialog::pickTypeForTest(int slot, ControlType t) {
    if (slot < 0 || slot >= (int) slots_.size()) return;
    pickType(*slots_[(size_t) slot], t);
}

void QuickMapDialog::learnSlotForTest(int slot) {
    if (slot < 0 || slot >= (int) slots_.size()) return;
    armSlot(*slots_[(size_t) slot]);
}

void QuickMapDialog::resized() {
    auto r = getLocalBounds().reduced(10);
    paramLine_.setBounds(r.removeFromTop(20));
    rangeLine_.setBounds(r.removeFromTop(16));
    r.removeFromTop(8);
    if (ranged_) {
        auto modeRow = r.removeFromTop(kModeH - 8);
        modeLabel_.setBounds(modeRow.removeFromLeft(74));
        mode_.setBounds(modeRow.removeFromLeft(190).reduced(0, 1));
        r.removeFromTop(8);
    }
    for (const auto& s : slots_) {
        auto box = r.removeFromTop(kSlotH - 8);
        s->head.setBounds(box.removeFromTop(16));
        box.removeFromTop(4);
        auto top = box.removeFromTop(26);
        s->clear.setBounds(top.removeFromRight(72).reduced(0, 1));
        top.removeFromRight(6);
        s->learn.setBounds(top.removeFromRight(72).reduced(0, 1));
        top.removeFromRight(8);
        s->sourceLabel.setBounds(top);
        box.removeFromTop(6);
        auto low = box.removeFromTop(24);
        s->deviceLabel.setBounds(low.removeFromLeft(48));
        s->device.setBounds(low.removeFromLeft(130).reduced(0, 1));
        low.removeFromLeft(10);
        s->numberLabel.setBounds(low.removeFromLeft(94));
        s->number.setBounds(low.removeFromLeft(52).reduced(0, 1));
        box.removeFromTop(8);
        auto how = box.removeFromTop(26);
        s->howLabel.setBounds(how.removeFromLeft(70));
        s->type.setBounds(how.removeFromLeft(300));
        how.removeFromLeft(6);
        s->how.setBounds(how.reduced(0, 1));
        r.removeFromTop(8);
    }
    auto foot = r.removeFromBottom(26);
    cancelBtn_.setBounds(foot.removeFromRight(76));
    foot.removeFromRight(8);
    okBtn_.setBounds(foot.removeFromRight(76));
}

}
