#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

namespace hum {

class NewPodPanel : public juce::Component {
public:
    std::function<void(int, int, int, int, bool)> onCreate;

    NewPodPanel() {
        auto stepper = [this](juce::Slider& s, int lo, int hi, int def) {
            s.setSliderStyle(juce::Slider::IncDecButtons);
            s.setRange(lo, hi, 1);
            s.setValue(def, juce::dontSendNotification);
            s.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 34, 22);
            s.onValueChange = [this] { repaint(); };
            addAndMakeVisible(s);
        };
        stepper(ins_, 0, 16, 2);
        stepper(outs_, 1, 16, 2);
        stepper(midiIns_, 0, 8, 1);
        stepper(midiOuts_, 0, 8, 1);

        ports_.addItem(tr("new-pod.stereo-pairs", "Stereo pairs"), 1);
        ports_.addItem(tr("new-pod.mono-ports", "Mono ports"), 2);
        ports_.setSelectedId(1, juce::dontSendNotification);
        ports_.onChange = [this] { repaint(); };
        addAndMakeVisible(ports_);

        createBtn_.setColour(juce::TextButton::buttonColourId, Palette::accentDim);
        createBtn_.setColour(juce::TextButton::textColourOffId, Palette::text);
        createBtn_.onClick = [this] { create(); };
        cancelBtn_.onClick = [this] { dismiss(); };
        addAndMakeVisible(createBtn_);
        addAndMakeVisible(cancelBtn_);

        setWantsKeyboardFocus(true);
        setSize(330, 390);
    }

    static void show(juce::Rectangle<int> screenAnchor,
                     std::function<void(int, int, int, int, bool)> onCreate) {
        auto content = std::make_unique<NewPodPanel>();
        content->onCreate = std::move(onCreate);
        juce::CallOutBox::launchAsynchronously(std::move(content), screenAnchor, nullptr);
    }

    void resized() override {
        auto r = getLocalBounds().reduced(14, 12);
        r.removeFromTop(44);
        preview_ = r.removeFromTop(110);
        r.removeFromTop(10);
        auto row = [&](juce::Component& c) {
            auto rr = r.removeFromTop(26);
            rr.removeFromLeft(96);
            c.setBounds(rr.removeFromLeft(120));
            r.removeFromTop(6);
        };
        row(ins_);
        row(outs_);
        row(midiIns_);
        row(midiOuts_);
        auto pr = r.removeFromTop(26);
        pr.removeFromLeft(96);
        ports_.setBounds(pr.removeFromLeft(140));
        r.removeFromTop(12);
        auto btns = r.removeFromTop(28);
        createBtn_.setBounds(btns.removeFromRight(90));
        btns.removeFromRight(8);
        cancelBtn_.setBounds(btns.removeFromRight(80));
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::background);
        auto r = getLocalBounds().reduced(14, 12);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawText(tr("new-pod.new-pod", "New Pod"), r.removeFromTop(20), juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(11.5f));
        g.drawText(tr("new-pod.a-patch-inside-one-box", "A patch inside one box. Double-click the box to open it."),
                   r.removeFromTop(18), juce::Justification::centredLeft);

        drawPodPreview(g);

        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(12.5f));
        auto rows = preview_.getBottom() + 10;
        const juce::String labels[] = {tr("new-pod.audio-in", "Audio in"), tr("new-pod.audio-out", "Audio out"), tr("new-pod.midi-in", "MIDI in"), tr("new-pod.midi-out", "MIDI out"), "Ports"};
        for (int i = 0; i < 5; ++i)
            g.drawText(labels[i], r.getX(), rows + i * 32, 92, 26,
                       juce::Justification::centredLeft);
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (k.getKeyCode() == juce::KeyPress::returnKey) { create(); return true; }
        if (k.getKeyCode() == juce::KeyPress::escapeKey) { dismiss(); return true; }
        return false;
    }

    void parentHierarchyChanged() override {
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<NewPodPanel>(this)] {
            if (safe != nullptr) safe->grabKeyboardFocus();
        });
    }

