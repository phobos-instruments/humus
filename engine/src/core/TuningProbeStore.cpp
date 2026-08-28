#include "core/TuningProbeStore.h"

#include <juce_core/juce_core.h>

#include "core/AppPaths.h"

namespace hum {
namespace tuningProbeStore {

namespace {
juce::File storeFile() { return appDataDir().getChildFile("tuning-probe.xml"); }

std::unique_ptr<juce::XmlElement> loadXml() {
    auto xml = juce::parseXML(storeFile().loadFileAsString());
    if (xml == nullptr || !xml->hasTagName("tuning-probe"))
        xml = std::make_unique<juce::XmlElement>("tuning-probe");
    return xml;
}
}

bool lookup(const std::string& classRaw, long long pluginMtimeMs, TuningProbeVerdict& out) {
    const auto xml = loadXml();
    for (auto* e : xml->getChildIterator()) {
        if (e->getStringAttribute("class") != juce::String(classRaw)) continue;
        if (pluginMtimeMs != 0) {
            const auto stored = e->getStringAttribute("mtime").getLargeIntValue();
            if (stored != 0 && stored != pluginMtimeMs) return false;
        }
        return parseTuningProbeVerdict(
            e->getStringAttribute("verdict").toStdString(), out);
    }
    return false;
}

void store(const std::string& classRaw, long long pluginMtimeMs, TuningProbeVerdict v) {
    if (v == TuningProbeVerdict::Inconclusive) return;
    auto xml = loadXml();
    juce::XmlElement* mine = nullptr;
    for (auto* e : xml->getChildIterator())
        if (e->getStringAttribute("class") == juce::String(classRaw)) { mine = e; break; }
    if (mine == nullptr) {
        mine = xml->createNewChildElement("plugin");
        mine->setAttribute("class", juce::String(classRaw));
    }
    mine->setAttribute("verdict", tuningProbeVerdictName(v));
    mine->setAttribute("mtime", juce::String((juce::int64) pluginMtimeMs));
    mine->setAttribute("probed-at",
                       juce::Time::getCurrentTime().toISO8601(true));
    storeFile().replaceWithText(xml->toString());
}

}
}
