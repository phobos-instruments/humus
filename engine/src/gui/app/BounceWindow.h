// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/app/BounceReceipt.h"
#include "gui/app/BounceWants.h"
#include "gui/host/EngineHost.h"
#include "gui/common/Localisation.h"
#include "gui/style/LookAndFeel.h"
#include "gui/video/VisualPlanBuilder.h"

namespace hum {

inline constexpr int kQualitySteps = 4;
inline constexpr int kQualityCrf[kQualitySteps] = {28, 23, 19, 14};

class BounceWindow : public juce::Component {
public:
    std::function<void(const BounceWants&)> onBounce;
    std::function<void()> onCancel;

    explicit BounceWindow(BounceOffer offer) : offer_(std::move(offer)) {
        row(audio_, tr("bounce.audio", "Audio"), offer_.hasSound,
            tr("bounce.no-sound", "nothing in this patch reaches the master"));
        row(video_, tr("bounce.video", "Video"),
            !offer_.videoNodes.empty() && movieKindAvailable(MovieKind::H264),
            !movieKindAvailable(MovieKind::H264)
                ? tr("bounce.no-encoder", "this build has no video encoder")
                : offer_.cameraOnly
                      ? tr("bounce.camera-only", "a camera has no timeline to render")
                      : tr("bounce.no-video-out", "no video output in this patch"));
        row(midi_, tr("bounce.midi", "MIDI"), offer_.hasNotes,
            tr("bounce.no-notes", "no notes to write"));
        audio_.setToggleState(offer_.hasSound, juce::dontSendNotification);
        video_.setToggleState(video_.isEnabled(), juce::dontSendNotification);

        juce::StringArray sounds;
        for (const auto& kind : receipt::soundKinds()) sounds.add(kind.label);
        choice(sound_, sounds, 1);
        juce::StringArray rates;
        for (int i = 0; i < kMp3RateSteps; ++i) rates.add(receipt::mp3RateLabel(i));
        choice(mp3_, rates, 1);
        juce::StringArray outs;
        for (const auto& n : offer_.videoNodes) outs.add(juce::String(n));
        if (outs.isEmpty()) outs.add(tr("bounce.no-output", "(none)"));
        choice(out_, outs, 1);
        choice(size_, {"1920 x 1080", "1280 x 720", "960 x 540", "640 x 360"}, 2);
        choice(fps_, {"60 fps", "30 fps", "25 fps", "24 fps"}, 2);
        choice(quality_, {tr("bounce.draft", "Draft"), tr("bounce.good", "Good"),
                          tr("bounce.high", "High"), tr("bounce.highest", "Highest")}, 3);

        choice(range_, {tr("bounce.whole-song", "Whole song"), tr("bounce.the-loop", "The loop"),
                        tr("bounce.the-selection", "The selection"),
                        tr("bounce.from-the-start", "From the start")}, 1);
        range_.setItemEnabled(2, offer_.loopTo > offer_.loopFrom);
        range_.setItemEnabled(3, offer_.selectTo > offer_.selectFrom);
        if (offer_.loopTo <= offer_.loopFrom)
            range_.changeItemText(2, tr("bounce.the-loop", "The loop") + " - "
                                         + tr("bounce.no-loop", "none set"));
        if (offer_.selectTo <= offer_.selectFrom)
            range_.changeItemText(3, tr("bounce.the-selection", "The selection") + " - "
                                         + tr("bounce.no-selection", "nothing selected"));

        bars_.setText("8", juce::dontSendNotification);
        bars_.setInputRestrictions(3, "0123456789");
        addAndMakeVisible(bars_);
        name_.setText(offer_.stem, juce::dontSendNotification);
        addAndMakeVisible(name_);
        label(extension_, {});
        extension_.setColour(juce::Label::textColourId, Palette::text);

        label(formatLabel_, tr("bounce.picture", "Picture"));
        label(qualityLabel_, tr("bounce.quality", "Quality"));
        label(rangeLabel_, tr("bounce.length", "Length"));
        label(barsLabel_, tr("bounce.bars", "bars"));
        label(folderLabel_, tr("bounce.save-to", "Save to"));
        label(nameLabel_, tr("bounce.name", "Name"));
        folder_.setText(offer_.folder.getFullPathName(), juce::dontSendNotification);
        addAndMakeVisible(folder_);
        choose_.setButtonText(tr("bounce.choose", "Choose..."));
        choose_.onClick = [this] { pickFolder(); };
        addAndMakeVisible(choose_);

        go_.setButtonText(tr("bounce.bounce", "Bounce"));
        stop_.setButtonText(tr("bounce.cancel", "Cancel"));
        go_.onClick = [this] { if (onBounce) onBounce(wants()); };
        stop_.onClick = [this] { if (onCancel) onCancel(); };
        addAndMakeVisible(go_);
        addAndMakeVisible(stop_);

        for (auto* c : {&audio_, &video_, &midi_}) c->onClick = [this] { refresh(); };
        for (auto* c : {&sound_, &mp3_, &out_, &size_, &fps_, &quality_, &range_})
            c->onChange = [this] { refresh(); };
        bars_.onTextChange = [this] { refresh(); };
        name_.onTextChange = [this] { refresh(); };
        setSize(576, 424);
        refresh();
    }

