#include "gui/ParameterPanel.h"

#include <cmath>

#include "core/ParamLabel.h"

#include "core/ParamSchema.h"
#include "core/ParamUnit.h"
#include "gui/LookAndFeel.h"
#include "gui/RootCollar.h"
#include "gui/Localisation.h"

namespace hum {

void ParameterPanel::show(const std::string& name) {
    rows_.clear();
    removeAllChildren();
    organism_ = name;
    if (name.empty()) { repaint(); return; }

    const OrganismModel* cm = host_.model().byName(name);
    if (!cm) { repaint(); return; }

    auto valueOf = [&](const std::string& p, double def) {
        for (auto& pr : cm->properties) if (pr.name == p) return pr.value;
        return def;
    };
    auto textOf = [&](const std::string& p) -> std::string {
        for (auto& pr : cm->properties) if (pr.name == p) return pr.text;
        return {};
    };

    for (const auto& d : schemaFor(cm->classRaw)) {
        Row row;
        row.param = d.name;
        row.label = std::make_unique<juce::Label>();
        row.label->setText(spacedParamName(d.name), juce::dontSendNotification);
        row.label->setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(*row.label);

        row.label->setJustificationType(juce::Justification::centred);
        row.label->setFont(juce::FontOptions(11.0f));
        row.label->setColour(juce::Label::textColourId, Palette::textDim);

        if (d.isText) {
            row.text = std::make_unique<juce::TextEditor>();
            row.text->setFont(juce::FontOptions(12.0f));
            row.text->setColour(juce::TextEditor::backgroundColourId, Palette::background);
            row.text->setColour(juce::TextEditor::textColourId, Palette::text);
            row.text->setColour(juce::TextEditor::outlineColourId, Palette::panelLight);
            row.text->setColour(juce::TextEditor::focusedOutlineColourId, Palette::accent);
            row.text->setText(juce::String::fromUTF8(textOf(d.name).c_str()),
                              juce::dontSendNotification);
            auto* e = row.text.get();
            std::string pname = d.name, cname = organism_;
            auto commit = [this, e, cname, pname] {
                const std::string v = e->getText().toStdString();
                if (auto* m = host_.model().byName(cname))
                    for (auto& pr : m->properties)
                        if (pr.name == pname && pr.text == v) return;
                host_.pushUndo();
                host_.setParamText(cname, pname, v);
            };
            e->onReturnKey = commit;
            e->onFocusLost = commit;
            addAndMakeVisible(*e);
        } else if (d.isBool) {
            row.toggle = std::make_unique<juce::ToggleButton>();
            row.toggle->setToggleState(valueOf(d.name, d.def) >= 0.5, juce::dontSendNotification);
            auto* t = row.toggle.get();
            std::string pname = d.name, cname = organism_;
            t->onClick = [this, t, cname, pname] {
                host_.pushUndo();
                host_.setParam(cname, pname, t->getToggleState() ? 1.0 : 0.0);
            };
            addAndMakeVisible(*t);
        } else {
            row.slider = std::make_unique<ParamSlider>();
            row.slider->getProperties().set("family", (int) familyOf(cm->displayClass));
            row.slider->paramLabel = d.name;
            row.slider->setRange(d.min, d.max, d.isEnum || d.isInt ? 1.0 : 0.0);
            row.slider->configureScaling();
            row.slider->setUnit(unitResolve(d.name, d.unit, d.min, d.max));
            row.slider->setValue(valueOf(d.name, d.def), juce::dontSendNotification);
            row.slider->setDoubleClickReturnValue(true, d.def);
            row.slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
            auto* s = row.slider.get();
            std::string pname = d.name, cname = organism_;
            s->onValueChange = [this, s, cname, pname] { host_.editParam(cname, pname, s->getValue()); };
            s->onDragStart = [this, cname, pname] { host_.beginParamDrag(cname, pname); };
            s->onDragEnd = [this] { host_.endParamDrag(); };
            s->setParamId(organism_, pname);
            s->onPopup = [this, pname](juce::Point<int> screen) { showParamMenu(pname, screen); };
            addAndMakeVisible(*s);
        }
        rows_.push_back(std::move(row));
    }
    resized();
    repaint();
}

void ParameterPanel::refreshAutomatedValues() {
    const bool pluginNode = host_.hostedPluginFor(organism_) != nullptr;
    for (auto& r : rows_) {
        if (!r.slider) continue;
        const bool tracked = host_.isLiveTracked(organism_, r.param);
        r.slider->setExternallyControlled(
            host_.isExternallyControlled(organism_, r.param));
        r.slider->setRollLocked(host_.rollLocked(organism_, r.param));
        if (!pluginNode && !tracked) continue;
        const double v = host_.liveParamValue(organism_, r.param);
        if (std::abs(v - r.slider->getValue()) > 1e-9) {
            r.slider->setValue(v, juce::dontSendNotification);
            ember::stamp(*r.slider);
        }
    }
}

void ParameterPanel::showParamMenu(const std::string& param, juce::Point<int> screenPos) {
    showAutomateMenu(host_, organism_, param, screenPos,
                     [this] { if (onAutomationChanged) onAutomationChanged(); });
}

int ParameterPanel::labelLines() const {
    int lines = 1;
    for (const auto& r : rows_) {
        if (r.text || !r.label) continue;
        juce::AttributedString as;
        as.setText(r.label->getText());
        as.setFont(r.label->getFont());
        juce::TextLayout tl;
        tl.createLayout(as, (float) (kCellW - 6));
        const float lh = r.label->getFont().getHeight();
        if (lh > 0.0f)
            lines = juce::jmax(lines, (int) std::lround(tl.getHeight() / lh));
    }
    return juce::jlimit(1, 3, lines);
}

int ParameterPanel::labelHeight() const { return 16 + (labelLines() - 1) * 13; }
int ParameterPanel::cellHeight() const { return 78 + labelHeight(); }

int ParameterPanel::preferredContentHeight(int width) const {
    int textRows = 0;
    for (const auto& r : rows_) textRows += r.text ? 1 : 0;
    const int cols = juce::jmax(1, (width - 16) / kCellW);
    const int knobs = (int) rows_.size() - textRows;
    const int rows = (knobs + cols - 1) / cols;
    return (wearsCollar() ? collar::kHeight : 0)
           + textRows * 48 + juce::jmax(textRows > 0 ? 0 : 1, rows) * cellHeight() + 12;
}

void ParameterPanel::resized() {
    const int cellW = kCellW, cellH = cellHeight(), labelH = labelHeight();
    auto area = getLocalBounds().reduced(8, 0);
    if (wearsCollar()) area.removeFromTop(collar::kHeight);
    for (auto& r : rows_) {
        if (!r.text) continue;
        auto band = area.removeFromTop(48).reduced(0, 2);
        r.label->setJustificationType(juce::Justification::centredLeft);
        r.label->setBounds(band.removeFromTop(labelH));
        r.text->setBounds(band.removeFromTop(24));
    }
    int cols = juce::jmax(1, area.getWidth() / cellW);
    int i = 0;
    for (auto& r : rows_) {
        if (r.text) continue;
        int col = i % cols, rowIdx = i / cols;
        juce::Rectangle<int> cell(area.getX() + col * cellW, area.getY() + rowIdx * cellH,
                                  cellW - 6, cellH - 8);
        r.label->setBounds(cell.removeFromTop(labelH));
        if (r.slider) r.slider->setBounds(cell);
        if (r.toggle) r.toggle->setBounds(cell.withSizeKeepingCentre(cell.getWidth(), 24));
        ++i;
    }
}

void ParameterPanel::paint(juce::Graphics& g) {
    if (organism_.empty()) {
        g.fillAll(Palette::panel);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(tr("parameter.no-selection", "(no selection)"), getLocalBounds().reduced(8).removeFromTop(24),
                   juce::Justification::centredLeft, true);
        return;
    }
    const auto* cm = host_.model().byName(organism_);
    collar::paintSoil(g, getLocalBounds().toFloat(),
                      cm ? familyOf(cm->displayClass) : Family::Utility);
    if (wearsCollar())
        collar::paint(g, host_, organism_, getLocalBounds().removeFromTop(collar::kHeight));
}

}
