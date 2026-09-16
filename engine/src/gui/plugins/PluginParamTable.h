// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/plugins/HostedPlugin.h"
#include "gui/editor/AutomateMenu.h"
#include "gui/editor/OrganismEditor.h"
#include "gui/host/PluginsHost.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/ParamGrouping.h"
#include "gui/editor/ParamSlider.h"
#include "gui/plugins/PluginTreeGroups.h"
#include "gui/common/Localisation.h"

namespace hum {

class PluginParamTable : public OrganismEditor {
public:
    PluginParamTable(PluginsHost& host, const std::string& name)
        : host_(host), name_(name) {
        addAndMakeVisible(viewport_);
        viewport_.setViewedComponent(&rowContainer_, false);
        viewport_.setScrollBarsShown(true, false);
        viewport_.setScrollBarThickness(10);

        search_.setFont(juce::FontOptions(12.0f));
        search_.setTextToShowWhenEmpty(juce::String::fromUTF8("Search parameters\xe2\x80\xa6"),
                                       Palette::textDim);
        search_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
        search_.setColour(juce::TextEditor::textColourId, Palette::text);
        search_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        search_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
        search_.onTextChange = [this] {
            filter_ = search_.getText().toStdString();
            viewport_.setViewPosition(0, 0);
            resized();
            rowContainer_.repaint();
        };
        search_.onEscapeKey = [this] { search_.setText({}, juce::sendNotification); };
        addChildComponent(search_);

        foldAll_.onClick = [this] { setAllFolded(!allFolded()); };
        addChildComponent(foldAll_);

        buildRows();
        updateFoldAllButton();
    }

    void reloadValues() override {
        rowContainer_.removeAllChildren();
        rows_.clear();
        rowNames_.clear();
        sections_.clear();
        buildRows();
        updateFoldAllButton();
        resized();
        repaint();
    }

    void refreshAutomatedValues() override {
        auto* hp = host_.hostedPluginFor(name_);
        if (!hp) return;
        const auto& jp = hp->instance()->getParameters();
        for (auto& r : rows_) {
            if (r.juceIndex >= (int) jp.size()) continue;
            const float v = jp[(size_t) r.juceIndex]->getValue();
            r.slider->setValue(v, juce::dontSendNotification);
            r.valueLabel->setText(jp[(size_t) r.juceIndex]->getText(v, 32),
                                  juce::dontSendNotification);
        }
    }

    int preferredContentWidth() const override { return 380; }
    int preferredContentHeight(int) const override {
        return (toolbarVisible() ? kToolbarH : 0) + juce::jmin(contentHeight(), kMaxScrollH);
    }

    void resized() override {
        auto area = getLocalBounds();
        const bool tb = toolbarVisible();
        search_.setVisible(tb);
        foldAll_.setVisible(tb && anyTitled());
        if (tb) {
            auto bar = area.removeFromTop(kToolbarH);
            if (foldAll_.isVisible()) foldAll_.setBounds(bar.removeFromRight(72).reduced(2, 3));
            search_.setBounds(bar.reduced(2, 3));
        }
        viewport_.setBounds(area);
        rowContainer_.setSize(area.getWidth(), juce::jmax(1, contentHeight()));
        layoutRows();
    }

    void paint(juce::Graphics&) override {}

private:
    static constexpr int kRowH      = 30;
    static constexpr int kHeaderH   = 22;
    static constexpr int kToolbarH  = 26;
    static constexpr int kAutoFoldAt = 64;
    static constexpr int kMaxScrollH = 300;
    static constexpr int kLabelW    = 118;
    static constexpr int kSliderMaxW = 220;

    struct Row {
        int juceIndex = -1;
        int stripe = 0;
        std::unique_ptr<juce::Label>  label;
        std::unique_ptr<ParamSlider>  slider;
        std::unique_ptr<juce::Label>  valueLabel;
    };

    struct SectionHeader : juce::Component {
        std::string title;
        int count = 0;
        bool collapsed = false;
        std::function<void()> onToggle;
        void paint(juce::Graphics& g) override {
            g.fillAll(Palette::panel.withMultipliedBrightness(1.16f));
            juce::Path tri;
            const float cx = 11.0f, cy = getHeight() / 2.0f;
            if (collapsed) tri.addTriangle(cx - 2.5f, cy - 4.5f, cx - 2.5f, cy + 4.5f, cx + 4.5f, cy);
            else           tri.addTriangle(cx - 4.5f, cy - 2.5f, cx + 4.5f, cy - 2.5f, cx, cy + 4.5f);
            g.setColour(Palette::textDim);
            g.fillPath(tri);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
            auto t = juce::String::fromUTF8(title.c_str());
            if (collapsed) t << " (" << count << ")";
            g.drawText(t, 22, 0, getWidth() - 28, getHeight(), juce::Justification::centredLeft);
        }
        void mouseUp(const juce::MouseEvent& e) override {
            if (getLocalBounds().contains(e.getPosition()) && onToggle) onToggle();
        }
        void mouseEnter(const juce::MouseEvent&) override {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }
    };

