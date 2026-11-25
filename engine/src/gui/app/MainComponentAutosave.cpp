// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"

#include <cstdint>

#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"

#include "gui/app/AppSettings.h"
#include "core/library/ExamplesTree.h"
#include "gui/common/Localisation.h"

namespace hum {

void MainComponent::autosaveTick(std::int64_t now) {
    history_.tick(currentFile_, now);
    autosaveInPlaceTick(now);
    if (--autosaveTicks_ > 0) return;
    const int intervalSec = AppSettings::instance().getInt("autosave.intervalSec", 60);
    constexpr int kTicksPerSec = 30;
    autosaveTicks_ = juce::jmax(1, intervalSec) * kTicksPerSec;
    if (intervalSec <= 0) return;
    if (!host_.isDirty() || host_.changeStamp() == lastAutosaveStamp_) return;
    performAutosave();
}

bool MainComponent::autosaveInPlaceOn() const {
    return AppSettings::instance().getInt(AutosaveInPlace::kSettingKey, 0) != 0;
}

void MainComponent::toggleAutosaveInPlace() {
    AppSettings::instance().set(AutosaveInPlace::kSettingKey, autosaveInPlaceOn() ? 0 : 1);
    menuItemsChanged();
}

void MainComponent::autosaveInPlaceTick(std::int64_t now) {
    if (!autosaveInPlaceOn()) return;
    const AutosaveInPlace::Moment moment{
        currentFile_.isNotEmpty(), host_.isDirty(),
        host_.isPlaying() || host_.isMixRecording() || host_.record().armed(), host_.changeStamp()};
    if (!inPlace_.due(moment, now)) return;
    const juce::File patch(currentFile_);
    if (!AutosaveInPlace::canWrite(patch, examples::roots(),
                                   juce::File::getSpecialLocation(juce::File::currentApplicationFile))) {
        inPlace_.failed(moment.stamp);
        return;
    }
    UiWatchdog::Suspend noStall(UiWatchdog::active());
    std::string err;
    if (host_.saveFile(currentFile_.toStdString(), err)) {
        clearAutosave();
        return;
    }
    inPlace_.failed(moment.stamp);
    notifyError(tr("main-autosave.autosave-failed", "autosave failed: {reason}", {{"reason", juce::String(err)}}));
}

void MainComponent::performAutosave() {
    autosave_.autosaveFile().getParentDirectory().createDirectory();
    std::string err;
    if (host_.saveCopy(autosave_.autosaveFile().getFullPathName().toStdString(), err)) {
        autosave_.writeMeta(currentFile_);
        lastAutosaveStamp_ = host_.changeStamp();
    }
}

void MainComponent::clearAutosave() {
    autosave_.clear();
    lastAutosaveStamp_ = host_.changeStamp();
}

void MainComponent::restoreAutosave(const AutosaveStore::Recovery& r) {
    std::string err;
    if (!host_.loadFile(r.autosave.getFullPathName().toStdString(), err)) {
        notifyError(tr("main-autosave.recovery-failed", "recovery failed: {reason}", {{"reason", juce::String(err)}}));
        return;
    }
    currentFile_ = r.originalPath;
    host_.markDirty();
    history_.begin(currentFile_, false, nowMs());
    AutosaveStore::discard(r);
    canvas_->exitToScope("");
    canvas_->select("");
    propsPane_->syncFromModel();
    refreshTimelinePanes();
    canvas_->refresh();
    notify(r.originalPath.isEmpty()
               ? tr("main-autosave.recovered-unsaved-session", "recovered unsaved session")
               : tr("main-autosave.recovered-unsaved-session-of", "recovered unsaved session ({name})",
                    {{"name", juce::File(r.originalPath).getFileName()}}));
}

}
