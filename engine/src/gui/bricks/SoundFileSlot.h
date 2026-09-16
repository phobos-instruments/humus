// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/app/AppPaths.h"
#include "core/timeline/ClipDrag.h"
#include "core/packs/Catalogue.h"
#include "core/library/UserLibrary.h"
#include "gui/host/BrickHost.h"
#include "gui/host/EngineHostClips.h"
#include "io/PatchDocument.h"
#include "gui/style/IconButton.h"
#include "gui/style/LookAndFeel.h"
#include "gui/app/StartDirs.h"
#include "gui/common/UiTicker.h"
#include "gui/common/Localisation.h"
#include "io/MediaRefs.h"

namespace hum {

class SoundFileSlot : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      public juce::DragAndDropTarget {
public:
    SoundFileSlot(BrickHost& host, std::string organism, std::string param,
                  std::string wildcard = {}, std::string title = {},
                  std::string kind = {}, bool saving = false)
        : host_(host), name_(std::move(organism)), param_(std::move(param)),
          wildcard_(std::move(wildcard)), title_(std::move(title)),
          kind_(std::move(kind)), saving_(saving),
          chooseBtn_(IconButton::Glyph::Open,
                     !title_.empty() ? juce::String(juce::CharPointer_UTF8(title_.c_str()))
                     : saving_ ? juce::String(tr("sound-file-slot.name-a-sound-file-to-write",
                                                 "Name a sound file to write..."))
                               : juce::String(tr("sound-file-slot.select-a-sound-file",
                                                 "Select a sound file..."))) {
        addAndMakeVisible(nameField_);

        chooseBtn_.onClick = [this] {
            juce::Logger::writeToLog("file slot " + juce::String(name_) + "/" + juce::String(param_) + ": browse clicked");
            choose();
        };
        addAndMakeVisible(chooseBtn_);

        revealBtn_.setTooltip(tr("sound-file-slot.show-the-file-in-its-folder",
                                 "Show the file in its folder"));
        revealBtn_.onClick = [this] {
            const auto f = fileFor(host_.liveParamText(name_, param_));
            juce::Logger::writeToLog("file slot " + juce::String(name_) + "/"
                                     + juce::String(param_) + ": reveal " + f.getFullPathName());
            if (f.existsAsFile()) f.revealToUser();
            else if (f.getParentDirectory().isDirectory()) f.getParentDirectory().revealToUser();
        };
        addChildComponent(revealBtn_);

        clearBtn_.setButtonText(juce::String::charToString(juce::juce_wchar(0x00d7)));
        clearBtn_.setTooltip(tr("sound-file-slot.clear-this-slot", "Clear this slot"));
        clearBtn_.onClick = [this] {
            host_.setParamText(name_, param_, "");
            refresh();
        };
        addChildComponent(clearBtn_);

        tickerId_ = UiTicker::instance().add([this] { nameField_.tick(); });
        nameField_.addMouseListener(this, false);
        refresh();
    }

