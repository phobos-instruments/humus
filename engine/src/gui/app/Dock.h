// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/app/FreeWindow.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class DockHeaderButton : public juce::Button {
public:
    enum Kind { Detach, Close, Up, Down };
    explicit DockHeaderButton(Kind k) : juce::Button({}), kind_(k) {}
    void paintButton(juce::Graphics& g, bool over, bool) override {
        auto r = getLocalBounds().toFloat().reduced(5.0f);
        g.setColour(over && isEnabled() ? Palette::accent : Palette::textDim);
        if (kind_ == Close) {
            g.drawLine(r.getX(), r.getY(), r.getRight(), r.getBottom(), 1.4f);
            g.drawLine(r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.4f);
        } else if (kind_ == Up || kind_ == Down) {
            const float tipY = kind_ == Up ? r.getY() + r.getHeight() * 0.25f : r.getBottom() - r.getHeight() * 0.25f;
            const float baseY = kind_ == Up ? r.getBottom() - r.getHeight() * 0.25f : r.getY() + r.getHeight() * 0.25f;
            juce::Path chevron;
            chevron.startNewSubPath(r.getX(), baseY);
            chevron.lineTo(r.getCentreX(), tipY);
            chevron.lineTo(r.getRight(), baseY);
            g.strokePath(chevron, juce::PathStrokeType(1.6f));
        } else {
            g.drawRect(r.getX(), r.getCentreY(), r.getWidth() * 0.6f, r.getHeight() * 0.5f, 1.0f);
            g.drawLine(r.getCentreX(), r.getCentreY(), r.getRight(), r.getY(), 1.4f);
            g.drawLine(r.getRight() - r.getWidth() * 0.35f, r.getY(), r.getRight(), r.getY(), 1.4f);
            g.drawLine(r.getRight(), r.getY(), r.getRight(), r.getY() + r.getHeight() * 0.35f, 1.4f);
        }
    }
private:
    Kind kind_;
};

class DockPane : public juce::Component {
public:
    static constexpr int kHeader = 20;

    explicit DockPane(juce::String title) : title_(std::move(title)) {
        addAndMakeVisible(detachBtn_);
        addAndMakeVisible(closeBtn_);
        detachBtn_.onClick = [this] { if (onDetach) onDetach(); };
        closeBtn_.onClick = [this] { if (onClose) onClose(); };
        applyTexts();
    }

    void applyTexts() {
        detachBtn_.setTooltip(tr("dock.detach-into-a-floating-window", "Detach into a floating window"));
        closeBtn_.setTooltip(tr("dock.hide-this-pane", "Hide this pane"));
    }

    void setContent(juce::Component* c) {
        content_ = c;
        if (c) addAndMakeVisible(c);
        resized();
    }
    juce::Component* content() const { return content_; }
    const juce::String& title() const { return title_; }
    void setTitle(juce::String t) { title_ = std::move(t); repaint(); }

    std::function<void()> onDetach, onClose;

    void showButtons(bool detach, bool close) {
        detachBtn_.setVisible(detach);
        closeBtn_.setVisible(close);
    }

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::panelLight);
        g.fillRect(0, 0, getWidth(), kHeader);
        g.setColour(Palette::border);
        g.drawRect(getLocalBounds(), 1);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(title_, 6, 0, getWidth() - 2 * kHeader - 6, kHeader, juce::Justification::centredLeft);
    }
    void resized() override {
        auto a = getLocalBounds();
        auto h = a.removeFromTop(kHeader);
        closeBtn_.setBounds(h.removeFromRight(kHeader));
        detachBtn_.setBounds(h.removeFromRight(kHeader));
        if (content_) content_->setBounds(a.reduced(1));
    }

private:
    juce::String title_;
    juce::Component* content_ = nullptr;
    DockHeaderButton detachBtn_{DockHeaderButton::Detach}, closeBtn_{DockHeaderButton::Close};
};

class TabSwitcher : public juce::Component {
public:
    static constexpr int kTabH = 18;

    void addTab(const juce::String& name, juce::Component* content) {
        tabs_.push_back({name, content});
        addChildComponent(content);
        if ((int) tabs_.size() == 1) content->setVisible(true);
        resized();
    }
    void setCurrentTab(int index, juce::NotificationType n = juce::sendNotification) {
        if (tabs_.empty()) return;
        current_ = juce::jlimit(0, (int) tabs_.size() - 1, index);
        for (int i = 0; i < (int) tabs_.size(); ++i)
            tabs_[(size_t) i].content->setVisible(i == current_);
        resized();
        repaint();
        if (n != juce::dontSendNotification && onTabChanged) onTabChanged(current_);
    }
    int currentTab() const { return current_; }
    juce::String tabName(int i) const {
        return i >= 0 && i < (int) tabs_.size() ? tabs_[(size_t) i].name : juce::String();
    }

