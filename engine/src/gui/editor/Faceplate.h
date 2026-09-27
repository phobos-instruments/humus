// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gui/editor/FaceplateOwner.h"
#include "gui/editor/LayoutCondition.h"
#include "gui/editor/views/BrickViews.h"
#include "gui/host/ModelHost.h"
#include "hum/LayoutSpec.h"

namespace hum {

class Faceplate : private RichOwner {
public:
    Faceplate(ModelHost& host, std::string organism, const LayoutSpec& spec, BrickViews& views,
              FaceplateOwner& owner);

    void build();
    void reloadValues();
    void reloadTextValues();
    void refreshLive();
    void openClip(int clip);
    void layout(int width, int height, int collar);

    std::string className() const override;
    bool holds(const LayoutCondition& condition) const;
    bool dimmedBy(const std::string& expr) const { return holds(LayoutCondition::parse(expr, spec_)); }

    double shownValue(const std::string& param) const;
    float alphaOf(const std::string& param) const;
    bool enabledOf(const std::string& param) const;
    bool fileSlotShows(const std::string& param, const std::string& fileName) const;
    void stepCombo(const std::string& param, bool forward);
    ComboView* comboOf(const std::string& param) const;
    std::vector<BrickView*> viewsAt(size_t index) { return index < controls_.size() ? viewsOf(controls_[index]) : std::vector<BrickView*>{}; }

private:
    struct Control {
        std::string param;
        LayoutCondition dimWhen, showWhen, clearWhen;
        std::vector<BrickView*> shownViews;
        int enumFirst = 0;
        int clearLatch = -1;
        int meterChannel = -1;
        int labelWant = -1;
        bool showValue = false;
        int textBoxWant = 48;
        std::unique_ptr<LabelView> label;
        std::unique_ptr<ValueView> value;
        std::unique_ptr<ToggleView> toggle;
        std::unique_ptr<ChoiceButtonsView> buttons;
        std::unique_ptr<ComboView> combo;
        std::function<void()> refillCombo;
        bool comboIdsAreValues = false;
        std::unique_ptr<MomentaryView> momentary;
        std::function<void()> refreshMomentary;
        std::unique_ptr<RichView> rich;
    };

    static std::vector<BrickView*> viewsOf(Control& c);
    static void syncValueLabel(Control& c);
    void applyDims();
    void runClearRules();
    std::optional<double> resetValueFor(const std::string& param) const;
    void openMenu(const std::string& param, Point at) const;
    std::string organism() const override { return name_; }
    void automationChanged() override { owner_.automationChanged(); }
    void reloadFace() override { reloadValues(); }
    void repaintFace() override { owner_.repaintFace(); }

    void buildControl(const LayoutSpec::Control& s, Control& c, int family);
    void buildMomentary(const LayoutSpec::Control& s, Control& c);
    void buildKnob(const LayoutSpec::Control& s, Control& c, int family);
    void buildRotarySwitch(const LayoutSpec::Control& s, Control& c, int family);
    void buildToggle(const LayoutSpec::Control& s, Control& c, int family);
    void buildEnumButtons(const LayoutSpec::Control& s, Control& c, int family);
    void buildSpinner(const LayoutSpec::Control& s, Control& c);
    void buildCombo(const LayoutSpec::Control& s, Control& c);
    void buildReclassCombo(const LayoutSpec::Control& s, Control& c, const std::string& key);

    ModelHost& host_;
    std::string name_;
    const LayoutSpec& spec_;
    BrickViews& views_;
    FaceplateOwner& owner_;
    std::vector<Control> controls_;
    bool meters_ = false;
};

}
