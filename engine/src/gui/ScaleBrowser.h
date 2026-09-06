#pragma once
#include <functional>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/FuzzyMatch.h"
#include "core/ScaleLibrary.h"
#include "gui/EngineHost.h"
#include "gui/LookAndFeel.h"
#include "gui/TextHighlight.h"
#include "gui/Localisation.h"

namespace hum {

class ScaleBrowser : public juce::Component, private juce::ListBoxModel {
public:
    std::function<void(const std::string& path)> onPick;

    ScaleBrowser() : entries_(library()) {
        search_.onNav = [this](const juce::KeyPress& k) { return navigate(k); };
        search_.setFont(juce::FontOptions(14.5f));
        search_.setTextToShowWhenEmpty(
            juce::String::fromUTF8("Search scales - name, region, tradition\xe2\x80\xa6"),
            Palette::textDim);
        search_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
        search_.setColour(juce::TextEditor::textColourId, Palette::text);
        search_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        search_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
        search_.onTextChange = [this] { refilter(); };
        search_.onEscapeKey = [this] { dismiss(); };
        search_.onReturnKey = [this] { pick(list_.getSelectedRow()); };
        addAndMakeVisible(search_);

        list_.setModel(this);
        list_.setRowHeight(kRowH);
        list_.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list_.setWantsKeyboardFocus(false);
        addAndMakeVisible(list_);

        refilter();
    }

    static void show(juce::Rectangle<int> screenAnchor,
                     std::function<void(const std::string&)> onPick) {
        auto content = std::make_unique<ScaleBrowser>();
        content->onPick = std::move(onPick);
        juce::CallOutBox::launchAsynchronously(std::move(content), screenAnchor, nullptr);
    }

    static const std::vector<ScaleInfo>& library() {
        static std::vector<ScaleInfo> cache;
        static juce::int64 stamp = -1;
        juce::int64 now = 0;
        for (const auto& r : scaleLibraryRoots())
            now += r.getLastModificationTime().toMilliseconds();
        if (now != stamp) {
            cache = scanScaleDirs(scaleLibraryRoots());
            stamp = now;
        }
        return cache;
    }

    void resized() override {
        auto r = getLocalBounds().reduced(8);
        search_.setBounds(r.removeFromTop(28));
        r.removeFromTop(6);
        list_.setBounds(r);
    }

    void parentHierarchyChanged() override {
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<ScaleBrowser>(this)] {
            if (safe != nullptr) safe->search_.grabKeyboardFocus();
        });
    }

private:
    static constexpr int kRowH = 36;
    static constexpr int kMaxRows = 12;

    struct Row {
        const ScaleInfo* info = nullptr;
        int score = 0;
    };

    struct SearchBox : juce::TextEditor {
        std::function<bool(const juce::KeyPress&)> onNav;
        bool keyPressed(const juce::KeyPress& k) override {
            if (onNav && onNav(k)) return true;
            return juce::TextEditor::keyPressed(k);
        }
    };

    bool navigate(const juce::KeyPress& k) {
        const int n = (int) shown_.size();
        if (n == 0) return false;
        int row = list_.getSelectedRow();
        if (k.getKeyCode() == juce::KeyPress::downKey) row = (row + 1) % n;
        else if (k.getKeyCode() == juce::KeyPress::upKey) row = (row + n - 1) % n;
        else return false;
        list_.selectRow(row);
        return true;
    }

    void refilter() {
        const auto q = search_.getText().toStdString();
        shown_.clear();
        for (const auto& e : entries_) {
            const int score =
                fuzzyScoreNameThenMeta(q, e.name, e.collection + " " + e.description);
            if (score < 0) continue;
            shown_.push_back({&e, score});
        }
        std::stable_sort(shown_.begin(), shown_.end(), [](const Row& a, const Row& b) {
            if (a.score != b.score) return a.score > b.score;
            if (a.info->collection != b.info->collection)
                return a.info->collection < b.info->collection;
            return a.info->name < b.info->name;
        });
        list_.updateContent();
        list_.selectRow(0);
        const int rows = juce::jlimit(1, kMaxRows, (int) shown_.size());
        setSize(460, 8 + 28 + 6 + rows * kRowH + 8);
        list_.repaint();
    }

    void pick(int row) {
        if (row < 0 || row >= (int) shown_.size()) return;
        const auto path = shown_[(size_t) row].info->path;
        dismiss();
        if (onPick) onPick(path);
    }

    void dismiss() {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
    }

    int getNumRows() override { return std::max(1, (int) shown_.size()); }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
        if (shown_.empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            const bool empty = entries_.empty();
            g.drawText(empty ? juce::String::fromUTF8(
                           "no scales installed - put .scl files in the scales folder")
                             : juce::String("no matches - Esc to close"),
                       10, 0, w - 20, h, juce::Justification::centredLeft);
            return;
        }
        const auto& e = *shown_[(size_t) row].info;
        if (selected) {
            g.setColour(Palette::accent.withAlpha(0.22f));
            g.fillRoundedRectangle(2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
        }
        const int half = h / 2;

        juce::String badge;
        if (e.degrees > 0) badge = juce::String(e.degrees) + juce::String::fromUTF8("\xc2\xb0");
        if (e.periodCents > 0.0 && std::abs(e.periodCents - 1200.0) > 1.0)
            badge += (std::abs(e.periodCents - 1901.955) < 2.0)
                         ? juce::String::fromUTF8(" \xc2\xb7 3:1")
                         : juce::String::fromUTF8(" \xc2\xb7 ") + juce::String(e.periodCents, 0) + "c";
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(Palette::textDim);
        const int badgeW = 88;
        g.drawText(badge, w - badgeW - 8, 0, badgeW, half + 4, juce::Justification::centredRight);

        drawMatchLit(g, juce::FontOptions(13.0f), juce::String::fromUTF8(e.name.c_str()),
                     search_.getText(), {10, 0, w - badgeW - 26, half + 4},
                     Palette::text, Palette::accent);

        g.setFont(juce::FontOptions(10.5f));
        g.setColour(Palette::textDim);
        juce::String sub = juce::String::fromUTF8(e.collection.c_str());
        if (sub.isNotEmpty() && !e.description.empty())
            sub += juce::String::fromUTF8(" \xe2\x80\xba ");
        sub += juce::String::fromUTF8(e.description.c_str());
        g.drawText(sub, 10, half, w - 20, h - half - 2, juce::Justification::centredLeft);
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override { pick(row); }
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override { pick(row); }

    SearchBox search_;
    juce::ListBox list_;
    const std::vector<ScaleInfo>& entries_;
    std::vector<Row> shown_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleBrowser)
};

