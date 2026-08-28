#include "io/AutosaveStore.h"

#include "io/PatchFormat.h"

#include <csignal>

#include "core/AppPaths.h"

#if JUCE_WINDOWS
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace hum {

AutosaveStore::AutosaveStore(juce::File dir, int pid) : dir_(std::move(dir)), pid_(pid) {}

juce::File AutosaveStore::defaultDir() {
    return appDataDir().getChildFile("autosave");
}

int AutosaveStore::currentPid() {
#if JUCE_WINDOWS
    return 0;
#else
    return (int) ::getpid();
#endif
}

bool AutosaveStore::pidAlive(int pid) {
#if JUCE_WINDOWS
    return false;
#else
    return pid > 0 && ::kill((pid_t) pid, 0) == 0;
#endif
}

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
