// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "core/timeline/ClipDrag.h"
#include "core/library/UserLibrary.h"
#include "gui/editor/files/SoundSlotModel.h"
#include "gui/editor/juce/JuceFilePicker.h"
#include "gui/editor/juce/JuceMediaLibrary.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostClips.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/UiTicker.h"
#include "gui/common/Localisation.h"

namespace hum {

class SoundFileSlot : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      public juce::DragAndDropTarget {
public:
    SoundFileSlot(BrickHost& host, std::string organism, std::string param,
                  std::string wildcard = {}, std::string title = {},
                  std::string kind = {}, bool saving = false)
        : host_(host), name_(organism), kind_(kind), media_(host),
          slot_(host, media_, organism, std::move(param), std::move(wildcard), title, std::move(kind), saving),
          chooseBtn_(IconButton::Glyph::Open,
                     !title.empty() ? juce::String(title)
                     : saving ? juce::String(tr("sound-file-slot.name-a-sound-file-to-write",
                                                "Name a sound file to write..."))
                              : juce::String(tr("sound-file-slot.select-a-sound-file",
                                                "Select a sound file..."))) {
        addAndMakeVisible(nameField_);

        chooseBtn_.onClick = [this] {
            juce::Logger::writeToLog("file slot " + juce::String(name_) + "/" + juce::String(slot_.param()) + ": browse clicked");
            choose();
        };
        addAndMakeVisible(chooseBtn_);

        revealBtn_.setTooltip(tr("sound-file-slot.show-the-file-in-its-folder",
                                 "Show the file in its folder"));
        revealBtn_.onClick = [this] {
            const auto f = juceFileAt(slot_.resolved(slot_.raw()));
            juce::Logger::writeToLog("file slot " + juce::String(name_) + "/"
                                     + juce::String(slot_.param()) + ": reveal " + f.getFullPathName());
            if (f.existsAsFile()) f.revealToUser();
            else if (f.getParentDirectory().isDirectory()) f.getParentDirectory().revealToUser();
        };
        addChildComponent(revealBtn_);

        clearBtn_.setButtonText(juce::String::charToString(juce::juce_wchar(0x00d7)));
        clearBtn_.setTooltip(tr("sound-file-slot.clear-this-slot", "Clear this slot"));
        clearBtn_.onClick = [this] {
            slot_.clear();
            refresh();
        };
        addChildComponent(clearBtn_);

        tickerId_ = UiTicker::instance().add([this] { nameField_.tick(); });
        nameField_.addMouseListener(this, false);
        refresh();
    }

    void chooseForTest() { choose(); }
    const files::SoundSlotModel& model() const { return slot_; }
    bool revealShowingForTest() const { return revealBtn_.isVisible(); }
    juce::String shownForTest() const { return shown_; }
    bool chooserOpenForTest() const { return picker_.open(); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) return;
        const juce::String cur = juce::String::fromUTF8(files::stripScheme(slot_.raw()).c_str());
        if (cur.isEmpty() || !juce::File(cur).existsAsFile()) return;
        const juce::File f(cur);
        if (f.isAChildOf(library::root())) return;
        juce::PopupMenu m;
        m.addItem(1, tr("sound-file-slot.add-to-library", "Add to Library"));
        m.showMenuAsync(juce::PopupMenu::Options(), [this, f](int r) {
            if (r != 1) return;
            const auto dir = kindDir();
            dir.createDirectory();
            auto target = dir.getChildFile(f.getFileName());
            if (target.existsAsFile() && target.getSize() != f.getSize())
                target = target.getNonexistentSibling();
            if (target.existsAsFile() || f.copyFileTo(target)) {
                slot_.take(target.getFullPathName().toStdString());
                refresh();
            }
        });
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging_ || e.mods.isPopupMenu() || e.getDistanceFromDragStart() < 10) return;
        const auto f = juceFileAt(files::stripScheme(slot_.raw()));
        if (!f.existsAsFile()) return;
        dragging_ = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            {f.getFullPathName()}, false, this, [this] { dragging_ = false; });
    }

    ~SoundFileSlot() override { UiTicker::instance().remove(tickerId_); }

    juce::File kindDir() const { return kindDirFor(kind_); }

    void refresh() {
        const auto shown = slot_.shown();
        shown_ = juce::String::fromUTF8(shown.text.c_str());
        nameField_.setMissing(shown.missing);
        nameField_.setText(shown_);
        nameField_.setSwatch(shown.clearable && !shown.missing ? swatch_ : juce::Colour());
        nameField_.setTooltip(juce::String::fromUTF8(shown.tooltip.c_str()));
        clearBtn_.setVisible(shown.clearable);
        revealBtn_.setVisible(shown.revealable);
    }

    juce::String shownName() const { return shown_; }
    bool stackedForTest() const { return stacked_; }

    void offersVideos(files::SoundSlotModel::VideoOffer m) { slot_.offersVideos(m); }

    void setSwatch(juce::Colour c) {
        swatch_ = c;
        refresh();
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        return files.size() == 1 && slot_.accepts(files[0].toStdString());
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        if (files.isEmpty()) return;
        slot_.take(juce::File(files[0]).getFullPathName().toStdString());
        refresh();
    }

    bool isInterestedInDragSource(const SourceDetails& d) override {
        return draggedClipFile(d).existsAsFile();
    }
    void itemDropped(const SourceDetails& d) override {
        const auto f = draggedClipFile(d);
        if (!f.existsAsFile()) return;
        slot_.take(f.getFullPathName().toStdString());
        refresh();
    }
    juce::File draggedClipFile(const SourceDetails& d) {
        std::string track;
        int clipId = 0;
        if (!clipdrag::parseVideoClip(d.description.toString().toStdString(), track, clipId))
            return {};
        const auto r = host_.clips().rangeOf(track, clipId);
        if (r.file.empty()) return {};
        const auto f = fileAt(r.file);
        return isInterestedInFileDrag({f.getFullPathName()}) ? f : juce::File();
    }

    void setStacked(bool on) {
        if (on == stacked_) return;
        stacked_ = on;
        resized();
    }

    static constexpr int kIconRowH = 20;

    void resized() override {
        auto r = getLocalBounds();
        if (stacked_) {
            auto icons = r.removeFromBottom(kIconRowH);
            nameField_.setBounds(r.withTrimmedBottom(2));
            const int side = juce::jmin(icons.getHeight(), 18);
            icons = icons.withSizeKeepingCentre(icons.getWidth(), side);
            chooseBtn_.setBounds(icons.removeFromLeft(side));
            icons.removeFromLeft(3);
            revealBtn_.setBounds(icons.removeFromLeft(side));
            clearBtn_.setBounds(icons.removeFromRight(side));
            return;
        }
        clearBtn_.setBounds(r.removeFromRight(r.getHeight()));
        r.removeFromRight(2);
        chooseBtn_.setBounds(r.removeFromRight(r.getHeight()));
        r.removeFromRight(2);
        revealBtn_.setBounds(r.removeFromRight(r.getHeight()));
        r.removeFromRight(4);
        nameField_.setBounds(r);
    }