    void chooseForTest() { choose(); }
    bool savingForTest() const { return saving_; }
    bool revealShowingForTest() const { return revealBtn_.isVisible(); }
    juce::String shownForTest() const { return shown_; }
    bool chooserOpenForTest() const { return chooser_ != nullptr; }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) return;
        const juce::String cur = stripScheme(host_.liveParamText(name_, param_));
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
                host_.setParamText(name_, param_, target.getFullPathName().toStdString());
                refresh();
            }
        });
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging_ || e.mods.isPopupMenu() || e.getDistanceFromDragStart() < 10) return;
        const juce::File f(stripScheme(host_.liveParamText(name_, param_)));
        if (!f.existsAsFile()) return;
        dragging_ = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            {f.getFullPathName()}, false, this, [this] { dragging_ = false; });
    }

    ~SoundFileSlot() override { UiTicker::instance().remove(tickerId_); }

    juce::File kindDir() const {
        return userContentRoot().getChildFile(kind_.empty() ? "Samples" : kind_);
    }

    juce::File startDir() const {
        const auto mine = kindDir();
        if (mine.isDirectory()
            && !mine.findChildFiles(juce::File::findFilesAndDirectories, false,
                                    "*").isEmpty())
            return mine;
        const auto path = assetSearchPath(kind_.empty() ? "Samples" : kind_);
        for (const auto& d : path)
            if (d != mine && d.isDirectory()) return d;
        return mine;
    }

    void refresh() {
        const std::string raw = host_.liveParamText(name_, param_);
        juce::String s = stripScheme(raw);
        const bool missing = !saving_ && s.isNotEmpty() && !fileFor(raw).exists();
        const bool awaited = saving_ && s.isNotEmpty() && !fileFor(raw).exists();
        shown_ = s.isEmpty() ? juce::String("(no file)")
               : missing ? tr("sound-file-slot.missing", "missing: ") + juce::File(s).getFileName()
                         : juce::File(s).getFileName();
        nameField_.setMissing(missing);
        nameField_.setText(shown_);
        nameField_.setSwatch(s.isNotEmpty() && !missing ? swatch_ : juce::Colour());
        nameField_.setTooltip(
            missing ? tr("sound-file-slot.missing-tip",
                         "Not found - File > Locate Missing Media, or pick it again here: ") + s
            : awaited ? tr("sound-file-slot.written-here", "The take will be written here: ") + s
                      : s);
        clearBtn_.setVisible(s.isNotEmpty());
        const auto here = fileFor(s.toStdString());
        revealBtn_.setVisible(s.isNotEmpty()
                              && (here.existsAsFile() || here.getParentDirectory().isDirectory()));
    }

    juce::File fileFor(const std::string& raw) const {
        const auto* cm = host_.model().byName(name_);
        return media::resolveRef(raw, cm ? cm->displayClass : std::string(), host_.documentDir());
    }

    juce::String shownName() const { return shown_; }

    void setSwatch(juce::Colour c) {
        swatch_ = c;
        refresh();
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        if (files.size() != 1) return false;
        const juce::File f(files[0]);
        const auto wc = acceptWildcard();
        for (auto& pat : juce::StringArray::fromTokens(wc, ";", {}))
            if (f.getFileName().matchesWildcard(pat.trim(), true)) return true;
        return false;
    }
    void filesDropped(const juce::StringArray& files, int, int) override {
        if (files.isEmpty()) return;
        host_.setParamText(name_, param_, juce::File(files[0]).getFullPathName().toStdString());
        refresh();
    }

    bool isInterestedInDragSource(const SourceDetails& d) override {
        return draggedClipFile(d).existsAsFile();
    }
    void itemDropped(const SourceDetails& d) override {
        const auto f = draggedClipFile(d);
        if (!f.existsAsFile()) return;
        host_.setParamText(name_, param_, f.getFullPathName().toStdString());
        refresh();
    }
    juce::File draggedClipFile(const SourceDetails& d) {
        std::string track;
        int clipId = 0;
        if (!clipdrag::parseVideoClip(d.description.toString().toStdString(), track, clipId))
            return {};
        const auto r = host_.clips().rangeOf(track, clipId);
        if (r.file.empty()) return {};
        const juce::File f(juce::String(juce::CharPointer_UTF8(r.file.c_str())));
        return isInterestedInFileDrag({f.getFullPathName()}) ? f : juce::File();
    }

    void resized() override {
        auto r = getLocalBounds();
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

    static juce::String stripScheme(const std::string& uri) {
        juce::String s(juce::CharPointer_UTF8(uri.c_str()));
        return s.startsWith("file://") ? s.substring(7) : s;
    }

    void choose() {
        juce::String cur = stripScheme(host_.liveParamText(name_, param_));
        juce::File start = cur.isNotEmpty()
                               ? juce::File(cur).getParentDirectory()
                           : saving_
                               ? startDirFor(DirPurpose::Recording)
                               : juce::File::getSpecialLocation(juce::File::userMusicDirectory);
        if (cur.isEmpty() && !saving_)
            if (const auto d = startDir(); d.isDirectory()) start = d;
        juce::String wc(juce::CharPointer_UTF8(wildcard_.c_str()));
        if (wc.isEmpty() && !kind_.empty())
            if (const auto* k = catalogue::find(kind_))
                wc = juce::String(juce::CharPointer_UTF8(k->wildcard.c_str()));
        if (wc.isEmpty()) {

            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            wc = fm.getWildcardForAllFormats();
        }
        chooser_ = std::make_unique<juce::FileChooser>(
            !title_.empty() ? juce::String(juce::CharPointer_UTF8(title_.c_str()))
            : saving_ ? juce::String(tr("sound-file-slot.name-a-sound-file-to-write-2",
                                        "Name a sound file to write"))
                      : juce::String(tr("sound-file-slot.select-a-sound-file-2",
                                        "Select a sound file")),
            start, wc);
        int flags = juce::FileBrowserComponent::canSelectFiles;
        flags |= saving_ ? (juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::warnAboutOverwriting)
                         : juce::FileBrowserComponent::openMode;
        if (!saving_ && wc.contains("synScene"))
            flags |= juce::FileBrowserComponent::canSelectDirectories;
        juce::Logger::writeToLog("file slot " + juce::String(name_) + "/" + juce::String(param_)
                                 + ": opening the chooser at " + start.getFullPathName());
        chooser_->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto f = fc.getResult();
            juce::Logger::writeToLog("file slot " + juce::String(name_) + "/" + juce::String(param_)
                                     + ": chooser returned '" + f.getFullPathName() + "'");
            if (f == juce::File()) return;
            if (saving_ && f.getFileExtension().isEmpty()) f = f.withFileExtension(".wav");
            if (saving_) rememberDirFor(DirPurpose::Recording, f);
            host_.setParamText(name_, param_, f.getFullPathName().toStdString());
            refresh();
        });
    }

    juce::String acceptWildcard() const {
        if (!wildcard_.empty()) return juce::String(juce::CharPointer_UTF8(wildcard_.c_str()));
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        return fm.getWildcardForAllFormats();
    }

    std::string displayClass() const {
        const auto* cm = host_.model().byName(name_);
        return cm != nullptr ? cm->displayClass : std::string();
    }

    BrickHost& host_;
    std::string name_, param_;
    std::string wildcard_, title_, kind_;
    bool saving_ = false;
    NameField nameField_;
    juce::Colour swatch_;
    juce::String shown_;
    bool dragging_ = false;
    IconButton chooseBtn_;
    IconButton revealBtn_{IconButton::Glyph::Reveal,
                          juce::String(tr("sound-file-slot.show-the-file-in-its-folder",
                                          "Show the file in its folder"))};
    juce::TextButton clearBtn_;
    std::unique_ptr<juce::FileChooser> chooser_;
    int tickerId_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFileSlot)
};

}
