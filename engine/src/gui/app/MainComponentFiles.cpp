// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"
#include "gui/app/RecentFiles.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"
#include "gui/tracks/TracksPane.h"
#include "gui/app/BounceJob.h"
#include "gui/app/BounceWindow.h"
#include "gui/app/BounceWants.h"
#include "gui/app/MissingMediaDialog.h"

#include "core/app/AppPaths.h"

#include "gui/app/StartDirs.h"

#include "gui/app/AppSettings.h"
#include "gui/app/Telemetry.h"
#include "gui/app/TelemetryEvents.h"
#include "io/PatchFormat.h"
#include "gui/common/Localisation.h"

namespace hum {

juce::File MainComponent::lastDir(bool forSave) const {
    auto& s = AppSettings::instance();
    const juce::String key = forSave ? "lastDir.save" : "lastDir.open";
    const juce::String alt = forSave ? "lastDir.open" : "lastDir.save";
    juce::File d(s.getString(key, ""));
    if (d.isDirectory()) return d;
    juce::File other(s.getString(alt, ""));
    if (other.isDirectory()) return other;
    juce::File legacy(s.getString("lastDir", ""));
    if (legacy.isDirectory()) return legacy;
    const auto home = userPatchesDir();
    home.createDirectory();
    return home.isDirectory() ? home
                              : juce::File::getSpecialLocation(juce::File::userHomeDirectory);
}

void MainComponent::rememberDir(const juce::File& chosen, bool forSave) {
    auto dir = chosen.isDirectory() ? chosen : chosen.getParentDirectory();
    if (!dir.isDirectory()) return;
    AppSettings::instance().set(forSave ? "lastDir.save" : "lastDir.open",
                                dir.getFullPathName());
}

void MainComponent::newPatch() {
    confirmDiscardThenRun([this] { newPatchImpl(); });
}

void MainComponent::newPatchImpl() {
    host_.newDocument(canvas_->visibleCentreForNode());
    currentFile_.clear();
    canvas_->exitToScope("");
    canvas_->select("");
    propsPane_->clearAll();
    if (tracksPane_) tracksPane_->rebuild();
    canvas_->refresh();
    setStatus(tr("main-files.new-patch", "new patch"));
}

void MainComponent::openPatch() {
    openPatchImpl();
}

void MainComponent::openPatchImpl() {
    chooser_ = std::make_unique<juce::FileChooser>("Open patch", lastDir(false), kPatchOpenFilter);
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this](const juce::FileChooser& fc) {
        const auto f = fc.getResult();
        if (f == juce::File()) return;
        confirmDiscardThenRun([this, f] { openFileAt(f); });
    });
}

bool MainComponent::openFileAt(const juce::File& f) {
    UiWatchdog::Suspend noStall(UiWatchdog::active());
    std::string err;
    if (!host_.loadFile(f.getFullPathName().toStdString(), err)) {
        setStatus(tr("main-files.open-failed", "open failed: ") + juce::String(err));
        return false;
    }
    currentFile_ = f.getFullPathName();
    int unavailable = 0;
    for (const auto& cm : host_.model().organisms)
        if (!host_.missingClassNote(cm.name).empty()) ++unavailable;
    juce::Logger::writeToLog("opened " + f.getFullPathName() + " with "
                             + juce::String(host_.model().organisms.size()) + " boxes, "
                             + juce::String(unavailable) + " unavailable");
    if (!host_.model().migrationNotes.empty()) {
        const auto& notes = host_.model().migrationNotes;
        for (const auto& n : notes) juce::Logger::writeToLog("migrated: " + juce::String(n));
        setStatus(juce::String((int) notes.size())
                  + tr("main-files.migrated", " cord(s) from an older patch were converted or dropped, see the log")
                  + " - " + juce::String(notes.front()));
    } else if (host_.model().newerFormat)
        setStatus(tr("main-files.newer-format",
                     "This patch was saved by a newer Humus; what this version does not understand will not survive a save"));
    else if (unavailable > 0)
        setStatus(juce::String(unavailable)
                  + tr("main-files.unavailable-boxes",
                       " box(es) in this patch are unavailable here and pass sound straight through"));
    rememberDir(f, false);
    recents::push(f);
    menuItemsChanged();
    canvas_->exitToScope("");
    canvas_->select("");
    propsPane_->syncFromModel();
    refreshTimelinePanes();
    canvas_->refresh();
    setStatus(tr("main-files.opened", "opened ") + f.getFileName() + strayWarning());
    if (const auto missing = host_.missingMedia(); !missing.empty()) {
        setStatus(missingmedia::statusLine((int) missing.size()));
        locateMissingMedia();
    }
    return true;
}

void MainComponent::locateMissingMedia() {
    if (host_.missingMedia().empty()) {
        setStatus(tr("main-files.no-missing-media", "every media file this patch uses is where it should be"));
        return;
    }
    missingmedia::show(host_, [this] {
        propsPane_->syncFromModel();
        refreshTimelinePanes();
        canvas_->refresh();
        const int left = (int) host_.missingMedia().size();
        setStatus(left == 0 ? tr("main-files.media-located", "media located")
                            : missingmedia::statusLine(left));
    });
}

juce::String MainComponent::strayWarning() const {
    const int n = pods::strayTotal(const_cast<EngineHost&>(host_).model());
    if (n == 0) return {};
    return juce::String("  -  ") + juce::String(n) + (n == 1 ? " cord" : " cords")
           + " reach past a pod boundary (amber pins)";
}

