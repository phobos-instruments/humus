// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cmath>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/packs/PackRegistry.h"
#include "core/app/PickerModel.h"
#include "gui/patcher/ClassPickerMenu.h"
#include "gui/style/Colours.h"
#include "gui/help/HelpDocs.h"
#include "gui/help/HelpMarkdown.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"
#include "gui/patcher/OrganismBlurb.h"
#include "gui/bricks/RootCollar.h"

namespace hum {

class ModernPicker : public juce::Component, private juce::Timer {
public:
    std::function<void(const std::string& cls)> onPick;

    ModernPicker();

    ~ModernPicker() override { stopTimer(); }

    bool keyPressed(const juce::KeyPress& k) override;

    int selectedForTest() const { return selected_; }
    void activateForTest(int i) { activate(i); }
    bool keyForTest(const juce::KeyPress& k) { return keyPressed(k); }
    float marqueeForTest(int ticks);
    bool selectedOverflowsForTest() const { return overflowPx_ > 0.0f; }

    int cellCountForTest() const { return (int) cells_.size(); }
    juce::Rectangle<int> cellBoundsForTest(int i) const { return cells_[(size_t) i].bounds; }
    bool cellIsFolderForTest(int i) const { return cells_[(size_t) i].folder; }
    std::string cellClassForTest(int i) const { return cells_[(size_t) i].l.cls; }
    void navigateForTest(picker::Path p) { navigateTo(std::move(p)); }
    void searchForTest(const juce::String& q) { search_.setText(q, false); rebuild(); }
    static juce::String descriptionForTest(const std::string& cls) { return descriptionFor(cls); }
    bool cardHandForTest(juce::Point<int> p) { return hoverHand(grid_, p); }
    bool crumbHandForTest(juce::Point<int> p) { return hoverHand(*this, p); }
    juce::Rectangle<int> crumbBoundsForTest(int i) const { return crumbRects_[(size_t) i]; }
    int crumbCountForTest() const { return (int) crumbRects_.size(); }

    static void show(juce::Rectangle<int> screenAnchor, std::function<void(const std::string&)> onPick);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseMove(const juce::MouseEvent& e) override;

    void mouseExit(const juce::MouseEvent&) override;

    void parentHierarchyChanged() override;

private:
    static constexpr int kCols = 1;
    static constexpr int kCardH = 50;
    static constexpr int kGap = 4;

    struct Cell {
        bool folder = false;
        picker::Folder f;
        picker::Leaf l;
        juce::Rectangle<int> bounds;
    };

    struct SearchBox : juce::TextEditor {
        std::function<bool(const juce::KeyPress&)> onKey;
        bool keyPressed(const juce::KeyPress& k) override {
            if (onKey && onKey(k)) return true;
            return juce::TextEditor::keyPressed(k);
        }
    };

    struct Grid : juce::Component {
        ModernPicker* owner = nullptr;
        void paint(juce::Graphics& g) override {
            for (size_t i = 0; i < owner->cells_.size(); ++i) owner->paintCard(g, i);
            if (owner->cells_.empty()) {
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(12.5f));
                g.drawText(juce::String(tr("modern-picker.no-matches-esc-to-close", "no matches - Esc to close")),
                           getLocalBounds().withHeight(40), juce::Justification::centred);
            }
        }
        void mouseDown(const juce::MouseEvent& e) override {
            owner->search_.grabKeyboardFocus();
            const int i = owner->cellAt(e.getPosition());
            if (i >= 0) owner->activate(i);
        }
        void mouseMove(const juce::MouseEvent& e) override {
            setMouseCursor(owner->cellAt(e.getPosition()) >= 0
                               ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
        }
        void mouseExit(const juce::MouseEvent&) override {
            setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    };

    static bool hoverHand(juce::Component& c, juce::Point<int> p);

    int cellAt(juce::Point<int> p) const;

    int crumbAt(juce::Point<int> p) const;

    static std::string pathKey(const picker::Path& p);

    void select(int i);

    void timerCallback() override;

    void rebuild();

    void layoutCards();

    void paintCard(juce::Graphics& g, size_t i);

    static void paintTile(juce::Graphics& g, const picker::Leaf& leaf, juce::Rectangle<int> tile);

    static juce::String descriptionFor(const std::string& cls) { return organismBlurb(cls); }

    void updateSearchPlaceholder();

    void navigateTo(picker::Path p);

    void activate(int i);

    bool handleKey(const juce::KeyPress& k);

    void showSelected();

    void dismiss();

    static constexpr double kMarqueeDelayMs = 1500.0;
    static constexpr float kMarqueeStep = 0.8f;
    static constexpr float kMarqueeRest = 24.0f;

    picker::Groups groups_;
    picker::Path path_;
    std::vector<Cell> cells_;
    int selected_ = -1;
    std::map<std::string, int> remembered_;
    double selectedSinceMs_ = 0.0;
    float marqueeOffset_ = 0.0f;
    float overflowPx_ = 0.0f;

    juce::Rectangle<int> crumbArea_;
    std::vector<juce::Rectangle<int>> crumbRects_;
    SearchBox search_;
    juce::Viewport view_;
    Grid grid_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernPicker)
};

}
