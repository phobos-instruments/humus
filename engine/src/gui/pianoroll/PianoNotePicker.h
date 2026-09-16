// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/style/Colours.h"
#include "gui/host/BrickHost.h"
#include "gui/style/LookAndFeel.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class PianoNotePicker : public juce::Component {
public:
    PianoNotePicker(int current, int lo, int hi, std::function<void(int)> onPick)
        : current_(current), lo_(lo), hi_(hi), onPick_(std::move(onPick)) {
        vLo_ = juce::jmax(lo_, current_ - kSpan / 2);
        vHi_ = juce::jmin(hi_, vLo_ + kSpan);
        vLo_ = juce::jmax(lo_, vHi_ - kSpan);
        layout();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::panel);
        auto strip = getLocalBounds().removeFromBottom(kLabelH);
        const int shown = hover_ >= 0 ? hover_ : current_;
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
        g.drawText(noteName(shown), strip, juce::Justification::centred);
        if (scrollable()) {
            g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
            g.setColour(vLo_ > lo_ ? Palette::text : Palette::border);
            g.drawText(juce::String::fromUTF8("\xe2\x80\xb9"),
                       strip.removeFromLeft(kArrowW), juce::Justification::centred);
            g.setColour(vHi_ < hi_ ? Palette::text : Palette::border);
            g.drawText(juce::String::fromUTF8("\xe2\x80\xba"),
                       strip.removeFromRight(kArrowW), juce::Justification::centred);
        }

        int x = 0;
        for (int n = vLo_; n <= vHi_; ++n) {
            if (!isWhite(n)) continue;
            const auto r = juce::Rectangle<float>((float) x, 0.0f, (float) kWhiteW, (float) kHeight);
            g.setColour(n == current_ ? Palette::accent
                        : n == hover_ ? Palette::accent.withAlpha(alpha::muted)
                                      : ink::keyboard::whiteKey);
            g.fillRect(r.reduced(0.5f));
            g.setColour(Palette::border);
            g.drawRect(r, 1.0f);
            if (n % 12 == 0) {
                g.setColour(n == current_ ? Palette::background : Palette::textDim);
                g.setFont(juce::FontOptions(8.5f));
                g.drawText("C" + juce::String(n / 12 - 1),
                           r.withTrimmedTop(kHeight - 13.0f).reduced(1.0f),
                           juce::Justification::centred);
            }
            x += kWhiteW;
        }
        x = 0;
        for (int n = vLo_; n <= vHi_; ++n) {
            if (!isWhite(n)) continue;
            if (n + 1 <= vHi_ && !isWhite(n + 1)) {
                const auto r = juce::Rectangle<float>((float) (x + kWhiteW - kBlackW / 2), 0.0f,
                                                      (float) kBlackW, kHeight * 0.62f);
                g.setColour((n + 1) == current_ ? Palette::accent
                            : (n + 1) == hover_ ? Palette::accent.brighter(0.2f)
                                                : ink::keyboard::blackKey);
                g.fillRect(r);
                g.setColour(Palette::border);
                g.drawRect(r, 1.0f);
            }
            x += kWhiteW;
        }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int n = noteAt(e.getPosition());
        if (n != hover_) { hover_ = n; repaint(); }
    }
    void mouseExit(const juce::MouseEvent&) override {
        if (hover_ >= 0) { hover_ = -1; repaint(); }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.y >= kHeight) {
            if (scrollable()) {
                if (e.x < kArrowW)                 shiftView(-12);
                else if (e.x > getWidth() - kArrowW) shiftView(12);
            }
            return;
        }
        const int n = noteAt(e.getPosition());
        if (n < lo_ || n > hi_) return;
        if (onPick_) onPick_(n);
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
    }
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override {
        if (w.deltaY != 0.0f) shiftView(w.deltaY > 0 ? 12 : -12);
    }

    static std::string noteName(int note) {
        static const char* k[12] = {"C", "C#", "D", "D#", "E", "F",
                                    "F#", "G", "G#", "A", "A#", "B"};
        note = juce::jlimit(0, kMidiMax, note);
        return std::string(k[note % 12]) + std::to_string(note / 12 - 1);
    }

private:
    static constexpr int kWhiteW = 16, kBlackW = 11, kHeight = 74, kLabelH = 18;
    static constexpr int kSpan = 60;
    static constexpr int kArrowW = 22;

    bool scrollable() const { return hi_ - lo_ > kSpan; }

    void shiftView(int d) {
        const int nvLo = juce::jlimit(lo_, juce::jmax(lo_, hi_ - kSpan), vLo_ + d);
        if (nvLo == vLo_) return;
        vLo_ = nvLo;
        vHi_ = juce::jmin(hi_, vLo_ + kSpan);
        hover_ = -1;
        layout();
        repaint();
    }

    void layout() {
        int whites = 0;
        for (int n = vLo_; n <= vHi_; ++n) if (isWhite(n)) ++whites;
        setSize(juce::jmax(scrollable() ? 2 * kArrowW + 60 : 1, whites * kWhiteW + 1),
                kHeight + kLabelH);
    }

    static bool isWhite(int n) {
        const int p = ((n % 12) + 12) % 12;
        return p == 0 || p == 2 || p == 4 || p == 5 || p == 7 || p == 9 || p == 11;
    }
    int noteAt(juce::Point<int> p) const {
        if (p.y >= kHeight) return -1;
        if (p.y < (int) (kHeight * 0.62f)) {
            int x = 0;
            for (int n = vLo_; n <= vHi_; ++n) {
                if (!isWhite(n)) continue;
                if (n + 1 <= vHi_ && !isWhite(n + 1)
                    && juce::Rectangle<int>(x + kWhiteW - kBlackW / 2, 0, kBlackW,
                                            (int) (kHeight * 0.62f)).contains(p))
                    return n + 1;
                x += kWhiteW;
            }
        }
        int x = 0;
        for (int n = vLo_; n <= vHi_; ++n) {
            if (!isWhite(n)) continue;
            if (juce::Rectangle<int>(x, 0, kWhiteW, kHeight).contains(p)) return n;
            x += kWhiteW;
        }
        return -1;
    }

    int current_, lo_, hi_;
    int vLo_ = 0, vHi_ = kMidiMax;
    int hover_ = -1;
    std::function<void(int)> onPick_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoNotePicker)
};

inline void showNotePicker(BrickHost& host, const std::string& cn, const std::string& param,
                           juce::Rectangle<int> anchorScreen, int lo = 0, int hi = kMidiMax,
                           std::function<void()> onChanged = {}) {
    const int cur = juce::jlimit(lo, hi, (int) host.liveParamValue(cn, param));
    auto picker = std::make_unique<PianoNotePicker>(cur, lo, hi,
        [&host, cn, param, onChanged = std::move(onChanged)](int n) {
            host.editParam(cn, param, (double) n);
            if (onChanged) onChanged();
        });
    juce::CallOutBox::launchAsynchronously(std::move(picker), anchorScreen, nullptr);
}

}
