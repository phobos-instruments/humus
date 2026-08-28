#include "gui/AppSettings.h"

#include "core/AppPaths.h"

namespace hum {

AppSettings& AppSettings::instance() {
    static AppSettings s;
    return s;
}

juce::File AppSettings::file() {
    return appDataDir().getChildFile("settings.xml");
}

AppSettings::AppSettings() {
    existedAtBoot_ = file().existsAsFile();
    if (auto xml = juce::XmlDocument::parse(file()))
        root_ = std::move(xml);
    if (!root_ || !root_->hasTagName("settings"))
        root_ = std::make_unique<juce::XmlElement>("settings");
}

void AppSettings::save() {
    if (batch_ > 0) { dirty_ = true; return; }
    auto f = file();
    f.getParentDirectory().createDirectory();
    if (auto disk = juce::XmlDocument::parse(f); disk && disk->hasTagName("settings")) {
        for (const auto& k : dirtyKeys_)
            if (root_->hasAttribute(k))
                disk->setAttribute(k, root_->getStringAttribute(k));
        root_ = std::move(disk);
    }
    root_->writeTo(f, {});
}

void AppSettings::endBatch() {
    if (batch_ > 0) --batch_;
    if (batch_ == 0 && dirty_) { dirty_ = false; save(); }
}

juce::String AppSettings::getString(const juce::String& key, const juce::String& def) const {
    return root_->getStringAttribute(key, def);
}
double AppSettings::getDouble(const juce::String& key, double def) const {
    return root_->hasAttribute(key) ? root_->getDoubleAttribute(key, def) : def;
}
int AppSettings::getInt(const juce::String& key, int def) const {
    return root_->hasAttribute(key) ? root_->getIntAttribute(key, def) : def;
}

void AppSettings::set(const juce::String& key, const juce::String& value) {
    root_->setAttribute(key, value);
    dirtyKeys_.insert(key);
    save();
}
void AppSettings::set(const juce::String& key, double value) {
    root_->setAttribute(key, value);
    dirtyKeys_.insert(key);
    save();
}
void AppSettings::set(const juce::String& key, int value) {
    root_->setAttribute(key, value);
    dirtyKeys_.insert(key);
    save();
}

}