private:
    void drawPodPreview(juce::Graphics& g) {
        const int aIns = (int) ins_.getValue(), aOuts = (int) outs_.getValue();
        const int mIns = (int) midiIns_.getValue(), mOuts = (int) midiOuts_.getValue();

        const int pin = 7, midiGap = 14;
        const int nA = juce::jmax(aIns, aOuts), nM = juce::jmax(mIns, mOuts);
        int spacing = pin + 6;
        const int avail = preview_.getWidth() - 8;
        if (24 + nA * spacing + (nM > 0 ? midiGap + nM * spacing : 0) > avail)
            spacing = juce::jmax(pin + 1, (avail - 24 - (nM > 0 ? midiGap : 0))
                                              / juce::jmax(1, nA + nM));
        int w = juce::jmax(120, 24 + nA * spacing + (nM > 0 ? midiGap + nM * spacing : 0));
        const int h = 52;
        juce::Rectangle<int> box(preview_.getCentreX() - w / 2, preview_.getY() + 16, w, h);
        auto bf = box.toFloat();
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(bf, 7.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(bf, 7.0f, 1.0f);
        g.setColour(Palette::accent.withAlpha(0.45f));
        g.drawRoundedRectangle(bf.reduced(3.0f), 5.0f, 1.0f);
        g.setColour(Palette::accent);
        g.setFont(juce::FontOptions(12.5f));
        g.drawText(juce::String::fromUTF8("Pod  \xe2\x96\xb8"), box,
                   juce::Justification::centred);

        const bool stereo = ports_.getSelectedId() == 1;
        auto dots = [&](int n, int y, bool midiSide, bool topEdge, juce::Colour c) {
            auto px = [&](int i) {
                return midiSide ? box.getRight() - 12 - (n - 1 - i) * spacing
                                : box.getX() + 12 + i * spacing;
            };
            g.setColour(c);
            for (int i = 0; i < n; ++i)
                g.fillEllipse((float) px(i) - pin / 2.0f, (float) y - pin / 2.0f,
                              (float) pin, (float) pin);
            if (midiSide || !stereo) return;
            g.setColour(c.withAlpha(0.8f));
            const float ty = topEdge ? (float) y - pin - 5.0f : (float) y + pin + 5.0f;
            const float leg = topEdge ? 4.0f : -4.0f;
            for (int i = 0; i + 1 < n; i += 2) {
                juce::Path tie;
                tie.startNewSubPath((float) px(i), ty + leg);
                tie.lineTo((float) px(i), ty);
                tie.lineTo((float) px(i + 1), ty);
                tie.lineTo((float) px(i + 1), ty + leg);
                g.strokePath(tie, juce::PathStrokeType(1.2f));
            }
        };
        dots(aIns, box.getY(), false, true, Palette::textDim);
        dots(aOuts, box.getBottom(), false, false, Palette::textDim);
        dots(mIns, box.getY(), true, true, Palette::midiCord());
        dots(mOuts, box.getBottom(), true, false, Palette::midiCord());

        auto caption = [&](int ch) -> juce::String {
            if (ch <= 0) return "0";
            if (!stereo) return juce::String(ch) + " mono";
            const int pairs = ch / 2, rest = ch % 2;
            juce::String s = pairs > 0 ? juce::String(pairs) + " stereo" : juce::String();
            if (rest) s += (s.isEmpty() ? "" : " + ") + juce::String("1 mono");
            return s;
        };
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(10.5f));
        g.drawText(tr("new-pod.in", "in: ") + caption(aIns) + tr("new-pod.out", "    out: ") + caption(aOuts),
                   preview_.withTop(preview_.getBottom() - 16),
                   juce::Justification::centredTop);
    }

    void create() {
        const auto fire = onCreate;
        const int a = (int) ins_.getValue(), b = (int) outs_.getValue();
        const int c = (int) midiIns_.getValue(), d = (int) midiOuts_.getValue();
        const bool stereo = ports_.getSelectedId() == 1;
        dismiss();
        if (fire) fire(a, b, c, d, stereo);
    }

    void dismiss() {
        if (auto* box = findParentComponentOfClass<juce::CallOutBox>()) box->dismiss();
    }

    juce::Rectangle<int> preview_;
    juce::Slider ins_, outs_, midiIns_, midiOuts_;
    juce::ComboBox ports_;
    juce::TextButton createBtn_{"Create"}, cancelBtn_{"Cancel"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewPodPanel)
};

}
