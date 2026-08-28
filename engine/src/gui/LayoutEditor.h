#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/Registry.h"
#include "core/ParamSchema.h"
#include "gui/OrganismEditor.h"
#include "gui/EngineHost.h"
#include "hum/LayoutSpec.h"
#include "hum/Number.h"
#include "gui/LookAndFeel.h"
#include "gui/ParamReset.h"
#include "gui/ParamSlider.h"
#include "gui/TapButton.h"
#include "gui/DeckView.h"
#include "gui/DeckControls.h"
#include "gui/FaderBank.h"
#include "gui/LooperBricks.h"
#include "gui/MidiKeyboardPanel.h"
#include "gui/PatternEditor.h"
#include "gui/PatternStepGrid.h"
#include "gui/PianoRollEditor.h"
#include "gui/PitchFader.h"
#include "gui/WaveformDisplay.h"
#include "gui/FileTransport.h"
#include "gui/GainShapeBrick.h"
#include "gui/HandGestureBrick.h"
#include "gui/RangeSlider.h"
#include "gui/RootCollar.h"
#include "gui/SequenceGridBrick.h"
#include "gui/RhythmicUnitPicker.h"
#include "gui/BankBrowser.h"
#include "gui/ScaleBrowser.h"
#include "gui/SoundFileSlot.h"
#include "hum/dsp/LevelMeter.h"

namespace hum {

class LiveSourceCombo : public juce::ComboBox {
public:
    std::function<void()> refreshItems;
    void showPopup() override {
        if (refreshItems) refreshItems();
        juce::ComboBox::showPopup();
    }
};

class LayoutEditor : public OrganismEditor {
public:
    LayoutEditor(EngineHost& host, std::string organism, LayoutSpec spec)
        : host_(host), name_(std::move(organism)), spec_(std::move(spec)) {
        build();
    }

    void reloadValues() override {
        for (auto& c : controls_) {
            if (c.slider) {
                c.slider->setValue(modelValue(c.param), juce::dontSendNotification);
                syncValueLabel(c);
            }
            if (c.combo && !c.param.empty()) {
                if (auto* live = dynamic_cast<LiveSourceCombo*>(c.combo.get()))
                    if (live->refreshItems) live->refreshItems();
                c.combo->setSelectedId((int) modelValue(c.param) + (c.comboIdsAreValues ? 0 : 1),
                                       juce::dontSendNotification);
            }
            if (c.range) c.range->refresh();
            if (c.toggle) c.toggle->setToggleState(modelValue(c.param) >= 0.5, juce::dontSendNotification);
            if (c.textToggle) {
                c.textToggle->setToggleState(modelValue(c.param) >= 0.5, juce::dontSendNotification);
                if (c.textToggle->onStateChange) c.textToggle->onStateChange();
            }
            if (!c.radioRow.empty()) {
                const int cur = (int) std::lround(modelValue(c.param));
                for (size_t j = 0; j < c.radioRow.size(); ++j)
                    c.radioRow[j]->setToggleState((int) j == cur, juce::dontSendNotification);
            }
            if (c.rhythmic) c.rhythmic->refresh();
            if (c.file) c.file->refresh();
            if (c.scaleFile) c.scaleFile->refresh();
            if (c.bankFile) c.bankFile->refresh();
            if (c.looperTracks) c.looperTracks->reload();
            if (c.stepGrid) c.stepGrid->reload();
            if (c.rich) c.rich->reloadValues();
        }
        applyDims();
    }

    void reloadTextValues() override {
        for (auto& c : controls_) {
            if (c.file) c.file->refresh();
            if (c.scaleFile) c.scaleFile->refresh();
            if (c.bankFile) c.bankFile->refresh();
            if (c.combo && !c.param.empty())
                if (auto* live = dynamic_cast<LiveSourceCombo*>(c.combo.get()))
                    if (live->refreshItems) live->refreshItems();
        }
        applyDims();
    }

