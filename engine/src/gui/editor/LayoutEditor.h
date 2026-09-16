// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "hum/Registry.h"
#include "core/params/ParamSchema.h"
#include "core/params/ParamUnit.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/host/EditorHost.h"
#include "hum/LayoutSpec.h"
#include "gui/editor/LayoutCondition.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/Skin.h"
#include "gui/editor/ParamSlider.h"
#include "gui/editor/RangeSlider.h"

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
    LayoutEditor(EditorHost& host, std::string organism, LayoutSpec spec)
        : host_(host), name_(std::move(organism)), spec_(std::move(spec)),
          skin_(Skin::forBlueprint(spec_)) {
        setOpaque(true);
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
                const int cur = (int) std::lround(modelValue(c.param)) - c.enumFirst;
                for (size_t j = 0; j < c.radioRow.size(); ++j)
                    c.radioRow[j]->setToggleState((int) j == cur, juce::dontSendNotification);
            }
            if (c.reloadBrick) c.reloadBrick();
            if (c.rich) c.rich->reloadValues();
        }
        applyDims();
    }

    void reloadTextValues() override {
        for (auto& c : controls_) {
            if (c.reloadBrick) c.reloadBrick();
            if (c.combo && !c.param.empty())
                if (auto* live = dynamic_cast<LiveSourceCombo*>(c.combo.get()))
                    if (live->refreshItems) live->refreshItems();
        }
        applyDims();
    }

    double shownValueForTest(const std::string& param) const {
        for (const auto& c : controls_)
            if (c.param == param && c.slider) return c.slider->getValue();
        return -1.0;
    }

    void refreshAutomatedValues() override;

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

    void paint(juce::Graphics& g) override;

    const Skin* skinForTest() const { return skin_.empty() ? nullptr : &skin_; }

    int collarHeight() const;

    static constexpr double kMaxGrow = 2.5;

private:
    struct Fit { double kx = 1.0, ky = 1.0; int xOff = 0; };
    Fit fit() const {
        Fit f;
        const bool stretch = spec_.resize == LayoutSpec::Resize::Stretch;
        f.kx = stretch ? (spec_.width > 0 ? (double) getWidth() / spec_.width : 1.0)
                       : scale(getWidth());
        f.ky = stretch ? 1.0 : f.kx;
        f.xOff = stretch ? 0
                         : juce::jmax(0, (getWidth() - (int) std::lround(spec_.width * f.kx)) / 2);
        return f;
    }
    void paintSkin(juce::Graphics& g);

    double scale(int width) const {
        if (spec_.width <= 0) return 1.0;
        const double want = (double) width / (double) spec_.width;
        if (spec_.resize == LayoutSpec::Resize::Grow) return std::min(want, kMaxGrow);
        return std::min(1.0, want);
    }

    struct Control {
        std::string param;
        LayoutCondition dimWhen;
        LayoutCondition showWhen;
        std::vector<juce::Component*> shownParts;
        int enumFirst = 0;
        LayoutCondition clearWhen;
        int clearLatch = -1;
        int meterChannel = -1;
        int labelWant = -1;
        bool showValue = false;
        int textBoxWant = 48;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<ParamSlider> slider;
        std::unique_ptr<RangeSlider> range;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::TextButton> textToggle;
        std::vector<std::unique_ptr<juce::Button>> radioRow;
        std::unique_ptr<juce::Button> stepPrev, stepNext;
        std::unique_ptr<juce::ComboBox> combo;
        bool comboIdsAreValues = false;
        std::unique_ptr<OrganismEditor> rich;
        std::unique_ptr<juce::Component> brick;
        std::function<void()> reloadBrick;
        std::function<void()> refreshBrickLive;

        template <class Widget>
        Widget* adopt(std::unique_ptr<Widget> w) {
            auto* raw = w.get();
            brick = std::move(w);
            return raw;
        }
    };

