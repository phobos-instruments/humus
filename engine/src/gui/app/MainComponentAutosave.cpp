// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"
#include "gui/patcher/PatcherCanvas.h"
#include "gui/properties/PropertiesPane.h"

#include "gui/app/AppSettings.h"
#include "gui/common/Localisation.h"

namespace hum {

void MainComponent::autosaveTick() {
    if (--autosaveTicks_ > 0) return;
    const int intervalSec = AppSettings::instance().getInt("autosave.intervalSec", 60);
    constexpr int kTicksPerSec = 30;
    autosaveTicks_ = juce::jmax(1, intervalSec) * kTicksPerSec;
    if (intervalSec <= 0) return;
    if (!host_.isDirty() || host_.changeStamp() == lastAutosaveStamp_) return;
    performAutosave();
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
        setStatus(tr("main-autosave.recovery-failed", "recovery failed: ") + juce::String(err));
        return;
    }
    currentFile_ = r.originalPath;
    host_.markDirty();
    AutosaveStore::discard(r);
    canvas_->exitToScope("");
    canvas_->select("");
    propsPane_->syncFromModel();
    refreshTimelinePanes();
    canvas_->refresh();
    setStatus(tr("main-autosave.recovered-unsaved-session", "recovered unsaved session")
              + (r.originalPath.isEmpty() ? juce::String()
                                          : " (" + juce::File(r.originalPath).getFileName() + ")"));
}

}