void MainComponent::savePatch(std::function<void()> onSaved) {
    if (currentFile_.isEmpty()) { savePatchAs(std::move(onSaved)); return; }
    UiWatchdog::Suspend noStall(UiWatchdog::active());
    std::string err;
    if (host_.saveFile(currentFile_.toStdString(), err)) {
        clearAutosave();
        setStatus(tr("main-files.saved", "saved ") + juce::File(currentFile_).getFileName());
        telemetryCount(telemetry::kPatchSaved);
        if (onSaved) onSaved();
    } else {
        setStatus(tr("main-files.save-failed", "save failed: ") + juce::String(err));
    }
}

void MainComponent::savePatchAs(std::function<void()> onSaved) {
    chooser_ = std::make_unique<juce::FileChooser>(tr("main-files.save-patch-as", "Save patch as"), lastDir(true),
                                                   juce::String("*.") + kPatchExt);
    chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, onSaved](const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        if (!f.hasFileExtension(kPatchExt) && !f.hasFileExtension(kLegacyPatchExt))
            f = f.withFileExtension(kPatchExt);
        UiWatchdog::Suspend noStall(UiWatchdog::active());
        std::string err;
        if (host_.saveFile(f.getFullPathName().toStdString(), err)) {
            currentFile_ = f.getFullPathName();
            clearAutosave();
            rememberDir(f, true);
            recents::push(f);
            menuItemsChanged();
            setStatus(tr("main-files.saved", "saved ") + f.getFileName());
            if (onSaved) onSaved();
        } else {
            setStatus(tr("main-files.save-failed", "save failed: ") + juce::String(err));
        }
    });
}

void MainComponent::confirmDiscardThenRun(std::function<void()> action) {
    if (!host_.isDirty()) { action(); return; }
    const juce::String name = currentFile_.isEmpty() ? juce::String("this patch")
                                                       : juce::File(currentFile_).getFileName();
    auto* aw = new juce::AlertWindow("Unsaved Changes",
                                     "Save changes to " + name + " before continuing?",
                                     juce::MessageBoxIconType::QuestionIcon);
    aw->addButton(tr("main-files.save", "Save"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton(tr("main-files.don-t-save", "Don't Save"), 2);
    aw->addButton(tr("main-files.cancel", "Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, action](int r) {
            if (r == 1) savePatch(action);
            else if (r == 2) action();
        }), true);
}

void MainComponent::closeProject() {
    if (!onCloseProject) return;
    confirmDiscardThenRun([this] { onCloseProject(); });
}

void MainComponent::toggleMixRecording() {
    if (host_.isMixRecording()) {
        host_.stopMixRecording();
        setStatus(tr("main-files.mix-recording-saved", "mix recording saved"));
        return;
    }
    chooser_ = std::make_unique<juce::FileChooser>(tr("main-files.record-live-performance-to", "Record live performance to"), lastDir(true), "*.wav");
    chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this](const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        if (!f.hasFileExtension("wav")) f = f.withFileExtension("wav");
        rememberDir(f, true);
        ensureAudio();
        std::string err;
        if (host_.startMixRecording(f.getFullPathName().toStdString(), err))
            setStatus(tr("main-files.recording-master-mix", "recording master mix -> ") + f.getFileName());
        else
            setStatus(tr("main-files.mix-record-failed", "mix record failed: ") + juce::String(err));
    });
}

void MainComponent::revertPatch() {
    if (currentFile_.isEmpty()) return;
    UiWatchdog::Suspend noStall(UiWatchdog::active());
    std::string err;
    if (host_.loadFile(currentFile_.toStdString(), err)) {
        canvas_->exitToScope("");
    canvas_->select(""); propsPane_->syncFromModel(); canvas_->refresh();
        setStatus(tr("main-files.reverted", "reverted ") + juce::File(currentFile_).getFileName() + strayWarning());
    } else {
        setStatus(tr("main-files.revert-failed", "revert failed: ") + juce::String(err));
    }
}

void MainComponent::bounce() {
    auto offer = bounceOffer(host_);
    if (host_.automation().loopEnabled()) {
        offer.loopFrom = host_.automation().loopStartBeat();
        offer.loopTo = host_.automation().loopEndBeat();
    }
    if (double from = 0.0, to = 0.0;
        tracksPane_ != nullptr && tracksPane_->timeSelection(from, to)) {
        offer.selectFrom = from;
        offer.selectTo = to;
    }
    offer.folder = startDirFor(DirPurpose::Recording);
    offer.stem = bounceStem(currentFile_, juce::Time::getCurrentTime());

    auto content = std::make_unique<BounceWindow>(offer);
    content->onBounce = [this](const BounceWants& wants) {
        closeBounceWindow();
        juce::MessageManager::callAsync([this, wants] { startBounce(wants); });
    };
    content->onCancel = [this] { closeBounceWindow(); };
    bounceWindow_.reset(content.get());
    juce::DialogWindow::LaunchOptions o;
    o.content.setNonOwned(content.release());
    o.dialogTitle = tr("main-files.bounce", "Bounce");
    o.dialogBackgroundColour = Palette::background;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    o.resizable = false;
    o.launchAsync();
}

void MainComponent::closeBounceWindow() {
    if (bounceWindow_ != nullptr)
        if (auto* dw = bounceWindow_->findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState(0);
    bounceWindow_.reset();
}

void MainComponent::startBounce(const BounceWants& wants) {
    if (host_.isPlaying()) host_.stop();
    if (wants.folder.isDirectory()) rememberDirFor(DirPurpose::Recording, wants.folder);
    bounce_ = std::make_unique<BounceJob>(host_, wants);
    bounce_->run([this](BounceJob::Told told) {
        setStatus(told.ok ? told.said
                          : tr("main-files.bounce-failed", "bounce failed: ") + told.said);
        juce::MessageManager::callAsync([this] { bounce_.reset(); });
    });
}

}