    struct Section {
        std::string title;
        std::vector<int> rows;
        std::unique_ptr<SectionHeader> header;
    };

    struct RowContainer : juce::Component {
        std::vector<Row>* rows = nullptr;
        void paint(juce::Graphics& g) override {
            if (!rows) return;
            g.fillAll(Palette::panel);
            for (const auto& r : *rows) {
                if (!r.label->isVisible()) continue;
                g.setColour(r.stripe % 2 == 0
                                ? Palette::panel
                                : Palette::panel.withMultipliedBrightness(1.06f));
                g.fillRect(0, r.label->getY(), getWidth(), kRowH);
            }
        }
    };

    std::vector<SectionView> currentViews() const {
        std::vector<ParamSection> secs(sections_.size());
        std::vector<char> folded(sections_.size(), 0);
        for (size_t i = 0; i < sections_.size(); ++i) {
            secs[i].title = sections_[i].title;
            secs[i].rows = sections_[i].rows;
            folded[i] = sections_[i].header && sections_[i].header->collapsed;
        }
        return planVisibleRows(secs, rowNames_, folded, filter_);
    }

    int contentHeight() const {
        int h = 0;
        for (const auto& v : currentViews())
            h += (v.showHeader ? kHeaderH : 0) + (int) v.rows.size() * kRowH;
        return h;
    }

    void layoutRows() {
        const auto views = currentViews();
        for (auto& r : rows_) {
            r.label->setVisible(false);
            r.slider->setVisible(false);
            r.valueLabel->setVisible(false);
        }
        int y = 0;
        for (size_t si = 0; si < sections_.size(); ++si) {
            const SectionView& v = views[si];
            if (auto* h = sections_[si].header.get()) {
                h->setVisible(v.showHeader);
                if (v.showHeader) {
                    h->setBounds(0, y, rowContainer_.getWidth(), kHeaderH);
                    y += kHeaderH;
                }
            }
            int stripe = 0;
            for (int i : v.rows) {
                Row& r = rows_[(size_t) i];
                r.label->setVisible(true);
                r.slider->setVisible(true);
                r.valueLabel->setVisible(true);
                auto row = juce::Rectangle<int>(0, y, rowContainer_.getWidth(), kRowH);
                r.label->setBounds(row.removeFromLeft(kLabelW));
                const int sliderW = juce::jlimit(90, kSliderMaxW, row.getWidth() * 11 / 20);
                r.slider->setBounds(row.removeFromLeft(sliderW).reduced(0, 4));
                row.removeFromLeft(8);
                r.valueLabel->setBounds(row.withTrimmedRight(6));
                r.stripe = stripe++;
                y += kRowH;
            }
        }
    }

    bool anyTitled() const {
        for (const auto& s : sections_) if (s.header) return true;
        return false;
    }
    bool toolbarVisible() const { return rows_.size() >= 10 || anyTitled(); }

    bool allFolded() const {
        bool any = false;
        for (const auto& s : sections_) {
            if (!s.header) continue;
            any = true;
            if (!s.header->collapsed) return false;
        }
        return any;
    }

    void setAllFolded(bool f) {
        for (auto& s : sections_) {
            if (!s.header) continue;
            s.header->collapsed = f;
            collapsed_[s.title] = f;
        }
        updateFoldAllButton();
        resized();
        rowContainer_.repaint();
    }

    void updateFoldAllButton() {
        foldAll_.setButtonText(allFolded() ? tr("plugin-param-table.open-all", "Open all") : tr("plugin-param-table.fold-all", "Fold all"));
    }

