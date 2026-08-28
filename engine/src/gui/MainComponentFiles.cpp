#include "gui/MainComponent.h"

#include "core/AppPaths.h"

#include "gui/AppSettings.h"
#include "gui/Telemetry.h"
#include "gui/TelemetryEvents.h"
#include "io/PatchFormat.h"

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
    setStatus("new patch");
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
        setStatus("open failed: " + juce::String(err));
        return false;
    }
    currentFile_ = f.getFullPathName();
    rememberDir(f, false);
    recents::push(f);
    menuItemsChanged();
    canvas_->exitToScope("");
    canvas_->select("");
    propsPane_->syncFromModel();
    refreshTimelinePanes();
    canvas_->refresh();
    setStatus("opened " + f.getFileName() + strayWarning());
    return true;
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
        setStatus("saved " + juce::File(currentFile_).getFileName());
        telemetryCount(telemetry::kPatchSaved);
        if (onSaved) onSaved();
    } else {
        setStatus("save failed: " + juce::String(err));
    }
}

void MainComponent::savePatchAs(std::function<void()> onSaved) {
    chooser_ = std::make_unique<juce::FileChooser>("Save patch as", lastDir(true),
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
            setStatus("saved " + f.getFileName());
            if (onSaved) onSaved();
        } else {
            setStatus("save failed: " + juce::String(err));
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
    aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Don't Save", 2);
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
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

void MainComponent::exportSound() {
    chooser_ = std::make_unique<juce::FileChooser>("Export to sound file", lastDir(true), "*.wav");
    chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this](const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        if (!f.hasFileExtension("wav")) f = f.withFileExtension("wav");
        rememberDir(f, true);
        auto path = f.getFullPathName().toStdString();
        auto name = f.getFileName();

        const double guess = host_.songEndSeconds();
        auto* aw = new juce::AlertWindow("Export to Sound File",
                                         "Length to render (seconds):", juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor("secs", guess > 0.0 ? juce::String(guess, 2) : juce::String("10"));
        aw->addButton("Export", 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        aw->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, aw, path, name](int r) {
                if (r == 1) {
                    double secs = aw->getTextEditorContents("secs").getDoubleValue();
                    if (secs <= 0.0) secs = 10.0;
                    UiWatchdog::Suspend noStall(UiWatchdog::active());
                    std::string err;
                    if (host_.renderToFile(path, secs, err))
                        setStatus("exported " + name + " (" + juce::String(secs, 1) + "s)");
                    else
                        setStatus("export failed: " + juce::String(err));
                }
            }), true);
    });
}

void MainComponent::toggleMixRecording() {
    if (host_.isMixRecording()) {
        host_.stopMixRecording();
        setStatus("mix recording saved");
        return;
    }
    chooser_ = std::make_unique<juce::FileChooser>("Record master mix to", lastDir(true), "*.wav");
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
            setStatus("recording master mix -> " + f.getFileName());
        else
            setStatus("mix record failed: " + juce::String(err));
    });
}

void MainComponent::revertPatch() {
    if (currentFile_.isEmpty()) return;
    UiWatchdog::Suspend noStall(UiWatchdog::active());
    std::string err;
    if (host_.loadFile(currentFile_.toStdString(), err)) {
        canvas_->exitToScope("");
    canvas_->select(""); propsPane_->syncFromModel(); canvas_->refresh();
        setStatus("reverted " + juce::File(currentFile_).getFileName() + strayWarning());
    } else {
        setStatus("revert failed: " + juce::String(err));
    }
}

}