    BounceWants wants() const {
        BounceWants w;
        w.audio = audio_.isEnabled() && audio_.getToggleState();
        w.video = video_.isEnabled() && video_.getToggleState();
        w.midi = midi_.isEnabled() && midi_.getToggleState();
        const auto kinds = receipt::soundKinds();
        w.soundKind = kinds[(size_t) juce::jlimit(0, (int) kinds.size() - 1,
                                                  sound_.getSelectedItemIndex())].extension;
        w.mp3Rate = juce::jlimit(0, kMp3RateSteps - 1, mp3_.getSelectedItemIndex());
        if (!offer_.videoNodes.empty())
            w.videoNode = offer_.videoNodes[(size_t) juce::jlimit(
                0, (int) offer_.videoNodes.size() - 1, out_.getSelectedItemIndex())];
        w.kind = MovieKind::H264;
        w.quality = kQualityCrf[juce::jlimit(0, kQualitySteps - 1,
                                             quality_.getSelectedItemIndex())];
        const int wide[4] = {1920, 1280, 960, 640};
        const int tall[4] = {1080, 720, 540, 360};
        const int pick = juce::jlimit(0, 3, size_.getSelectedItemIndex());
        w.width = wide[pick];
        w.height = tall[pick];
        w.fps = fps_.getText().getDoubleValue();
        switch (range_.getSelectedId()) {
            case 2: w.fromBeat = offer_.loopFrom; w.toBeat = offer_.loopTo; break;
            case 3: w.fromBeat = offer_.selectFrom; w.toBeat = offer_.selectTo; break;
            case 4:
                w.fromBeat = 0.0;
                w.toBeat = std::max(1.0, bars_.getText().getDoubleValue()) * 4.0;
                break;
            default: w.fromBeat = 0.0; w.toBeat = offer_.songEndBeat; break;
        }
        w.folder = juce::File(folder_.getText());
        w.stem = name_.getText().trim();
        return w;
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::background);
        g.setColour(Palette::border);
        for (auto y : rules_) g.drawHorizontalLine(y, 20.0f, (float) getWidth() - 20.0f);
        paintReceipt(g);
    }

