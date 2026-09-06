#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/LookAndFeel.h"
#include "gui/VideoThumbCache.h"
#include "gui/VideoThumbStore.h"
#include "gui/VideoTakeRecorder.h"
#include "gui/Localisation.h"

namespace hum {

class VideoCacheSettingsView : public juce::Component {
public:
    VideoCacheSettingsView() {
        title_.setText(tr("video-cache-settings.video", "Video"), juce::dontSendNotification);
        title_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        addAndMakeVisible(title_);

        blurb_.setText(tr("video-cache-settings.video-clips-on-the-timeline",
           "Video clips on the timeline wear a strip of thumbnails, one frame per "
           "second of tape. They are read once and kept on disk so a tape you cut "
           "again opens with its strip already drawn."),
                       juce::dontSendNotification);
        blurb_.setFont(juce::FontOptions(12.0f));
        blurb_.setColour(juce::Label::textColourId, Palette::textDim);
        blurb_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(blurb_);

        sizeLabel_.setText(tr("video-cache-settings.keep-at-most", "Keep at most"), juce::dontSendNotification);
        sizeLabel_.setFont(juce::FontOptions(13.0f));
        addAndMakeVisible(sizeLabel_);

        for (const int mb : kSizes)
            sizeCombo_.addItem(mb == 0 ? juce::String(tr("video-cache-settings.nothing-decode-every-time", "Nothing (decode every time)"))
                                       : juce::String(mb) + " MB",
                               mb + 1);
        sizeCombo_.onChange = [this] {
            thumbstore::setBudgetMB(sizeCombo_.getSelectedId() - 1);
            thumbstore::prune();
            refresh();
        };
        addAndMakeVisible(sizeCombo_);

        clearBtn_.onClick = [this] {
            thumbstore::clear();
            VideoThumbCache::instance().forget();
            refresh();
        };
        addAndMakeVisible(clearBtn_);

        usage_.setFont(juce::FontOptions(12.0f));
        usage_.setColour(juce::Label::textColourId, Palette::textDim);
        addAndMakeVisible(usage_);

        takeLabel_.setText(tr("video-cache-settings.record-at", "Record video at"),
                           juce::dontSendNotification);
        takeLabel_.setFont(juce::FontOptions(13.0f));
        addAndMakeVisible(takeLabel_);
        for (const int h : videotake::kHeights)
            takeCombo_.addItem(juce::String(videotake::widthFor(h)) + " x " + juce::String(h), h);
        takeCombo_.onChange = [this] { videotake::setHeightSetting(takeCombo_.getSelectedId()); };
        addAndMakeVisible(takeCombo_);
        takeBlurb_.setText(tr("video-cache-settings.a-video-track-armed",
           "A video track armed to record writes whatever reaches its inlet as a compact "
           "movie in the recordings folder, and the take lands on the timeline as a clip."),
                           juce::dontSendNotification);
        takeBlurb_.setFont(juce::FontOptions(12.0f));
        takeBlurb_.setColour(juce::Label::textColourId, Palette::textDim);
        takeBlurb_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(takeBlurb_);
        refresh();
    }

    void refresh() {
        const int mb = thumbstore::budgetMB();
        int pick = kSizes[0];
        for (const int s : kSizes)
            if (s <= mb) pick = s;
        sizeCombo_.setSelectedId(pick + 1, juce::dontSendNotification);
        const auto used = thumbstore::bytesUsed();
        usage_.setText(used <= 0 ? juce::String(tr("video-cache-settings.nothing-stored-yet", "Nothing stored yet."))
                                 : juce::File::descriptionOfSizeInBytes(used) + " stored in "
                                       + thumbstore::dir().getFullPathName(),
                       juce::dontSendNotification);
        clearBtn_.setEnabled(used > 0);
        takeCombo_.setSelectedId(videotake::heightSetting(), juce::dontSendNotification);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(8, 4);
        title_.setBounds(area.removeFromTop(26));
        area.removeFromTop(6);
        blurb_.setBounds(area.removeFromTop(52));
        area.removeFromTop(10);
        auto row = area.removeFromTop(26);
        sizeLabel_.setBounds(row.removeFromLeft(110));
        sizeCombo_.setBounds(row.removeFromLeft(220));
        row.removeFromLeft(12);
        clearBtn_.setBounds(row.removeFromLeft(120));
        area.removeFromTop(10);
        usage_.setBounds(area.removeFromTop(20));
        area.removeFromTop(18);
        auto take = area.removeFromTop(26);
        takeLabel_.setBounds(take.removeFromLeft(110));
        takeCombo_.setBounds(take.removeFromLeft(220));
        area.removeFromTop(6);
        takeBlurb_.setBounds(area.removeFromTop(40));
    }

private:
    static constexpr int kSizes[] = {0, 64, 128, 256, 512, 1024, 2048};

    juce::Label title_, blurb_, sizeLabel_, usage_, takeLabel_, takeBlurb_;
    juce::ComboBox sizeCombo_, takeCombo_;
    juce::TextButton clearBtn_{tr("video-cache-settings.clear-now", "Clear now")};
};

}