    void buildRows() {
        rowContainer_.rows = &rows_;
        auto* hp = host_.hostedPluginFor(name_);
        if (!hp) return;
        const auto& ourParams = hp->params.all();
        const auto& jp        = hp->instance()->getParameters();
        const auto groups     = pluginTreeGroups(*hp->instance());
        std::vector<std::pair<std::string, std::string>> meta;

        std::vector<std::array<std::string, 3>> triples;
        std::vector<int> tripleIdx;
        for (const auto& p : ourParams) {
            if (p.index < 0 || p.index >= (int) jp.size()) continue;
            auto* hosted = hp->instance()->getHostedParameter(p.index);
            triples.push_back({hosted ? hosted->getParameterID().toStdString() : std::string(),
                               p.name,
                               jp[(size_t) p.index]->getName(64).trim().toStdString()});
            tripleIdx.push_back(p.index);
        }
        const auto keep = collapseAliasedRows(triples);
        std::vector<std::pair<std::string, int>> liveNames;
        for (size_t k = 0; k < triples.size(); ++k)
            if (keep[k]) liveNames.push_back({triples[k][2], tripleIdx[k]});
        const auto display = disambiguateNames(liveNames);

        size_t ord = 0;
        for (const auto& p : ourParams) {
            if (p.index < 0 || p.index >= (int) jp.size()) continue;
            if (!keep[ord++]) continue;
            auto* juceParam = jp[(size_t) p.index];
            const std::string& dispName = display[rows_.size()];
            const float curVal = juceParam->getValue();
            const std::string pname = p.name;
            const int juceIdx = p.index;

            Row r;
            r.juceIndex = juceIdx;

            r.label = std::make_unique<juce::Label>();
            r.label->setText(juce::String::fromUTF8(dispName.c_str()), juce::dontSendNotification);
            r.label->setFont(juce::FontOptions(12.0f));
            r.label->setColour(juce::Label::textColourId, Palette::text);
            r.label->setJustificationType(juce::Justification::centredLeft);
            r.label->setMinimumHorizontalScale(0.7f);
            rowContainer_.addAndMakeVisible(*r.label);

            r.slider = std::make_unique<ParamSlider>(juce::Slider::LinearHorizontal,
                                                     juce::Slider::NoTextBox);
            r.slider->paramLabel = juce::String::fromUTF8(dispName.c_str());
            r.slider->setRange(0.0, 1.0);
            r.slider->setValue(curVal, juce::dontSendNotification);
            r.slider->setDoubleClickReturnValue(true, (double) juceParam->getDefaultValue());

            auto* s  = r.slider.get();

            r.slider->onDragStart = [this, pname] { host_.beginParamDrag(name_, pname); };
            r.slider->onDragEnd   = [this]         { host_.endParamDrag(); };
            r.slider->onPopup = [this, pname](juce::Point<int> pt) {
                showAutomateMenu(host_, name_, pname, pt,
                                 [this] { if (onAutomationChanged) onAutomationChanged(); });
            };
            rowContainer_.addAndMakeVisible(*r.slider);

            r.valueLabel = std::make_unique<juce::Label>();
            r.valueLabel->setText(juceParam->getText(curVal, 32), juce::dontSendNotification);
            r.valueLabel->setFont(juce::FontOptions(12.0f));
            r.valueLabel->setColour(juce::Label::textColourId, Palette::text);
            r.valueLabel->setJustificationType(juce::Justification::centredLeft);
            r.valueLabel->setMinimumHorizontalScale(0.8f);
            rowContainer_.addAndMakeVisible(*r.valueLabel);

            auto* vl = r.valueLabel.get();
            s->onValueChange = [this, s, vl, pname, juceIdx] {
                host_.editParam(name_, pname, s->getValue());
                if (auto* hp2 = host_.hostedPluginFor(name_)) {
                    const auto& jp2 = hp2->instance()->getParameters();
                    if (juceIdx < (int) jp2.size())
                        vl->setText(jp2[(size_t) juceIdx]->getText((float) s->getValue(), 32),
                                    juce::dontSendNotification);
                }
            };

            const auto git = groups.find(juceParam);
            meta.push_back({dispName, git != groups.end() ? git->second : std::string()});
            rowNames_.push_back(meta.back().first);
            rows_.push_back(std::move(r));
        }

        for (auto& ps : planParamSections(meta)) {
            Section s;
            s.title = ps.title;
            s.rows = std::move(ps.rows);
            if (!s.title.empty()) {
                s.header = std::make_unique<SectionHeader>();
                s.header->title = s.title;
                s.header->count = (int) s.rows.size();
                const auto cit = collapsed_.find(s.title);
                s.header->collapsed = cit != collapsed_.end()
                                          ? cit->second
                                          : (int) s.rows.size() >= kAutoFoldAt;
                auto* h = s.header.get();
                s.header->onToggle = [this, h] {
                    collapsed_[h->title] = h->collapsed = !h->collapsed;
                    updateFoldAllButton();
                    resized();
                    rowContainer_.repaint();
                };
                rowContainer_.addAndMakeVisible(*s.header);
            }
            sections_.push_back(std::move(s));
        }
    }

    PluginsHost& host_;
    std::string name_;
    RowContainer rowContainer_;
    juce::Viewport viewport_;
    juce::TextEditor search_;
    juce::TextButton foldAll_;
    std::string filter_;
    std::vector<Row> rows_;
    std::vector<std::string> rowNames_;
    std::vector<Section> sections_;
    std::map<std::string, bool> collapsed_;
};

}