    void resized() override {
        auto b = getLocalBounds().reduced(20, 16);
        auto foot = b.removeFromBottom(30);
        go_.setBounds(foot.removeFromRight(112));
        foot.removeFromRight(8);
        stop_.setBounds(foot.removeFromRight(112));
        b.removeFromBottom(12);
        receipt_ = b.removeFromBottom(104);
        b.removeFromBottom(14);

        rules_.clear();
        const int labelW = 92;
        auto line = [&b](int height) { return b.removeFromTop(height); };
        auto gap = [&b](int height) { b.removeFromTop(height); };

        auto audioRow = line(24);
        audio_.setBounds(audioRow.removeFromLeft(audio_.isEnabled() ? labelW + 24 : 480));
        if (audio_.isEnabled()) {
            if (mp3_.isVisible()) mp3_.setBounds(audioRow.removeFromRight(148));
            sound_.setBounds(audioRow.withTrimmedLeft(8).withTrimmedRight(mp3_.isVisible() ? 8 : 0));
        } else {
            sound_.setBounds({});
        }
        gap(6);
        auto videoRow = line(24);
        video_.setBounds(videoRow.removeFromLeft(video_.isEnabled() ? labelW + 24 : 480));
        if (video_.isEnabled()) out_.setBounds(videoRow.withTrimmedLeft(8));
        else out_.setBounds({});
        gap(6);
        auto formatRow = line(24);
        formatLabel_.setBounds(formatRow.removeFromLeft(labelW).withTrimmedLeft(24));
        formatRow.removeFromLeft(8);
        size_.setBounds(formatRow.removeFromLeft(132));
        formatRow.removeFromLeft(8);
        fps_.setBounds(formatRow.removeFromLeft(88));
        gap(6);
        auto qualityRow = line(24);
        qualityLabel_.setBounds(qualityRow.removeFromLeft(labelW * 2).withTrimmedLeft(24));
        qualityRow.removeFromLeft(8);
        quality_.setBounds(qualityRow.removeFromLeft(132));
        gap(6);
        midi_.setBounds(line(24));
        gap(10);
        rules_.push_back(b.getY());
        gap(12);

        auto rangeRow = line(24);
        rangeLabel_.setBounds(rangeRow.removeFromLeft(labelW));
        rangeRow.removeFromLeft(8);
        range_.setBounds(rangeRow.removeFromLeft(180));
        rangeRow.removeFromLeft(10);
        bars_.setBounds(rangeRow.removeFromLeft(52));
        rangeRow.removeFromLeft(6);
        barsLabel_.setBounds(rangeRow.removeFromLeft(48));
        gap(10);
        auto folderRow = line(24);
        folderLabel_.setBounds(folderRow.removeFromLeft(labelW));
        folderRow.removeFromLeft(8);
        choose_.setBounds(folderRow.removeFromRight(96));
        folderRow.removeFromRight(8);
        folder_.setBounds(folderRow);
        gap(6);
        auto nameRow = line(24);
        nameLabel_.setBounds(nameRow.removeFromLeft(labelW));
        nameRow.removeFromLeft(8);
        extension_.setBounds(nameRow.removeFromRight(120));
        name_.setBounds(nameRow);
    }

    juce::Rectangle<int> receiptBoundsForTest() const { return receipt_; }
    receipt::Summary summaryForTest() const { return receipt::summarise(wants(), offer_.tempo); }
    bool canBounceForTest() const { return go_.isEnabled(); }
    void chooseRateForTest(int step) {
        mp3_.setSelectedItemIndex(juce::jlimit(0, kMp3RateSteps - 1, step));
        refresh();
    }
    bool rateShowingForTest() const { return mp3_.isVisible(); }
    void chooseSoundForTest(const juce::String& extension) {
        const auto kinds = receipt::soundKinds();
        for (size_t i = 0; i < kinds.size(); ++i)
            if (extension == kinds[i].extension) sound_.setSelectedItemIndex((int) i);
        refresh();
    }

    void tickForTest(bool sound, bool picture, bool notes) {
        audio_.setToggleState(sound, juce::dontSendNotification);
        video_.setToggleState(picture, juce::dontSendNotification);
        midi_.setToggleState(notes, juce::dontSendNotification);
        refresh();
    }
private:
    void refresh() {
        const auto w = wants();
        const bool picture = video_.isEnabled() && video_.getToggleState();
        for (auto* c : {&size_, &fps_, &out_, &quality_}) c->setEnabled(picture);
        const bool inside = picture && audio_.isEnabled() && audio_.getToggleState();
        sound_.setEnabled(audio_.isEnabled() && audio_.getToggleState() && !inside);
        audio_.setButtonText(inside ? tr("bounce.audio", "Audio") + " - "
                                          + tr("bounce.inside-the-movie", "goes inside the movie")
                                    : tr("bounce.audio", "Audio"));
        const bool wantsMp3 = sound_.isEnabled() && w.soundKind == "mp3";
        mp3_.setVisible(wantsMp3);
        resized();
        const bool counted = range_.getSelectedId() == 4;
        bars_.setVisible(counted);
        barsLabel_.setVisible(counted);
        juce::StringArray extensions;
        for (const auto& file : receipt::summarise(w, offer_.tempo).files)
            extensions.add(juce::File(file.file).getFileExtension());
        extension_.setText(extensions.joinIntoString(" + "), juce::dontSendNotification);
        go_.setEnabled(!w.nothingChosen() && w.stem.isNotEmpty() && w.toBeat > w.fromBeat);
        repaint();
    }

