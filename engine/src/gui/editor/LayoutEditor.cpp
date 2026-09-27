// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/LayoutEditor.h"

#include <cmath>

#include "core/packs/Categories.h"
#include "core/packs/PackRegistry.h"
#include "gui/bricks/RootCollar.h"
#include "gui/common/PerfLog.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/LayoutModel.h"
#include "gui/editor/MomentaryPress.h"
#include "gui/editor/juce/JuceChoiceViews.h"

namespace hum {

void LayoutEditor::openMenu(const std::string& param, Point at) {
    showAutomateMenu(host_, name_, param, {at.x, at.y}, [this] { automationChanged(); });
}

int LayoutEditor::coverTicks() const {
    auto* device = host_.audioDevices().getCurrentAudioDevice();
    return device == nullptr ? momentary::coverTicksFor(0.0, 0)
                             : momentary::coverTicksFor(device->getCurrentSampleRate(),
                                                        device->getCurrentBufferSizeSamples());
}

bool LayoutEditor::knowsClass(const std::string& className) const {
    return PackRegistry::instance().classManifest(className) != nullptr;
}

void LayoutEditor::paintSkin(juce::Graphics& g) {
    skin_.paintPanel(g, getLocalBounds().toFloat());
    const auto f = layout::fitFor(spec_, getWidth());
    const juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale((float) f.kx, (float) f.ky)
                       .translated((float) f.xOff, (float) collarHeight()));
    skin_.paintFace(g, {0.0f, 0.0f, (float) spec_.width, (float) spec_.height});
}

void LayoutEditor::resized() {
    if (deferLayout_) {
        layoutDirty_ = true;
        return;
    }
    perf::Scope scope("editor.layout");
    face_.layout(getWidth(), getHeight(), collarHeight());
    placeScrolledViews();
}

void LayoutEditor::adoptScrolledViews() {
    if (!scrolls()) return;
    scroller_.setContent(scrollContent_);
    addAndMakeVisible(scroller_);
    for (size_t i = 0; i < spec_.controls.size(); ++i) {
        if (spec_.controls[i].y < spec_.scrollFrom) continue;
        for (auto* view : face_.viewsAt(i))
            for (auto* widget : juceWidgetsOf(view)) {
                const bool shown = widget->isVisible();
                scrollContent_.addChildComponent(widget);
                widget->setVisible(shown);
                scrolled_.push_back(widget);
            }
    }
}

int LayoutEditor::scrollTop() const {
    const auto f = layout::fitFor(spec_, getWidth());
    return collarHeight() + (int) std::lround(spec_.scrollFrom * f.ky);
}

void LayoutEditor::placeScrolledViews() {
    if (!scrolls()) return;
    const int top = scrollTop();
    scroller_.setBounds(0, top, getWidth(), juce::jmax(0, getHeight() - top));
    for (auto* widget : scrolled_) widget->setTopLeftPosition(widget->getX(), widget->getY() - top);
    fitScrollExtent();
}

void LayoutEditor::fitScrollExtent() {
    if (!scrolls()) return;
    int bottom = 0;
    for (auto* widget : scrolled_)
        if (widget->isVisible()) bottom = juce::jmax(bottom, widget->getBottom());
    constexpr int kScrollPad = 6;
    const int height = juce::jmax(scroller_.getHeight(), bottom + kScrollPad);
    if (scroller_.contentHeight() != height) scroller_.setContentHeight(height);
}

void LayoutEditor::paint(juce::Graphics& g) {
    perf::Scope scope("editor.paint");
    if (!skin_.empty()) paintSkin(g);
    else collar::paintSoil(g, getLocalBounds().toFloat(), familyOf(face_.className()));
    if (wearsCollar())
        collar::paint(g, host_, name_, getLocalBounds().removeFromTop(collar::kHeight));
}

int LayoutEditor::collarHeight() const { return wearsCollar() ? collar::kHeight : 0; }

juce::ComboBox* LayoutEditor::comboFor(const std::string& param) const {
    if (auto* view = dynamic_cast<JuceComboView*>(face_.comboOf(param))) return &view->comboBox();
    return nullptr;
}

}