    std::function<void(int)> onTabChanged;

    void paint(juce::Graphics& g) override {
        g.setColour(Palette::panelLight);
        g.fillRect(0, 0, getWidth(), kTabH);
        g.setFont(juce::FontOptions(11.0f));
        for (int i = 0; i < (int) tabs_.size(); ++i) {
            const auto r = tabBounds(i);
            const bool sel = i == current_;
            g.setColour(sel ? Palette::panel : Palette::panelLight);
            g.fillRect(r);
            g.setColour(Palette::border);
            g.drawRect(r, 1);
            g.setColour(sel ? Palette::accent : Palette::textDim);
            g.drawText(tabs_[(size_t) i].name, r, juce::Justification::centred);
        }
        g.setColour(Palette::border);
        g.drawHorizontalLine(kTabH - 1, 0.0f, (float) getWidth());
    }
    void resized() override {
        const auto body = getLocalBounds().withTrimmedTop(kTabH);
        for (auto& t : tabs_) t.content->setBounds(body);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.getPosition().y >= kTabH) return;
        for (int i = 0; i < (int) tabs_.size(); ++i)
            if (tabBounds(i).contains(e.getPosition())) { setCurrentTab(i); return; }
    }

private:
    struct Tab { juce::String name; juce::Component* content; };
    juce::Rectangle<int> tabBounds(int i) const { return {i * kTabW, 0, kTabW, kTabH}; }
    static constexpr int kTabW = 92;
    std::vector<Tab> tabs_;
    int current_ = 0;
};

class SplitterBar : public juce::Component {
public:
    static constexpr int kThick = 10;

    enum Orient { Vertical, Horizontal };
    explicit SplitterBar(Orient o) : orient_(o) {
        setMouseCursor(o == Vertical ? juce::MouseCursor::LeftRightResizeCursor
                                     : juce::MouseCursor::UpDownResizeCursor);
    }
    std::function<void(int delta)> onDrag;
    std::function<void()> onDragEnd;
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        if (hot_) {
            g.setColour(Palette::accent.withAlpha(alpha::scrim));
            g.fillRoundedRectangle(r.reduced(1.5f), 3.0f);
            g.setColour(Palette::accent);
            const auto c = r.getCentre();
            for (int i = -1; i <= 1; ++i) {
                const float dx = orient_ == Vertical ? 0.0f : (float) i * 7.0f;
                const float dy = orient_ == Vertical ? (float) i * 7.0f : 0.0f;
                g.fillEllipse(c.x + dx - 1.25f, c.y + dy - 1.25f, 2.5f, 2.5f);
            }
            return;
        }
        g.setColour(Palette::border);
        if (orient_ == Vertical) g.fillRect(r.withSizeKeepingCentre(2.0f, r.getHeight()));
        else                     g.fillRect(r.withSizeKeepingCentre(r.getWidth(), 2.0f));
    }
    void mouseEnter(const juce::MouseEvent&) override { hot_ = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override {
        if (!isMouseButtonDown()) { hot_ = false; repaint(); }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        last_ = orient_ == Vertical ? e.getScreenX() : e.getScreenY();
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        int now = orient_ == Vertical ? e.getScreenX() : e.getScreenY();
        if (onDrag) onDrag(now - last_);
        last_ = now;
    }
    void mouseUp(const juce::MouseEvent&) override {
        if (onDragEnd) onDragEnd();
        if (!isMouseOver()) { hot_ = false; repaint(); }
    }
private:
    Orient orient_;
    int last_ = 0;
    bool hot_ = false;
};

class PaneHolder : public juce::Component {
public:
    void setParts(juce::Component* strip, int stripH, juce::Component* content) {
        strip_ = strip;
        stripH_ = stripH;
        content_ = content;
        if (strip_) addAndMakeVisible(*strip_);
        if (content_) addAndMakeVisible(*content_);
        resized();
    }
    void release() {
        if (strip_) removeChildComponent(strip_);
        if (content_) removeChildComponent(content_);
        strip_ = content_ = nullptr;
    }
    void resized() override {
        auto r = getLocalBounds();
        if (strip_) strip_->setBounds(r.removeFromTop(stripH_));
        if (content_) content_->setBounds(r);
    }
private:
    juce::Component* strip_ = nullptr;
    juce::Component* content_ = nullptr;
    int stripH_ = 0;
};

}