public:
    bool dimmedBy(const std::string& expr) const {
        return holds(LayoutCondition::parse(expr, spec_));
    }

    bool holds(const LayoutCondition& condition) const {
        return !condition.empty()
               && condition.holds([this](const std::string& p) { return modelValue(p); },
                                  [](const std::string& f) { return Registry::instance().flag(f); });
    }

    juce::ComboBox* comboFor(const std::string& param) const {
        for (const auto& c : controls_)
            if (c.param == param && c.combo) return c.combo.get();
        return nullptr;
    }
    float alphaFor(const std::string& param) const {
        for (const auto& c : controls_) {
            if (c.param != param) continue;
            if (c.brick) return c.brick->getAlpha();
            if (c.combo) return c.combo->getAlpha();
            if (c.slider) return c.slider->getAlpha();
            if (c.toggle) return c.toggle->getAlpha();
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
            if (c.brick) return c.brick->isEnabled();
        }
        return true;
    }
    bool fileSlotShows(const std::string& param, const std::string& fileName) const;
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
            const int now = holds(c.clearWhen) ? 1 : 0;
            const int was = c.clearLatch;
            c.clearLatch = now;
            if (was != 1 && now == 1 && was != -1
                && !host_.liveParamText(name_, c.param).empty())
                host_.setParamText(name_, c.param, "");
        }
    }

    struct PillLabel : juce::Label {
        void paint(juce::Graphics& g) override {
            const auto r = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
            juce::Label::paint(g);
        }
    };

    static std::vector<juce::Component*> partsOf(Control& c) {
        std::vector<juce::Component*> out;
        for (juce::Component* k : {(juce::Component*) c.slider.get(), (juce::Component*) c.combo.get(),
                                   (juce::Component*) c.label.get(), (juce::Component*) c.toggle.get(),
                                   (juce::Component*) c.textToggle.get(), (juce::Component*) c.range.get(),
                                   (juce::Component*) c.rich.get(), c.brick.get(),
                                   (juce::Component*) c.stepPrev.get(), (juce::Component*) c.stepNext.get()})
            if (k != nullptr) out.push_back(k);
        for (auto& b : c.radioRow) out.push_back(b.get());
        return out;
    }

    void applyDims() {
        runClearRules();
        for (auto& c : controls_) {
            if (!c.dimWhen.empty()) {
                const bool dim = holds(c.dimWhen);
                for (auto* k : partsOf(c)) {
                    k->setAlpha(dim ? 0.35f : 1.0f);
                    k->setEnabled(!dim);
                }
            }
            if (!c.showWhen.empty()) {
                if (c.shownParts.empty())
                    for (auto* k : partsOf(c))
                        if (k->isVisible()) c.shownParts.push_back(k);
                const bool show = holds(c.showWhen);
                for (auto* k : c.shownParts) k->setVisible(show);
            }
        }
    }

    static void syncValueLabel(Control& c) {
        if (c.showValue && c.label && c.slider)
            c.label->setText(c.slider->getTextFromValue(c.slider->getValue()),
                             juce::dontSendNotification);
    }

    struct ValueSetup {
        std::string pn, cn;
        double lo = 0.0, hi = 1.0;
        bool isInt = false;
        Unit unit = Unit::None;
        int family = 0;
    };

    void build();
    bool buildBrick(const LayoutSpec::Control& s, Control& c);
    void buildValueControl(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildSliderControl(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildRotarySwitch(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildRangeSlider(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildToggle(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildMiniToggle(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildEnumButtons(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildRhythmicUnit(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildSoundFile(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildScaleFile(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildBankFile(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildFileTransport(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildCombo(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void buildSpinner(const LayoutSpec::Control& s, Control& c, const ValueSetup& v);
    void resized() override;
    void setLayoutDeferred(bool deferred) override {
        deferLayout_ = deferred;
        if (!deferred && layoutDirty_) {
            layoutDirty_ = false;
            resized();
        }
    }

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

    EditorHost& host_;
    std::string name_;
    LayoutSpec spec_;
    Skin skin_;
    std::vector<Control> controls_;
    bool deferLayout_ = false, layoutDirty_ = false;
};

}
