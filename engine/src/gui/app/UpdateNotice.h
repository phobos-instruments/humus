// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/TimedCard.h"
#include "gui/style/Colours.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

class UpdateNotice : public TimedCard {
public:
    std::function<void()> onDownload, onSkip, onLater, onCancel;

    UpdateNotice(const juce::String& version, const juce::String& notes,
                 const juce::String& fileName = {})
        : title_("Humus " + version + " is ready"),
          notes_(notes.upToFirstOccurrenceOf("\n", false, false)),
          file_(fileName) {
        download_.setButtonText(tr("update-notice.download", "Download"));
        skip_.setButtonText(tr("update-notice.skip-this-version", "Skip this version"));
        later_.setButtonText(tr("update-notice.later", "Later"));
        download_.onClick = [this] { if (onDownload) onDownload(); };
        skip_.onClick = [this] { if (onSkip) onSkip(); };
        later_.onClick = [this] {
            if (busy_ && onCancel) { onCancel(); return; }
            if (onLater) onLater();
        };
        onDismiss = [this] {
            if (busy_ && onCancel) { onCancel(); return; }
            if (onLater) onLater();
        };
        addAndMakeVisible(download_);
        addAndMakeVisible(skip_);
        addAndMakeVisible(later_);
        hideDismiss();
        const int body = (file_.isEmpty() ? 0 : 18) + (notes_.isEmpty() ? 0 : 32);
        setSize(kCardWidth, 90 + body);
    }

    void setProgress(double fraction) {
        busy_ = true;
        progress_ = fraction;
        download_.setVisible(false);
        skip_.setVisible(false);
        later_.setButtonText(tr("update-notice.cancel", "Cancel"));
        resized();
        repaint();
    }

    void setOutcome(const juce::String& message, const juce::String& action) {
        busy_ = false;
        notes_ = message;
        download_.setButtonText(action);
        download_.setVisible(true);
        skip_.setVisible(false);
        later_.setButtonText(tr("update-notice.close", "Close"));
        resized();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        paintBody(g, true);
        g.setColour(Palette::text);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(title_, 14, 10, titleWidth(), 20, juce::Justification::centredLeft);
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

    void layout() override {
        placeButtons(buttonRow(), {&download_, &skip_, &later_});
    }

private:
    juce::String title_, notes_, file_;
    bool busy_ = false;
    double progress_ = 0.0;
    juce::TextButton download_, skip_, later_;
};

}
