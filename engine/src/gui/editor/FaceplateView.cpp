// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/FaceplateView.h"

#include <cmath>
#include <utility>

#include "core/packs/Categories.h"
#include "core/packs/PackRegistry.h"
#include "gui/bricks/RootCollar.h"
#include "gui/common/PerfLog.h"
#include "gui/editor/LayoutModel.h"
#include "gui/editor/juce/JuceChoiceViews.h"
#include "gui/editor/juce/JuceView.h"

namespace hum {

FaceplateView::FaceplateView(ModelHost& host, std::string organism, LayoutSpec spec, const ViewsMaker& makeViews)
    : host_(host), name_(std::move(organism)), spec_(std::move(spec)), skin_(Skin::forBlueprint(spec_)),
      views_(makeViews(*this)), face_(host, name_, spec_, *views_, *this) {
    setOpaque(true);
    face_.build();
    adoptScrolledViews();
}

bool FaceplateView::knowsClass(const std::string& className) const {
    return PackRegistry::instance().classManifest(className) != nullptr;
}

void FaceplateView::paintSkin(juce::Graphics& g) {
    skin_.paintPanel(g, getLocalBounds().toFloat());
    const auto f = layout::fitFor(spec_, getWidth());
    const juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale((float) f.kx, (float) f.ky)
                       .translated((float) f.xOff, (float) collarHeight()));
    skin_.paintFace(g, {0.0f, 0.0f, (float) spec_.width, (float) spec_.height});
}

void FaceplateView::resized() {
    if (deferLayout_) {
        layoutDirty_ = true;
        return;
    }
    perf::Scope scope("editor.layout");
    face_.layout(getWidth(), getHeight(), collarHeight());
    placeScrolledViews();
}

void FaceplateView::adoptScrolledViews() {
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

int FaceplateView::scrollTop() const {
    const auto f = layout::fitFor(spec_, getWidth());
    return collarHeight() + (int) std::lround(spec_.scrollFrom * f.ky);
}

void FaceplateView::placeScrolledViews() {
    if (!scrolls()) return;
    const int top = scrollTop();
    scroller_.setBounds(0, top, getWidth(), juce::jmax(0, getHeight() - top));
    for (auto* widget : scrolled_) widget->setTopLeftPosition(widget->getX(), widget->getY() - top);
    fitScrollExtent();
}

void FaceplateView::fitScrollExtent() {
    if (!scrolls()) return;
    int bottom = 0;
    for (auto* widget : scrolled_)
        if (widget->isVisible()) bottom = juce::jmax(bottom, widget->getBottom());
    constexpr int kScrollPad = 6;
    const int height = juce::jmax(scroller_.getHeight(), bottom + kScrollPad);
    if (scroller_.contentHeight() != height) scroller_.setContentHeight(height);
}

void FaceplateView::paint(juce::Graphics& g) {
    perf::Scope scope("editor.paint");
    if (!skin_.empty()) paintSkin(g);
    else collar::paintSoil(g, getLocalBounds().toFloat(), familyOf(face_.className()));
    if (wearsCollar())
        collar::paint(g, host_, name_, getLocalBounds().removeFromTop(collar::kHeight));
}

int FaceplateView::collarHeight() const { return wearsCollar() ? collar::kHeight : 0; }

juce::ComboBox* FaceplateView::comboFor(const std::string& param) const {
    if (auto* view = dynamic_cast<JuceComboView*>(face_.comboOf(param))) return &view->comboBox();
    return nullptr;
}

}