private:

    class NameField : public juce::Component, public juce::SettableTooltipClient {
    public:
        void setSwatch(juce::Colour c) {
            if (c == swatch_) return;
            swatch_ = c;
            repaint();
        }
        void setMissing(bool on) {
            if (on == missing_) return;
            missing_ = on;
            repaint();
        }
        void setText(const juce::String& t) {
            if (t == text_) return;
            text_ = t;
            offset_ = 0.0f;
            repaint();
        }

        void tick() {
            if (!overflow_ || isMouseOver() || !isShowing()) return;
            offset_ += 0.6f;
            const float wrap = textW_ + kGap;
            if (offset_ >= wrap) offset_ -= wrap;
            repaint();
        }

        void paint(juce::Graphics& g) override {
            g.fillAll(Palette::panel.darker(0.3f));
            const juce::Font f(juce::FontOptions(11.0f));
            textW_ = juce::GlyphArrangement::getStringWidth(f, text_);
            auto area = getLocalBounds();
            if (!swatch_.isTransparent()) {
                g.setColour(swatch_);
                g.fillRect(area.removeFromLeft(kSwatchW));
            }
            area.reduce(kPad, 0);
            overflow_ = textW_ > (float) area.getWidth();
            g.setColour(missing_ ? Palette::warnAmber() : Palette::text);
            g.setFont(f);
            if (!overflow_) {
                offset_ = 0.0f;
                g.drawText(text_, area, juce::Justification::centredLeft, false);
                return;
            }
            g.reduceClipRegion(area);
            const float y = (float) getHeight();
            const float x0 = (float) area.getX() - offset_;
            g.drawSingleLineText(text_, (int) x0, (int) (y * 0.5f + f.getAscent() * 0.5f) - 1);
            g.drawSingleLineText(text_, (int) (x0 + textW_ + kGap),
                                 (int) (y * 0.5f + f.getAscent() * 0.5f) - 1);
        }

        void mouseEnter(const juce::MouseEvent&) override { repaint(); }

    private:
        static constexpr int kPad = 4;
        static constexpr int kSwatchW = 4;
        static constexpr float kGap = 28.0f;
        juce::Colour swatch_;
        juce::String text_;
        float textW_ = 0.0f, offset_ = 0.0f;
        bool overflow_ = false;
        bool missing_ = false;
    };

    void choose() {
        const auto request = slot_.request();
        juce::Logger::writeToLog("file slot " + juce::String(name_) + "/" + juce::String(slot_.param())
                                 + ": opening the chooser at " + juceFileAt(request.startDir).getFullPathName());
        picker_.pick(request, [this](const std::vector<std::string>& paths) {
            juce::Logger::writeToLog("file slot " + juce::String(name_) + "/" + juce::String(slot_.param())
                                     + ": chooser returned '"
                                     + juce::String::fromUTF8(paths.empty() ? "" : paths.front().c_str()) + "'");
            if (slot_.chosen(paths)) refresh();
        });
    }

    BrickHost& host_;
    std::string name_, kind_;
    JuceMediaLibrary media_;
    files::SoundSlotModel slot_;
    NameField nameField_;
    juce::Colour swatch_;
    juce::String shown_;
    bool dragging_ = false;
    bool stacked_ = false;
    IconButton chooseBtn_;
    IconButton revealBtn_{IconButton::Glyph::Reveal,
                          juce::String(tr("sound-file-slot.show-the-file-in-its-folder",
                                          "Show the file in its folder"))};
    juce::TextButton clearBtn_;
    JuceFilePicker picker_;
    int tickerId_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFileSlot)
};

}
