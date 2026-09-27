// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <utility>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/TimedCard.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"

namespace hum {

class NoticeCard : public TimedCard {
public:
    enum class Kind { Passing, Sticky };
    static constexpr double kLingerSeconds = 3.0;
    static constexpr float kFontH = 13.0f;
    static constexpr int kOkH = 24;
    static constexpr int kTextInset = 13;
    static constexpr int kBarH = 3;
    static constexpr int kBarDrop = 7;

    std::function<void(const juce::String&)> toClipboard = [](const juce::String& text) {
        juce::SystemClipboard::copyTextToClipboard(text);
    };

    NoticeCard(const juce::String& text, Kind kind, const juce::String& details = {}) {
        copy_.setButtonText(tr("notice.copy", "Copy"));
        copy_.setTooltip(tr("notice.copy-tip", "Copy this message to paste it somewhere else"));
        copy_.onClick = [this] {
            toClipboard(copyText());
            copy_.setButtonText(tr("notice.copied", "Copied"));
        };
        details_ = details;
        set(text, kind);
    }

    void offerCopy(bool on) {
        copyable_ = on;
        set(text_, kind_);
        resized();
    }

    juce::String copyText() const {
        return details_.isEmpty() || details_.equalsIgnoreCase(text_) ? text_ : text_ + "\n\n" + details_;
    }
    juce::TextButton& copyButton() { return copy_; }

    static juce::String capitalised(const juce::String& text) {
        return text.substring(0, 1).toUpperCase() + text.substring(1);
    }

    void set(const juce::String& text, Kind kind) {
        text_ = capitalised(text);
        kind_ = kind;
        if (kind == Kind::Sticky && copyable_) addAndMakeVisible(copy_);
        else copy_.setVisible(false);
        const int textH = (int) std::ceil(textLayout().getHeight());
        const int buttonsH = copy_.isVisible() ? 2 * kOkH + kButtonGap : kOkH;
        setSize(kCardWidth, std::max(textH, buttonsH) + 2 * kTextInset);
        if (kind == Kind::Passing) expireIn(kLingerSeconds); else keep();
        repaint();
    }

    void setAction(const juce::String& label, std::function<void()> run) {
        action_.setButtonText(label);
        action_.onClick = [this, run = std::move(run)] {
            if (run) run();
            dismiss();
        };
        addAndMakeVisible(action_);
        set(text_, kind_);
        resized();
    }

    void setDismissLabel(const juce::String& label) {
        dismissButton().setButtonText(label);
        set(text_, kind_);
        resized();
    }

    juce::TextButton& actionButton() { return action_; }
    bool hasAction() const { return action_.isVisible(); }

    void setProgress(float done) {
        const float want = done < 0.0f ? -1.0f : std::min(1.0f, done);
        if (std::abs(want - progress_) < 0.005f) return;
        progress_ = want;
        repaint();
    }
    Kind kind() const { return kind_; }
    const juce::String& text() const { return text_; }

    void paint(juce::Graphics& g) override {
        paintBody(g, kind_ == Kind::Passing);
        const auto layout = textLayout();
        layout.draw(g, {(float) kPad, ((float) getHeight() - layout.getHeight()) * 0.5f,
                        (float) textWidth(), layout.getHeight()});
        paintProgress(g);
    }

    void layout() override {
        const int w = std::max(okWidth(), copy_.isVisible() ? buttonWidth(copy_, kOkH) : 0);
        const int stack = copy_.isVisible() ? 2 * kOkH + kButtonGap : kOkH;
        const int y = (getHeight() - stack) / 2;
        dismissButton().setBounds(getWidth() - (kPad - 2) - w, y, w, kOkH);
        if (copy_.isVisible()) copy_.setBounds(dismissButton().getX(), y + kOkH + kButtonGap, w, kOkH);
        if (action_.isVisible()) {
            const int a = buttonWidth(action_, kOkH);
            action_.setBounds(dismissButton().getX() - kButtonGap - a, y, a, kOkH);
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (kind_ == Kind::Passing && e.eventComponent == this) dismiss();
    }

private:
    void paintProgress(juce::Graphics& g) const {
        if (progress_ < 0.0f) return;
        const juce::Rectangle<float> track((float) kPad, (float) (getHeight() - kBarDrop),
                                           (float) (getWidth() - 2 * kPad), (float) kBarH);
        g.setColour(Palette::border);
        g.fillRoundedRectangle(track, (float) kBarH * 0.5f);
        if (progress_ <= 0.0f) return;
        g.setColour(Palette::accent);
        g.fillRoundedRectangle(track.withWidth(std::max((float) kBarH, track.getWidth() * progress_)),
                               (float) kBarH * 0.5f);
    }

    int okWidth() { return buttonWidth(dismissButton(), kOkH); }
    int buttonsWidth() {
        const int ok = std::max(okWidth(), copy_.isVisible() ? buttonWidth(copy_, kOkH) : 0) + kButtonGap;
        return action_.isVisible() ? ok + buttonWidth(action_, kOkH) + kButtonGap : ok;
    }
    int textWidth() const { return kCardWidth - 2 * kPad - const_cast<NoticeCard*>(this)->buttonsWidth(); }

    juce::TextLayout textLayout() const {
        juce::AttributedString s;
        s.setWordWrap(juce::AttributedString::byWord);
        s.append(text_, juce::FontOptions(kFontH), Palette::text);
        juce::TextLayout layout;
        layout.createLayout(s, (float) textWidth());
        return layout;
    }

    juce::TextButton action_, copy_;
    juce::String text_, details_;
    bool copyable_ = false;
    Kind kind_ = Kind::Passing;
    float progress_ = -1.0f;
};

class NoticeDesk {
public:
    std::function<void(std::unique_ptr<juce::Component>)> present;

    void say(const juce::String& text, NoticeCard::Kind kind = NoticeCard::Kind::Passing,
             const juce::String& key = {}, const juce::String& details = {}) {
        post(text, kind, key, details, false);
    }

    void warn(const juce::String& text, const juce::String& key = {}, const juce::String& details = {}) {
        post(text, NoticeCard::Kind::Sticky, key, details, true);
    }

    NoticeCard* cardFor(const juce::String& key) {
        const auto it = keyed_.find(key);
        if (it == keyed_.end() || it->second == nullptr || it->second->dismissed()) return nullptr;
        return it->second.getComponent();
    }

private:
    void post(const juce::String& text, NoticeCard::Kind kind, const juce::String& key, const juce::String& details,
              bool copyable) {
        if (key.isEmpty()) {
            if (text.isEmpty() || !present) return;
            auto card = std::make_unique<NoticeCard>(text, kind, details);
            card->offerCopy(copyable);
            present(std::move(card));
            return;
        }
        auto& slot = keyed_[key];
        if (slot != nullptr && slot->dismissed()) slot = nullptr;
        if (text.isEmpty()) {
            if (slot != nullptr) slot->dismiss();
            slot = nullptr;
            return;
        }
        if (slot != nullptr) {
            slot->set(text, kind);
            slot->offerCopy(copyable);
            return;
        }
        auto card = std::make_unique<NoticeCard>(text, kind, details);
        card->offerCopy(copyable);
        slot = card.get();
        if (present) present(std::move(card));
    }

    std::map<juce::String, juce::Component::SafePointer<NoticeCard>> keyed_;
};

}