class ScaleFileSlot : public juce::Component {
public:
    ScaleFileSlot(EngineHost& host, std::string organism, std::string param)
        : host_(host), name_(std::move(organism)), param_(std::move(param)) {
        pathLabel_.setColour(juce::Label::backgroundColourId, Palette::panel.darker(0.3f));
        pathLabel_.setColour(juce::Label::textColourId, Palette::text);
        pathLabel_.setFont(juce::FontOptions(11.0f));
        pathLabel_.setJustificationType(juce::Justification::centredLeft);
        pathLabel_.setMinimumHorizontalScale(0.7f);
        addAndMakeVisible(pathLabel_);

        browseBtn_.setButtonText(juce::String::fromUTF8("Browse\xe2\x80\xa6"));
        browseBtn_.onClick = [this] { browse(); };
        addAndMakeVisible(browseBtn_);

        clearBtn_.setButtonText(juce::String::charToString(juce::juce_wchar(0x00d7)));
        clearBtn_.setTooltip(tr("scale-browser.clear-this-scale", "Clear this scale"));
        clearBtn_.onClick = [this] {
            host_.setParamText(name_, param_, "");
            refresh();
        };
        addChildComponent(clearBtn_);
        refresh();
    }

    void refresh() {
        juce::String s(juce::CharPointer_UTF8(host_.liveParamText(name_, param_).c_str()));
        if (s.startsWith("file://")) s = s.substring(7);
        pathLabel_.setText(s.isEmpty() ? tr("scale-browser.no-scale", "(no scale)") : juce::File(s).getFileNameWithoutExtension(),
                           juce::dontSendNotification);
        pathLabel_.setTooltip(s);
        const bool loaded = s.isNotEmpty();
        if (loaded != clearBtn_.isVisible()) {
            clearBtn_.setVisible(loaded);
            resized();
        }
    }

    void resized() override {
        auto r = getLocalBounds();
        if (clearBtn_.isVisible()) {
            clearBtn_.setBounds(r.removeFromRight(r.getHeight()));
            r.removeFromRight(2);
        }
        browseBtn_.setBounds(r.removeFromRight(84));
        r.removeFromRight(4);
        pathLabel_.setBounds(r);
    }

private:
    void browse() {
        ScaleBrowser::show(
            browseBtn_.getScreenBounds(),
            [this, safe = juce::Component::SafePointer<ScaleFileSlot>(this)](const std::string& path) {
                if (safe == nullptr) return;
                host_.setParamText(name_, param_, path);
                host_.editParam(name_, "Preset", 11.0);
                refresh();
            });
    }

    EngineHost& host_;
    std::string name_, param_;
    juce::Label pathLabel_;
    juce::TextButton browseBtn_;
    juce::TextButton clearBtn_;
};

}
