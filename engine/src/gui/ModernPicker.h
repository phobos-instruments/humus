#pragma once
#include <cmath>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/PickerModel.h"
#include "gui/ClassPickerMenu.h"
#include "gui/HelpDocs.h"
#include "gui/HelpMarkdown.h"
#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

namespace hum {

class ModernPicker : public juce::Component {
public:
    std::function<void(const std::string& cls)> onPick;

    ModernPicker() {
        groups_ = classPickerGroups();

        search_.onKey = [this](const juce::KeyPress& k) { return handleKey(k); };
        search_.setFont(juce::FontOptions(14.0f));
        search_.setColour(juce::TextEditor::backgroundColourId, Palette::panelLight);
        search_.setColour(juce::TextEditor::textColourId, Palette::text);
        search_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        search_.setColour(juce::TextEditor::focusedOutlineColourId, Palette::accentDim);
        search_.onTextChange = [this] { rebuild(); };
        search_.onEscapeKey = [this] { dismiss(); };
        search_.onReturnKey = [this] { activate(selected_); };
        addAndMakeVisible(search_);

        grid_.owner = this;
        view_.setViewedComponent(&grid_, false);
        view_.setScrollBarsShown(true, false);
        addAndMakeVisible(view_);

        setSize(560, 430);
        rebuild();
    }

