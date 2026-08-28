#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"

namespace hum {

class UpdateNotice : public juce::Component {
public:
    std::function<void()> onDownload, onSkip, onLater, onCancel;

    UpdateNotice(const juce::String& version, const juce::String& notes,
                 const juce::String& fileName = {})
        : title_("Humus " + version + " is ready"),
          notes_(notes.upToFirstOccurrenceOf("\n", false, false)),
          file_(fileName) {
        download_.setButtonText("Download");
        skip_.setButtonText("Skip this version");
        later_.setButtonText("Later");
        download_.onClick = [this] { if (onDownload) onDownload(); };
        skip_.onClick = [this] { if (onSkip) onSkip(); };
        later_.onClick = [this] {
            if (busy_ && onCancel) { onCancel(); return; }
            if (onLater) onLater();
        };
        addAndMakeVisible(download_);
        addAndMakeVisible(skip_);
        addAndMakeVisible(later_);
        const int body = (file_.isEmpty() ? 0 : 18) + (notes_.isEmpty() ? 0 : 32);
        setSize(348, 96 + body);
    }

    void setProgress(double fraction) {
        busy_ = true;
        progress_ = fraction;
        download_.setVisible(false);
        skip_.setVisible(false);
        later_.setButtonText("Cancel");
        resized();
        repaint();
    }

    void setOutcome(const juce::String& message, const juce::String& action) {
        busy_ = false;
        notes_ = message;
        download_.setButtonText(action);
        download_.setVisible(true);
        skip_.setVisible(false);
        later_.setButtonText("Close");
        resized();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(Palette::accent.withAlpha(0.7f));
        g.drawRoundedRectangle(r, 6.0f, 1.2f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(title_, 14, 10, getWidth() - 28, 20, juce::Justification::centredLeft);
        if (busy_) {
            const auto bar = juce::Rectangle<float>(14.0f, 38.0f,
                                                    (float) getWidth() - 28.0f, 6.0f);
            g.setColour(Palette::panelLight);
            g.fillRoundedRectangle(bar, 3.0f);
            g.setColour(Palette::accent);
            const double f = progress_ < 0.0 ? 1.0 : juce::jlimit(0.0, 1.0, progress_);
            g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * (float) f), 3.0f);
        } else {
            int y = 32;
            if (file_.isNotEmpty()) {
                g.setColour(Palette::text);
                g.setFont(juce::FontOptions(11.5f));
                g.drawText(file_, 14, y, getWidth() - 28, 16,
                           juce::Justification::centredLeft);
                y += 18;
            }
            if (notes_.isNotEmpty()) {
                g.setColour(Palette::textDim);
                g.setFont(juce::FontOptions(12.0f));
                g.drawFittedText(notes_, 14, y, getWidth() - 28, 30,
                                 juce::Justification::topLeft, 2);
            }
        }
    }

    void resized() override {
        auto row = getLocalBounds().reduced(12).removeFromBottom(26);
        if (download_.isVisible()) {
            download_.setBounds(row.removeFromLeft(
                download_.getButtonText().length() > 10 ? 132 : 96));
            row.removeFromLeft(8);
        }
        if (skip_.isVisible()) {
            skip_.setBounds(row.removeFromLeft(130));
            row.removeFromLeft(8);
        }
        later_.setBounds(row.removeFromLeft(70));
    }

private:
    juce::String title_, notes_, file_;
    bool busy_ = false;
    double progress_ = 0.0;
    juce::TextButton download_, skip_, later_;
};

class NagCard : public juce::Component {
public:
    std::function<void()> onEnterLicense, onLater;

    NagCard() {
        enter_.setButtonText("Enter License");
        later_.setButtonText("Later");
        enter_.onClick = [this] { if (onEnterLicense) onEnterLicense(); };
        later_.onClick = [this] { if (onLater) onLater(); };
        addAndMakeVisible(enter_);
        addAndMakeVisible(later_);
        setSize(348, 114);
    }

    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(Palette::border);
        g.drawRoundedRectangle(r, 6.0f, 1.2f);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("Humus is unregistered", 14, 10, getWidth() - 28, 20,
                   juce::Justification::centredLeft);
        g.setColour(Palette::textDim);
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText("Everything works, and keeps working. Got a code? "
                         "Make it official.",
                         14, 32, getWidth() - 28, 32,
                         juce::Justification::topLeft, 2);
    }

    void resized() override {
        auto row = getLocalBounds().reduced(12).removeFromBottom(26);
        enter_.setBounds(row.removeFromLeft(120));
        row.removeFromLeft(8);
        later_.setBounds(row.removeFromLeft(70));
    }

private:
    juce::TextButton enter_, later_;
};

}
