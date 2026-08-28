#pragma once
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class AutosaveStore {
public:
    explicit AutosaveStore(juce::File dir = defaultDir(), int pid = currentPid());

    static juce::File defaultDir();
    static int currentPid();
    static bool pidAlive(int pid);

    juce::File autosaveFile() const;
    void writeMeta(const juce::String& originalPath);
    void clear();

    struct Recovery {
        juce::File autosave;
        juce::File meta;
        juce::String originalPath;
        juce::Time savedAt;
    };

    std::vector<Recovery> findOrphans() const;

    static void discard(const Recovery& r);

private:
    juce::File metaFileFor(const juce::File& autosave) const;

    juce::File dir_;
    int pid_ = 0;
};

}
