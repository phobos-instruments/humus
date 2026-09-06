#pragma once
#include <juce_core/juce_core.h>

#include "gui/AppSettings.h"

namespace hum {

namespace recents {
inline juce::StringArray get() {
    juce::StringArray a;
    a.addTokens(AppSettings::instance().getString("recentFiles", ""), "\n", "");
    a.removeEmptyStrings();
    for (int i = a.size(); --i >= 0;)
        if (!juce::File(a[i]).existsAsFile()) a.remove(i);
    return a;
}
inline void push(const juce::File& f) {
    auto a = get();
    a.removeString(f.getFullPathName());
    a.insert(0, f.getFullPathName());
    while (a.size() > 10) a.remove(a.size() - 1);
    AppSettings::instance().set("recentFiles", a.joinIntoString("\n"));
}
inline void clear() { AppSettings::instance().set("recentFiles", ""); }

inline constexpr int kMenuIdBase = 200;

inline juce::File resolve(const juce::StringArray& shown, int id) {
    const int i = id - kMenuIdBase;
    return i >= 0 && i < shown.size() ? juce::File(shown[i]) : juce::File();
}

inline juce::StringArray labels(const juce::StringArray& paths) {
    juce::StringArray out;
    for (const auto& p : paths) {
        const juce::File f(p);
        int sameName = 0;
        for (const auto& other : paths)
            if (juce::File(other).getFileName() == f.getFileName()) ++sameName;
        out.add(sameName > 1
                    ? f.getFileName() + "  -  " + f.getParentDirectory().getFileName()
                    : f.getFileName());
    }
    return out;
}
}

}