    static void show(juce::Rectangle<int> screenAnchor,
                     std::function<void(const std::string&)> onPick) {
        auto content = std::make_unique<ModernPicker>();
        content->onPick = std::move(onPick);
        juce::CallOutBox::launchAsynchronously(std::move(content), screenAnchor, nullptr);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(10);
        crumbArea_ = r.removeFromTop(24);
        r.removeFromTop(6);
        search_.setBounds(r.removeFromTop(28));
        r.removeFromTop(6);
        view_.setBounds(r);
        layoutCards();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::background);
        crumbRects_.clear();
        const auto trail = picker::breadcrumb(path_);
        int x = crumbArea_.getX();
        const juce::Font pillFont = juce::FontOptions(12.0f);
        g.setFont(pillFont);
        for (size_t i = 0; i < trail.size(); ++i) {
            const auto label = juce::String::fromUTF8(trail[i].c_str());
            const int tw = (int) std::ceil(juce::GlyphArrangement::getStringWidth(pillFont, label));
            const juce::Rectangle<int> pill(x, crumbArea_.getY(), tw + 16, crumbArea_.getHeight());
            const bool last = i + 1 == trail.size();
            g.setColour(last ? Palette::accent.withAlpha(0.25f) : Palette::panelLight);
            g.fillRoundedRectangle(pill.toFloat(), 10.0f);
            g.setColour(last ? Palette::text : Palette::textDim);
            g.drawText(label, pill, juce::Justification::centred);
            crumbRects_.push_back(pill);
            x = pill.getRight() + 4;
            if (!last) {
                g.setColour(Palette::textDim);
                g.drawText(juce::String::fromUTF8("\xe2\x80\xba"), x, crumbArea_.getY(), 10,
                           crumbArea_.getHeight(), juce::Justification::centred);
                x += 14;
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        for (size_t i = 0; i < crumbRects_.size(); ++i)
            if (crumbRects_[i].contains(e.getPosition())) {
                navigateTo(picker::Path(path_.begin(), path_.begin() + (long) i));
                return;
            }
    }

    void parentHierarchyChanged() override {
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<ModernPicker>(this)] {
            if (safe != nullptr) safe->search_.grabKeyboardFocus();
        });
    }

private:
    static constexpr int kCols = 3;
    static constexpr int kCardH = 56;
    static constexpr int kGap = 6;

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
            for (size_t i = 0; i < owner->cells_.size(); ++i)
                if (owner->cells_[i].bounds.contains(e.getPosition())) {
                    owner->activate((int) i);
                    return;
                }
        }
    };

    void rebuild() {
        cells_.clear();
        const auto query = search_.getText().toStdString();
        if (query.empty()) {
            const auto level = picker::childrenAt(groups_, path_);
            for (const auto& f : level.folders) {
                Cell c;
                c.folder = true;
                c.f = f;
                cells_.push_back(std::move(c));
            }
            for (const auto& l : level.leaves) {
                Cell c;
                c.l = l;
                cells_.push_back(std::move(c));
            }
        } else {
            for (auto& l : picker::searchScoped(groups_, path_, query)) {
                Cell c;
                c.l = std::move(l);
                cells_.push_back(std::move(c));
            }
        }
        selected_ = cells_.empty() ? -1 : 0;
        updateSearchPlaceholder();
        layoutCards();
        grid_.repaint();
        repaint();
    }

    void layoutCards() {
        const int w = view_.getWidth() - view_.getScrollBarThickness() - 2;
        if (w <= 0) return;
        const int cardW = (w - (kCols - 1) * kGap) / kCols;
        int x = 0, y = 0, col = 0;
        for (auto& c : cells_) {
            c.bounds = {x, y, cardW, kCardH};
            if (++col == kCols) {
                col = 0;
                x = 0;
                y += kCardH + kGap;
            } else {
                x += cardW + kGap;
            }
        }
        const int rows = ((int) cells_.size() + kCols - 1) / kCols;
        grid_.setSize(w, juce::jmax(view_.getHeight(), rows * (kCardH + kGap)));
    }

    void paintCard(juce::Graphics& g, size_t i) {
        const auto& c = cells_[i];
        const auto r = c.bounds.toFloat();
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour((int) i == selected_ ? Palette::accent : Palette::border);
        g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, (int) i == selected_ ? 1.6f : 1.0f);

        auto inner = c.bounds.reduced(8, 6);
        const auto tile = inner.removeFromLeft(inner.getHeight()).toFloat();
        inner.removeFromLeft(8);
        if (c.folder) {
            g.setColour(Palette::panelLight);
            const auto fr = tile.reduced(4.0f);
            g.fillRoundedRectangle(fr.withTrimmedTop(fr.getHeight() * 0.22f), 3.0f);
            g.fillRoundedRectangle(fr.withWidth(fr.getWidth() * 0.45f)
                                       .withHeight(fr.getHeight() * 0.34f), 3.0f);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
            g.drawText(juce::String::fromUTF8(c.f.label.c_str()),
                       inner.removeFromTop(inner.getHeight() / 2), juce::Justification::bottomLeft);
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(juce::String(c.f.classCount)
                           + (c.f.classCount == 1 ? " organism" : " organisms"),
                       inner, juce::Justification::topLeft);
            return;
        }
        const int tint = picker::tintIndex(c.l.cls, 8);
        g.setColour(Palette::accent.withRotatedHue(((float) tint - 3.5f) * 0.03f).withAlpha(0.30f));
        g.fillRoundedRectangle(tile.reduced(2.0f), 5.0f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8(picker::monogram(c.l.display).c_str()),
                   tile.toNearestInt(), juce::Justification::centred);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8(c.l.display.c_str()),
                   inner.removeFromTop(inner.getHeight() / 2), juce::Justification::bottomLeft);
        const auto desc = descriptionFor(c.l.display);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(desc.isNotEmpty() ? desc : juce::String::fromUTF8(c.l.tag.c_str()),
                   inner, juce::Justification::topLeft);
    }

    static juce::String descriptionFor(const std::string& display) {
        static std::map<std::string, juce::String> cache;
        auto it = cache.find(display);
        if (it != cache.end()) return it->second;
        juce::String out;
        const auto doc = loadOrganismDoc(display);
        if (doc.text.isNotEmpty()) {
            const auto blocks = help_detail::parseHelpDoc(doc, juce::String(display)).second;
            for (const auto& b : blocks)
                if (b.kind == help_detail::Block::Para && b.a.isNotEmpty()) {
                    out = b.a;
                    break;
                }
            if (out.length() > 90)
                out = out.substring(0, 90).upToLastOccurrenceOf(" ", false, false)
                    + juce::String::fromUTF8("\xe2\x80\xa6");
        }
        cache[display] = out;
        return out;
    }

    void updateSearchPlaceholder() {
        const auto scope = path_.empty()
            ? juce::String("organisms")
            : juce::String::fromUTF8(path_.back().c_str());
        search_.setTextToShowWhenEmpty("Search " + (path_.empty() ? scope : "in " + scope)
                                           + juce::String::fromUTF8("\xe2\x80\xa6"),
                                       Palette::textDim);
        search_.repaint();
    }

    void navigateTo(picker::Path p) {
        path_ = std::move(p);
        search_.setText({}, juce::dontSendNotification);
        rebuild();
        view_.setViewPosition(0, 0);
    }

    void activate(int i) {
        if (i < 0 || i >= (int) cells_.size()) return;
        const auto& c = cells_[(size_t) i];
        if (c.folder) {
            auto p = path_;
            p.push_back(c.f.label);
            navigateTo(std::move(p));
            return;
        }
        const auto cls = c.l.cls;
        dismiss();
        if (onPick) onPick(cls);
    }

    bool handleKey(const juce::KeyPress& k) {
        const int n = (int) cells_.size();
        const int code = k.getKeyCode();
        if (code == juce::KeyPress::backspaceKey && search_.isEmpty()) {
            if (!path_.empty()) navigateTo(picker::parentOf(path_));
            return true;
        }
        if (n == 0) return false;
        int next = selected_;
        if (code == juce::KeyPress::rightKey) next = juce::jmin(n - 1, selected_ + 1);
        else if (code == juce::KeyPress::leftKey) next = juce::jmax(0, selected_ - 1);
        else if (code == juce::KeyPress::downKey) next = juce::jmin(n - 1, selected_ + kCols);
        else if (code == juce::KeyPress::upKey) next = juce::jmax(0, selected_ - kCols);
        else return false;
        if ((code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey)
            && !search_.isEmpty())
            return false;
        selected_ = next;
        showSelected();
        grid_.repaint();
        return true;
    }

    void showSelected() {
        if (selected_ < 0 || selected_ >= (int) cells_.size()) return;
        const auto b = cells_[(size_t) selected_].bounds;
        auto pos = view_.getViewPosition();
        if (b.getBottom() > pos.y + view_.getHeight()) pos.y = b.getBottom() - view_.getHeight();
        if (b.getY() < pos.y) pos.y = b.getY();
        view_.setViewPosition(pos);
    }

    void dismiss() {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
    }

    picker::Groups groups_;
    picker::Path path_;
    std::vector<Cell> cells_;
    int selected_ = -1;

    juce::Rectangle<int> crumbArea_;
    std::vector<juce::Rectangle<int>> crumbRects_;
    SearchBox search_;
    juce::Viewport view_;
    Grid grid_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernPicker)
};

}