    void refreshAutomatedValues() override {
        bool gateMoved = false;
        constexpr int kMaxMeters = hum::LevelMeter::kMax;
        float meters[kMaxMeters] = {};
        const int meterCh = meters_ ? host_.nodeMeter(name_, meters, kMaxMeters) : 0;
        for (auto& c : controls_) {
            if (c.slider && c.meterChannel >= 0)
                c.slider->setMeterLevel(c.meterChannel < meterCh
                                            ? meters[c.meterChannel] : 0.0f);
            if (c.slider) {
                const bool tracked = host_.isLiveTracked(name_, c.param);
                c.slider->setExternallyControlled(
                    host_.isExternallyControlled(name_, c.param));
                c.slider->setRollLocked(host_.rollLocked(name_, c.param));
                if (tracked) {
                    double v = host_.liveParamValue(name_, c.param);
                    if (std::abs(v - c.slider->getValue()) > 1e-9) {
                        c.slider->setValue(v, juce::dontSendNotification);
                        syncValueLabel(c);
                        ember::stamp(*c.slider);
                    }
                }
            }
            if (juce::Button* b = c.toggle ? (juce::Button*) c.toggle.get()
                                           : (juce::Button*) c.textToggle.get();
                b != nullptr && host_.isLiveTracked(name_, c.param)) {
                const bool on = host_.liveParamValue(name_, c.param) >= 0.5;
                if (b->getToggleState() != on) {
                    b->setToggleState(on, juce::dontSendNotification);
                    gateMoved = true;
                }
            }
            if (!c.radioRow.empty() && host_.isLiveTracked(name_, c.param)) {
                const int cur = (int) std::lround(host_.liveParamValue(name_, c.param));
                if (cur >= 0 && cur < (int) c.radioRow.size()
                    && !c.radioRow[(size_t) cur]->getToggleState()) {
                    for (size_t j = 0; j < c.radioRow.size(); ++j)
                        c.radioRow[j]->setToggleState((int) j == cur, juce::dontSendNotification);
                    gateMoved = true;
                }
            }
            if (c.range) c.range->refresh();
            if (c.rich) c.rich->refreshAutomatedValues();
        }
        runClearRules();
        if (gateMoved) applyDims();
    }

    bool meters_ = false;

    void openClip(int clip) override {
        for (auto& c : controls_)
            if (c.rich) c.rich->openClip(clip);
    }

    int preferredContentWidth() const override { return spec_.width; }
    int preferredContentHeight(int width) const override {
        const int c = collarHeight();
        if (spec_.resize == LayoutSpec::Resize::Stretch) return spec_.height + c;
        return (int) std::lround(spec_.height * scale(width)) + c;
    }

    void paint(juce::Graphics& g) override {
        collar::paintSoil(g, getLocalBounds().toFloat(), familyOf(className()));
        if (wearsCollar())
            collar::paint(g, host_, name_, getLocalBounds().removeFromTop(collar::kHeight));
    }

    int collarHeight() const { return wearsCollar() ? collar::kHeight : 0; }

private:
    double scale(int width) const {
        if (spec_.width <= 0) return 1.0;
        return std::min(1.0, (double) width / (double) spec_.width);
    }

    struct Control {
        std::string param;
        std::string dimWhen;
        std::string clearWhen;
        int clearLatch = -1;
        int meterChannel = -1;
        bool showValue = false;
        int textBoxWant = 48;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<ParamSlider> slider;
        std::unique_ptr<RangeSlider> range;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::TextButton> textToggle;
        std::vector<std::unique_ptr<juce::Button>> radioRow;
        std::unique_ptr<RhythmicUnitPicker> rhythmic;
        std::unique_ptr<SoundFileSlot> file;
        std::unique_ptr<juce::Button> stepPrev, stepNext;
        std::unique_ptr<ScaleFileSlot> scaleFile;
        std::unique_ptr<BankFileSlot> bankFile;
        std::unique_ptr<FileTransport> transport;
        std::unique_ptr<DeckView> deck;
        std::unique_ptr<PitchFader> deckPitch;
        std::unique_ptr<DeckControls> deckControls;
        std::unique_ptr<MidiKeyboardStrip> keyboard;
        std::unique_ptr<FaderBank> faderBank;
        std::unique_ptr<WaveformDisplay> waveform;
        std::unique_ptr<juce::ComboBox> combo;
        bool comboIdsAreValues = false;
        std::unique_ptr<MomentaryButton> momentary;
        std::unique_ptr<TapButton> tap;
        std::unique_ptr<LooperTrackStrip> looperTracks;
        std::unique_ptr<PatternStepGrid> stepGrid;
        std::unique_ptr<GainShapeBrick> gainShape;
        std::unique_ptr<SequenceGridBrick> seqGrid;
        std::unique_ptr<HandGestureBrick> handGestures;
        std::unique_ptr<OrganismEditor> rich;
    };

public:
    bool dimmedBy(const std::string& dimWhen) const {
        if (dimWhen.empty()) return false;
        for (size_t at = 0; at <= dimWhen.size();) {
            const auto amp = dimWhen.find('&', at);
            if (!clauseHolds(dimWhen.substr(at, amp == std::string::npos ? amp : amp - at)))
                return false;
            if (amp == std::string::npos) break;
            at = amp + 1;
        }
        return true;
    }

    bool clauseHolds(const std::string& clause) const {
        if (clause.empty()) return true;
        const bool inv = clause[0] == '!';
        std::string gate = inv ? clause.substr(1) : clause;
        bool on;
        if (!gate.empty() && gate[0] == '@') {
            on = Registry::instance().flag(gate.substr(1));
        } else if (const auto eq = gate.find('='); eq != std::string::npos) {
            const double want = scanDouble(gate.substr(eq + 1).c_str());
            gate = gate.substr(0, eq);
            on = std::abs(modelValue(gate) - want) < 0.5;
        } else {
            on = modelValue(gate) >= 0.5;
        }
        return on != inv;
    }

