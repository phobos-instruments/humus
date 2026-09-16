// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/AutosaveStore.h"

#include "io/PatchFormat.h"

#include "core/app/AppPaths.h"
#include "core/plugins/ProcessId.h"

namespace hum {

AutosaveStore::AutosaveStore(juce::File dir, int pid) : dir_(std::move(dir)), pid_(pid) {}

juce::File AutosaveStore::defaultDir() {
    return appDataDir().getChildFile("autosave");
}

int AutosaveStore::currentPid() { return currentProcessId(); }

bool AutosaveStore::pidAlive(int pid) { return processAlive(pid); }

juce::File AutosaveStore::autosaveFile() const {
    return dir_.getChildFile("session-" + juce::String(pid_) + "." + kPatchExt);
}

juce::File AutosaveStore::metaFileFor(const juce::File& autosave) const {
    return autosave.withFileExtension("meta.xml");
}

void AutosaveStore::writeMeta(const juce::String& originalPath) {
    dir_.createDirectory();
    juce::XmlElement xml("autosave-session");
    xml.setAttribute("original", originalPath);
    xml.setAttribute("saved-at",
                     juce::String(juce::Time::getCurrentTime().toMilliseconds()));
    xml.writeTo(metaFileFor(autosaveFile()));
}

void AutosaveStore::clear() {
    autosaveFile().deleteFile();
    metaFileFor(autosaveFile()).deleteFile();
}

std::vector<AutosaveStore::Recovery> AutosaveStore::findOrphans() const {
    std::vector<Recovery> out;
    const juce::String pattern = juce::String("session-*.") + kPatchExt
                               + ";session-*." + kLegacyPatchExt;
    for (const auto& f : dir_.findChildFiles(juce::File::findFiles, false, pattern)) {
        const int pid = f.getFileNameWithoutExtension().fromLastOccurrenceOf("-", false, false)
                            .getIntValue();
        if (pid <= 0 || pidAlive(pid)) continue;

        Recovery r;
        r.autosave = f;
        r.meta = metaFileFor(f);
        if (auto xml = juce::parseXML(r.meta)) {
            r.originalPath = xml->getStringAttribute("original");
            r.savedAt = juce::Time((juce::int64) xml->getDoubleAttribute("saved-at"));
        }

        const juce::File original(r.originalPath);
        const bool untitled = r.originalPath.isEmpty();
        const bool worthIt = untitled || !original.existsAsFile()
                          || (f.getLastModificationTime() > original.getLastModificationTime()
                              && !f.hasIdenticalContentTo(original));
        if (worthIt) out.push_back(std::move(r));
        else discard(r);
    }
    return out;
}

void AutosaveStore::discard(const Recovery& r) {
    r.autosave.deleteFile();
    r.meta.deleteFile();
}

}
