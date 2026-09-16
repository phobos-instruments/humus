// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/ClassString.h"
#include "core/app/FuzzyMatch.h"
#include "gui/patcher/ClassPickerMenu.h"
#include "gui/style/Colours.h"
#include "gui/help/HelpBody.h"
#include "gui/help/HelpDocs.h"
#include "gui/help/HelpMarkdown.h"
#include "gui/help/HelpPageGeometry.h"
#include "gui/style/LookAndFeel.h"
#include "gui/editor/OrganismEditorFactory.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/common/Localisation.h"

namespace hum {

class EngineHost;

class HelpPage : public juce::Component {
public:
    HelpPage(juce::Image preview, juce::Image node, const HelpDoc& doc,
             const std::string& cls,
             std::function<void(const std::string&)> onOrganism = {})
        : preview_(preview), node_(node), body_(doc, cls, std::move(onOrganism)) {
        addAndMakeVisible(body_);
    }

    void layoutTo(int width) {
        const auto row = helpImageRow(preview_.getWidth(), preview_.getHeight(),
                                      node_.getWidth(), node_.getHeight(), width, kPad);
        imgArea_ = row.preview;
        nodeArea_ = row.node;
        body_.layoutTo(width);
        body_.setTopLeftPosition(0, row.bottom);
        setSize(width, row.bottom + body_.getHeight());
    }

    void paint(juce::Graphics& g) override {
        auto frame = [&](const juce::Image& img, const juce::Rectangle<float>& area) {
            if (!img.isValid() || area.isEmpty()) return;
            g.drawImage(img, area);
            g.setColour(Palette::border);
            g.drawRoundedRectangle(area.expanded(0.5f), 3.0f, 1.0f);
        };
        frame(preview_, imgArea_);
        frame(node_, nodeArea_);
    }

private:
    static constexpr int kPad = 12;
    juce::Image preview_, node_;
    HelpBody body_;
    juce::Rectangle<float> imgArea_, nodeArea_;
};

class HelpBrowser : public juce::Component, private juce::ListBoxModel {
public:
    HelpBrowser();
    ~HelpBrowser() override;

    void showClass(const std::string& wanted);

    std::vector<std::string> indexDisplaysForTest() const;

    void paint(juce::Graphics& g) override { g.fillAll(Palette::background); }

    void setSearchText(const juce::String& q);

    void resized() override;

private:
    struct Row {
        enum Kind { Header, Item } kind = Item;
        int level = 0;
        int id = -1;
        juce::String label;
        std::string display;
        juce::String tag;
        int score = 0;
    };

    void buildOutline();

    void collapseSharedPages();

    void appendVisibleOutline();

    void toggleHeader(const Row& h);

    void refilter();

    const juce::String& pageTextOf(const std::string& display);

    int firstItemRow() const;

    void render(const std::string& displayClass);

    juce::Image previewFor(const std::string& displayClass);

    juce::Image nodeFor(const std::string& displayClass);

    int getNumRows() override { return juce::jmax(1, (int) shown_.size()); }

    void selectedRowsChanged(int row) override;

    void listBoxItemClicked(int row, const juce::MouseEvent&) override;

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;

    juce::TextEditor search_;
    juce::ListBox list_;
    juce::Label heading_;
    juce::Viewport viewport_;
    std::unique_ptr<HelpPage> body_;
    std::unique_ptr<EngineHost> previewHost_;
    std::map<std::string, juce::Image> previews_;
    std::map<std::string, juce::Image> nodes_;
    std::vector<Row> outline_, shown_;
    std::set<int> collapsed_;
    std::map<std::string, juce::String> pageText_;
    std::map<std::string, std::string> rowOf_;
    bool searching_ = false;
};

}
