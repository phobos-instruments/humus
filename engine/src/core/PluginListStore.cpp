#include "core/PluginListStore.h"

#include "core/AppPaths.h"

namespace hum::pluginListStore {
namespace {

constexpr const char* kLegacyKey = "plugins.knownList";

juce::File listFile(const juce::File& appDir) { return appDir.getChildFile("plugin-list.xml"); }
juce::File settingsFile(const juce::File& appDir) { return appDir.getChildFile("settings.xml"); }

std::string dropLegacyAttribute(const juce::File& appDir) {
    auto xml = juce::XmlDocument::parse(settingsFile(appDir));
    if (!xml || !xml->hasAttribute(kLegacyKey)) return {};
    const auto list = xml->getStringAttribute(kLegacyKey);
    xml->removeAttribute(kLegacyKey);
    xml->writeTo(settingsFile(appDir), {});
    return list.toStdString();
}

std::string migrateFromSettings(const juce::File& appDir) {
    auto xml = juce::XmlDocument::parse(settingsFile(appDir));
    if (!xml || !xml->hasAttribute(kLegacyKey)) return {};
    const auto list = xml->getStringAttribute(kLegacyKey);
    if (list.isNotEmpty() && !listFile(appDir).replaceWithText(list))
        return list.toStdString();
    return dropLegacyAttribute(appDir);
}

}

std::string loadFrom(const juce::File& appDir) {
    auto f = listFile(appDir);
    if (!f.existsAsFile()) return migrateFromSettings(appDir);
    dropLegacyAttribute(appDir);
    return f.loadFileAsString().toStdString();
}

void saveTo(const juce::File& appDir, const std::string& xml) {
    appDir.createDirectory();
    listFile(appDir).replaceWithText(juce::String(xml));
}

std::string load() { return loadFrom(appDataDir()); }
void save(const std::string& xml) { saveTo(appDataDir(), xml); }

}
