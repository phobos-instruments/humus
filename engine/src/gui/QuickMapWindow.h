#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"
#include "gui/Localisation.h"

namespace hum {

class QuickMapWindow : public juce::DocumentWindow {
public:
    QuickMapWindow(const juce::String& title, const juce::String& what,
                   std::function<void()> onCancel)
        : juce::DocumentWindow(title, Palette::panel, juce::DocumentWindow::closeButton) {
        setUsingNativeTitleBar(true);
        auto* c = new Content(what, std::move(onCancel));
        setContentOwned(c, true);
        setAlwaysOnTop(true);
        centreAroundComponent(nullptr, getWidth(), getHeight());
        setVisible(true);
        toFront(true);
        c->grabKeyboardFocus();
    }

    void setCountdown(int seconds) {
        if (auto* c = dynamic_cast<Content*>(getContentComponent())) c->setCountdown(seconds);
    }
    void setMessage(const juce::String& what) {
        if (auto* c = dynamic_cast<Content*>(getContentComponent())) c->setMessage(what);
    }

    void closeButtonPressed() override {
        if (auto* c = dynamic_cast<Content*>(getContentComponent())) c->cancel();
    }

private:
    class Content : public juce::Component {
    public:
        Content(const juce::String& what, std::function<void()> onCancel)
            : onCancel_(std::move(onCancel)) {
            textH_ = juce::StringArray::fromLines(what).size() * 19 + 4;
            setSize(460, 8 + textH_ + 18 + 8 + 26 + 8);
            setWantsKeyboardFocus(true);
            text_.setText(what, juce::dontSendNotification);
            text_.setJustificationType(juce::Justification::centred);
            text_.setMinimumHorizontalScale(1.0f);
            addAndMakeVisible(text_);
            wait_.setJustificationType(juce::Justification::centred);
            wait_.setColour(juce::Label::textColourId, Palette::textDim);
            wait_.setFont(juce::FontOptions(12.0f));
            addAndMakeVisible(wait_);
            cancelBtn_.setButtonText(tr("quick-map.cancel", "Cancel"));
            cancelBtn_.onClick = [this] { cancel(); };
            addAndMakeVisible(cancelBtn_);
        }
        void setMessage(const juce::String& what) {
            text_.setText(what, juce::dontSendNotification);
        }
        void setCountdown(int seconds) {
            wait_.setText(tr("quick-map.waiting", "waiting ") + juce::String(seconds) + "s "
                              + juce::String("-") + " Esc to cancel",
                          juce::dontSendNotification);
        }
        void cancel() {
            if (onCancel_)
                juce::MessageManager::callAsync([cb = onCancel_] { cb(); });
        }
        bool keyPressed(const juce::KeyPress& k) override {
            if (k == juce::KeyPress::escapeKey) { cancel(); return true; }
            return false;
        }
        void resized() override {
            auto r = getLocalBounds().reduced(12, 8);
            text_.setBounds(r.removeFromTop(textH_));
            wait_.setBounds(r.removeFromTop(18));
            r.removeFromTop(8);
            cancelBtn_.setBounds(r.removeFromTop(26).withSizeKeepingCentre(90, 26));
        }
    private:
        int textH_ = 60;
        juce::Label text_, wait_;
        juce::TextButton cancelBtn_;
        std::function<void()> onCancel_;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickMapWindow)
};

namespace mapconflict {
inline void ask(const juce::String& kind, const juce::String& source, const juce::String& target,
                const std::vector<std::pair<std::string, std::string>>& others,
                std::function<void(bool steal)> onResolve) {
    juce::String list;
    for (const auto& [o, p] : others) {
        if (list.isNotEmpty()) list << ", ";
        list << juce::String(o) << " / " << juce::String(p);
    }
    juce::AlertWindow::showYesNoCancelBox(
        juce::MessageBoxIconType::QuestionIcon, source + " is already assigned",
        source + " already controls " + list + ".\n\nReassign it to " + target
            + ", or add " + target + " so they move together?",
        tr("quick-map.reassign", "Reassign"), tr("quick-map.add-both", "Add both"),
        tr("quick-map.cancel", "Cancel"), nullptr,
        juce::ModalCallbackFunction::create([onResolve](int r) {
            if (r == 1) onResolve(true);
            else if (r == 2) onResolve(false);
        }));
}
}

}