    void paintReceipt(juce::Graphics& g) {
        g.setColour(Palette::panel);
        g.fillRoundedRectangle(receipt_.toFloat(), 3.0f);
        g.setColour(Palette::accentDim);
        g.fillRect(receipt_.getX(), receipt_.getY(), 2, receipt_.getHeight());

        auto area = receipt_.reduced(16, 12).withTrimmedLeft(4);
        g.setColour(Palette::accent);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(tr("bounce.estimations", "ESTIMATIONS"), area.removeFromTop(14),
                   juce::Justification::centredLeft);
        area.removeFromTop(6);

        const auto made = receipt::summarise(wants(), offer_.tempo);
        if (made.files.empty()) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(tr("bounce.nothing-yet", "Nothing yet. Tick something above."),
                       area.removeFromTop(16), juce::Justification::centredLeft);
            return;
        }
        auto row = [&](const juce::String& name, const juce::String& value) {
            auto line = area.removeFromTop(18);
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(name, line.removeFromLeft(170), juce::Justification::centredLeft);
            g.setColour(Palette::text);
            g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
            g.drawText(value, line, juce::Justification::centredLeft, true);
        };
        row(tr("bounce.length", "Length"), made.duration);
        row(tr("bounce.estimated-size", "Estimated size"), made.size);
        row(tr("bounce.estimated-bounce-time", "Estimated bounce time"), made.time);
    }

    void pickFolder() {
        chooser_ = std::make_unique<juce::FileChooser>(tr("bounce.save-to", "Save to"),
                                                       juce::File(folder_.getText()));
        chooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectDirectories,
                              [this](const juce::FileChooser& fc) {
            if (fc.getResult() == juce::File()) return;
            folder_.setText(fc.getResult().getFullPathName(), juce::dontSendNotification);
            refresh();
        });
    }

    void row(juce::ToggleButton& b, const juce::String& text, bool live, const juce::String& why) {
        b.setButtonText(live ? text : text + " - " + why);
        b.setEnabled(live);
        addAndMakeVisible(b);
    }

    void choice(juce::ComboBox& c, const juce::StringArray& items, int selected) {
        c.addItemList(items, 1);
        c.setSelectedItemIndex(juce::jlimit(0, std::max(0, items.size() - 1), selected - 1),
                               juce::dontSendNotification);
        addAndMakeVisible(c);
    }

    void label(juce::Label& l, const juce::String& text) {
        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::FontOptions(12.0f));
        l.setColour(juce::Label::textColourId, Palette::textDim);
        addAndMakeVisible(l);
    }

    BounceOffer offer_;
    juce::ToggleButton audio_, video_, midi_;
    juce::ComboBox sound_, mp3_, out_, size_, fps_, quality_, range_;
    juce::TextEditor bars_, name_;
    juce::Label formatLabel_, qualityLabel_, rangeLabel_, barsLabel_, folderLabel_, nameLabel_,
        folder_, extension_;
    juce::TextButton choose_, go_, stop_;
    std::vector<int> rules_;
    juce::Rectangle<int> receipt_;
    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BounceWindow)
};

inline juce::String bounceStem(const juce::String& patchPath, juce::Time when) {
    const auto named = juce::File(patchPath).getFileNameWithoutExtension();
    return (named.isNotEmpty() ? named : juce::String("Untitled"))
           + when.formatted("_%Y%m%d-%H%M");
}

inline bool videoIsLive(EngineHost& host, const std::string& node, int depth = 0) {
    if (node.empty() || depth > 16) return true;
    if (dynamic_cast<CamPreviewSource*>(host.liveOrganism(node)) != nullptr) return true;
    bool fed = false;
    for (const auto& c : host.model().videoConnections)
        if (c.dst == node) {
            fed = true;
            if (!videoIsLive(host, c.src, depth + 1)) return false;
        }
    return fed;
}

inline BounceOffer bounceOffer(EngineHost& host) {
    BounceOffer offer;
    for (const auto& cm : host.model().organisms) {
        if (!isVideoOutputNode(host, cm.name)) continue;
        if (host.videoSourceInto(cm.name, 0).empty()) continue;
        if (videoIsLive(host, cm.name)) {
            offer.cameraOnly = true;
            continue;
        }
        offer.videoNodes.push_back(cm.name);
    }
    if (!offer.videoNodes.empty()) offer.cameraOnly = false;
    const auto master = host.masterOutputName();
    for (const auto& c : host.model().connections)
        if (c.dst == master) offer.hasSound = true;
    for (const auto& cm : host.model().organisms)
        for (const auto& ch : cm.pattern.channels)
            if (ch.type == "note-events") offer.hasNotes = true;
    offer.tempo = host.tempo();
    offer.songEndBeat = host.songEndBeat();
    return offer;
}

}
