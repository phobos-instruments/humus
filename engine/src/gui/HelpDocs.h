#pragma once
#include <cctype>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Categories.h"
#include "core/PackRegistry.h"

namespace hum {

inline std::string genericHelpClass(const std::string& c) {
    if (c.size() < 2) return {};
    if (std::isdigit((unsigned char) c.back()) && !std::isdigit((unsigned char) c.front())) {
        size_t e = c.size();
        while (e > 0 && std::isdigit((unsigned char) c[e - 1])) --e;
        if (e > 1) return c.substr(0, e);
    }
    if (c.rfind("Midi", 0) == 0) {
        size_t i = 4;
        while (i < c.size() && std::isdigit((unsigned char) c[i])) ++i;
        if (i > 4 && i < c.size() && std::isupper((unsigned char) c[i]))
            return "Midi" + c.substr(i);
    }
    if (std::isdigit((unsigned char) c.front())) {
        size_t i = 0;
        while (i < c.size() && std::isdigit((unsigned char) c[i])) ++i;
        if (i + 1 < c.size() && c[i] == 'x' && std::isdigit((unsigned char) c[i + 1])) {
            ++i;
            while (i < c.size() && std::isdigit((unsigned char) c[i])) ++i;
        }
        const std::string base = c.substr(i);
        if (base.size() >= 2 && std::isupper((unsigned char) base[0])) return base;
        return {};
    }
    const char p = c.front();
    if (p != 'S' && p != 'M' && p != 'P') return {};
    size_t i = 1;
    while (i < c.size() && std::isdigit((unsigned char) c[i])) ++i;
    const std::string base = c.substr(i);
    if (base.size() < 2 || base == c) return {};
    if (!std::isupper((unsigned char) base[0])) return {};
    return base;
}

inline std::string renamedHelpClass(const std::string& c) {
    static constexpr std::pair<const char*, const char*> kRenames[] = {
        {"CamIn", "CameraIn"},         {"VHS", "VideoPlayer"},
        {"PodInlet", "PodIn"},         {"PodOutlet", "PodOut"},
        {"SPodInlet", "PodIn"},        {"SPodOutlet", "PodOut"},
        {"PodMidiInlet", "PodIn"},     {"PodMidiOutlet", "PodOut"},
        {"PodMidiIn", "PodIn"},        {"PodMidiOut", "PodOut"},
        {"AuxIn", "AudioIn"},          {"AuxOut", "AudioOut"},
    };
    for (const auto& [oldName, newName] : kRenames)
        if (c == oldName) return newName;
    return {};
}

inline std::vector<std::string> helpCandidateNames(const std::string& displayClass) {
    std::vector<std::string> names{displayClass};
    if (const auto g = genericHelpClass(displayClass); !g.empty()) {
        if (std::isdigit((unsigned char) displayClass.front())) names.push_back("Multi" + g);
        names.push_back(g);
    }
    for (size_t i = 0; i < names.size(); ++i)
        if (const auto r = renamedHelpClass(names[i]); !r.empty()) {
            bool have = false;
            for (const auto& n : names) have = have || n == r;
            if (!have) names.insert(names.begin() + (long) i + 1, r);
        }
    return names;
}

struct HelpDoc {
    juce::String text;
    std::string docClass;
};

inline HelpDoc loadOrganismDoc(const std::string& displayClass) {
    const auto* pack = PackRegistry::instance().packOf(displayClass);
    const std::vector<std::string> names = helpCandidateNames(displayClass);
    for (const auto& n : names)
        if (!pack) pack = PackRegistry::instance().packOf(n);
    if (!pack)
        for (const auto& p : PackRegistry::instance().packs())
            if (p.manifest.id == "core") { pack = &p; break; }
    if (pack == nullptr) return {};
    const juce::File dir = juce::File(juce::String(pack->dir)).getChildFile("help");
    for (const auto& n : names) {
        const auto f = dir.getChildFile(juce::String(n) + ".md");
        if (f.existsAsFile())
            if (auto t = f.loadFileAsString().trim(); t.isNotEmpty()) return {t, n};
    }
    return {};
}

inline juce::String loadOrganismHelp(const std::string& displayClass) {
    const auto page = loadOrganismDoc(displayClass);
    if (page.text.isNotEmpty()) return page.text;
    return "No help is available for " + juce::String(displayClass) + ".";
}

inline std::string helpSubtitle(const HelpDoc& page, const std::string& displayClass) {
    if (!page.docClass.empty())
        if (const auto* m = PackRegistry::instance().classManifest(page.docClass))
            if (!m->category.empty()) return m->category;
    return categoryOf(displayClass);
}

}