    juce::ComboBox* comboFor(const std::string& param) const {
        for (const auto& c : controls_)
            if (c.param == param && c.combo) return c.combo.get();
        return nullptr;
    }
    float alphaFor(const std::string& param) const {
        for (const auto& c : controls_) {
            if (c.param != param) continue;
            if (c.file) return c.file->getAlpha();
            if (c.combo) return c.combo->getAlpha();
            if (c.slider) return c.slider->getAlpha();
            if (c.toggle) return c.toggle->getAlpha();
            if (c.bankFile) return c.bankFile->getAlpha();
        }
        return 1.0f;
    }
    bool enabledFor(const std::string& param) const {
        for (const auto& c : controls_) {
            if (c.param != param) continue;
            if (c.slider) return c.slider->isEnabled();
            if (c.combo) return c.combo->isEnabled();
            if (c.range) return c.range->isEnabled();
            if (c.toggle) return c.toggle->isEnabled();
            if (c.rhythmic) return c.rhythmic->isEnabled();
            if (c.deckPitch) return c.deckPitch->isEnabled();
            if (c.file) return c.file->isEnabled();
            if (c.scaleFile) return c.scaleFile->isEnabled();
            if (c.bankFile) return c.bankFile->isEnabled();
        }
        return true;
    }
    bool fileSlotShows(const std::string& param, const std::string& fileName) const {
        for (const auto& c : controls_)
            if (c.param == param && c.file)
                return c.file->shownName() == juce::String(juce::CharPointer_UTF8(fileName.c_str()));
        return false;
    }
    void stepCombo(const std::string& param, bool forward) {
        for (const auto& c : controls_)
            if (c.param == param) {
                if (auto* b = forward ? c.stepNext.get() : c.stepPrev.get())
                    if (b->onClick) b->onClick();
                return;
            }
    }

private:
    void runClearRules() {
        for (auto& c : controls_) {
            if (c.clearWhen.empty() || c.param.empty()) continue;
            const int now = dimmedBy(c.clearWhen) ? 1 : 0;
            const int was = c.clearLatch;
            c.clearLatch = now;
            if (was != 1 && now == 1 && was != -1
                && !host_.liveParamText(name_, c.param).empty())
                host_.setParamText(name_, c.param, "");
        }
    }

    void applyDims() {
        runClearRules();
        for (auto& c : controls_) {
            if (c.dimWhen.empty()) continue;
            const bool dim = dimmedBy(c.dimWhen);
            auto apply = [dim](juce::Component* k) {
                if (k == nullptr) return;
                k->setAlpha(dim ? 0.35f : 1.0f);
                k->setEnabled(!dim);
            };
            apply(c.slider.get());
            apply(c.combo.get());
            apply(c.label.get());
            apply(c.file.get());
            apply(c.scaleFile.get());
            apply(c.bankFile.get());
            apply(c.toggle.get());
            apply(c.textToggle.get());
            apply(c.range.get());
            apply(c.rich.get());
            apply(c.momentary.get());
            apply(c.rhythmic.get());
            apply(c.stepPrev.get());
            apply(c.stepNext.get());
            apply(c.deck.get());
            apply(c.deckPitch.get());
            apply(c.deckControls.get());
            apply(c.faderBank.get());
            apply(c.gainShape.get());
            apply(c.handGestures.get());
            apply(c.keyboard.get());
            apply(c.looperTracks.get());
            apply(c.seqGrid.get());
            apply(c.stepGrid.get());
            apply(c.tap.get());
            apply(c.transport.get());
            apply(c.waveform.get());
            for (auto& b : c.radioRow) apply(b.get());
        }
    }

    static void syncValueLabel(Control& c) {
        if (c.showValue && c.label && c.slider)
            c.label->setText(c.slider->getTextFromValue(c.slider->getValue()),
                             juce::dontSendNotification);
    }

    void build();
    bool buildBrick(const LayoutSpec::Control& s, Control& c);
    void resized() override;

    double modelValue(const std::string& param) const {
        if (auto* cm = host_.model().byName(name_)) {
            for (auto& pr : cm->properties)
                if (pr.name == param) {
                    for (auto& d : schemaFor(cm->classRaw))
                        if (d.name == param && d.isText) return pr.text.empty() ? 0.0 : 1.0;
                    return pr.value;
                }
            for (auto& d : schemaFor(cm->classRaw)) if (d.name == param) return d.def;
        }
        return 0.0;
    }

    std::string className() const {
        if (auto* cm = host_.model().byName(name_)) return cm->displayClass;
        return {};
    }

    EngineHost& host_;
    std::string name_;
    LayoutSpec spec_;
    std::vector<Control> controls_;
};

}
