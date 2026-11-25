// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/PluginHost.h"

#include <algorithm>

#include "core/packs/ClassString.h"
#include "hum/Registry.h"

#include "hum/dsp/DspMath.h"

namespace hum {

PluginHost& PluginHost::instance() {
    static PluginHost h;
    return h;
}

PluginHost::PluginHost() {
    formats_.addFormat(new juce::VST3PluginFormat());
#if JUCE_PLUGINHOST_LV2
    formats_.addFormat(new juce::LV2PluginFormat());
#endif
#if JUCE_PLUGINHOST_AU && JUCE_MAC
    formats_.addFormat(new juce::AudioUnitPluginFormat());
#endif
}

namespace {
bool auKeyMatches(const std::string& key, const juce::String& fileOrIdentifier) {
    if (key.size() != 12) return false;
    const juce::String type(key.substr(0, 4)), sub(key.substr(4, 4)), manu(key.substr(8, 4));
    const int t = fileOrIdentifier.indexOf(type);
    if (t < 0) return false;
    const int su = fileOrIdentifier.indexOf(t, sub);
    if (su < 0) return false;
    return fileOrIdentifier.indexOf(su, manu) >= 0;
}
}

namespace {
std::string sanitizeName(const juce::String& name) {
    auto s = name.trim();
    for (auto c : {'?', '&', '='}) s = s.replaceCharacter(c, '_');
    return s.toStdString();
}
}

std::string PluginHost::classRawFor(const juce::PluginDescription& desc) {
    const std::string name = sanitizeName(desc.name);
    if (desc.pluginFormatName == "VST3") {
        const int uid = desc.uniqueId != 0 ? desc.uniqueId : desc.deprecatedUid;
        return name + "?type=vst3&uid=" + juce::String::toHexString(uid).toStdString();
    }
    if (desc.pluginFormatName == "LV2")
        return name + "?type=lv2&uri="
             + juce::URL::addEscapeChars(desc.fileOrIdentifier, false).toStdString();
    if (desc.pluginFormatName == "CLAP")
        return name + "?type=clap&uid=" + juce::String::toHexString(desc.uniqueId).toStdString();
    if (desc.pluginFormatName == "AudioUnit")
        return name + "?type=au&auid="
             + juce::URL::addEscapeChars(desc.fileOrIdentifier, false).toStdString();
    return name + "?type=" + desc.pluginFormatName.toLowerCase().toStdString();
}

std::unique_ptr<juce::PluginDescription> PluginHost::descriptionFor(const std::string& classRaw) const {
    const auto id = parseClassString(classRaw);
    for (const auto& d : list_.getTypes()) {
        if (id.kind == "vst3" && d.pluginFormatName == "VST3") {
            const int uid = d.uniqueId != 0 ? d.uniqueId : d.deprecatedUid;
            if (juce::String::toHexString(uid).toStdString() == id.get("uid"))
                return std::make_unique<juce::PluginDescription>(d);
        } else if (id.kind == "clap" && d.pluginFormatName == "CLAP") {
            if (juce::String::toHexString(d.uniqueId).toStdString() == id.get("uid"))
                return std::make_unique<juce::PluginDescription>(d);
        } else if (id.kind == "lv2" && d.pluginFormatName == "LV2") {
            if (juce::URL::removeEscapeChars(juce::String(id.get("uri"))) == d.fileOrIdentifier)
                return std::make_unique<juce::PluginDescription>(d);
        } else if (id.kind == "au" && d.pluginFormatName == "AudioUnit") {
            if (!id.get("auid").empty()
                && juce::URL::removeEscapeChars(juce::String(id.get("auid"))) == d.fileOrIdentifier)
                return std::make_unique<juce::PluginDescription>(d);
            if (auKeyMatches(id.get("key"), d.fileOrIdentifier))
                return std::make_unique<juce::PluginDescription>(d);
        }
    }
    return nullptr;
}

std::unique_ptr<juce::AudioPluginInstance> PluginHost::createInstance(
    const juce::PluginDescription& desc, double sampleRate, int blockSize, std::string& error) {
    juce::String err;
    auto inst = formats_.createPluginInstance(desc, sampleRate, blockSize, err);
    if (!inst) error = err.toStdString();
    else ++instancesMade_;
    return inst;
}

bool PluginHost::createsOnlyAsync(const juce::PluginDescription& desc) {
    if (!juce::MessageManager::getInstance()->isThisTheMessageThread()) return false;
    for (auto* format : formats_.getFormats())
        if (format->getName() == desc.pluginFormatName)
            return format->requiresUnblockedMessageThreadDuringCreation(desc);
    return false;
}

std::unique_ptr<juce::AudioPluginInstance> PluginHost::takeAsyncInstance(const std::string& classRaw,
                                                                         const juce::PluginDescription& desc) {
    auto& wait = async_[classRaw];
    if (!wait.ready.empty()) {
        auto ready = std::move(wait.ready.back());
        wait.ready.pop_back();
        ++instancesMade_;
        return ready;
    }
    if (wait.inFlight || wait.failed) return nullptr;
    startAsync(classRaw, desc);
    return nullptr;
}

bool PluginHost::prepareAsync(const std::string& classRaw, const juce::PluginDescription& desc, int count) {
    auto& wait = async_[classRaw];
    if (wait.failed || (int) wait.ready.size() >= count) return true;
    if (!wait.inFlight) startAsync(classRaw, desc);
    return false;
}

void PluginHost::startAsync(const std::string& classRaw, const juce::PluginDescription& desc) {
    async_[classRaw].inFlight = true;
    formats_.createPluginInstanceAsync(
        desc, kDefaultSampleRate, 512,
        [this, classRaw](std::unique_ptr<juce::AudioPluginInstance> made, const juce::String&) {
            auto held = std::make_shared<std::unique_ptr<juce::AudioPluginInstance>>(std::move(made));
            auto land = [this, classRaw, held] { landAsyncInstance(classRaw, std::move(*held)); };
            if (juce::MessageManager::getInstance()->isThisTheMessageThread()) land();
            else juce::MessageManager::callAsync(std::move(land));
        });
}

void PluginHost::landAsyncInstance(const std::string& classRaw, std::unique_ptr<juce::AudioPluginInstance> made) {
    auto& wait = async_[classRaw];
    wait.inFlight = false;
    if (made == nullptr) {
        wait.failed = true;
        return;
    }
    if (asyncListeners_.empty()) {
        retire(std::move(made), classRaw);
        return;
    }
    learnFrom(classRaw, *made);
    wait.ready.push_back(std::move(made));
    const auto listeners = asyncListeners_;
    for (const auto& entry : listeners) entry.second();
}

int PluginHost::addAsyncReadyListener(std::function<void()> listener) {
    asyncListeners_[++nextAsyncListener_] = std::move(listener);
    return nextAsyncListener_;
}

void PluginHost::removeAsyncReadyListener(int id) {
    asyncListeners_.erase(id);
    if (!asyncListeners_.empty()) return;
    for (auto& [classRaw, wait] : async_)
        for (auto& left : wait.ready) retire(std::move(left), classRaw);
    async_.clear();
}

std::vector<std::string> PluginHost::classRawList() const {
    std::vector<std::string> out;
    for (const auto& d : list_.getTypes()) out.push_back(classRawFor(d));
    std::sort(out.begin(), out.end());
    return out;
}

bool PluginHost::isPluginClass(const std::string& classRaw) const {
    return isPluginKind(parseClassString(classRaw).kind);
}

const PluginHost::PaletteFacts* PluginHost::paletteFactsFor(const std::string& classRaw) {
    if (factsCount_ != list_.getNumTypes()) {
        factsCache_.clear();
        for (const auto& d : list_.getTypes()) {
            PaletteFacts f;
            f.vendor = d.manufacturerName.isNotEmpty() ? d.manufacturerName.toStdString()
                                                       : "Unknown Vendor";
            f.instrument = d.isInstrument;
            factsCache_.emplace(classRawFor(d), std::move(f));
        }
        factsCount_ = list_.getNumTypes();
    }
    const auto it = factsCache_.find(classRaw);
    return it == factsCache_.end() ? nullptr : &it->second;
}

bool PluginHost::isInstrument(const std::string& classRaw) {
    if (const auto* f = paletteFactsFor(classRaw)) return f->instrument;
    if (auto d = descriptionFor(classRaw)) return d->isInstrument;
    return false;
}

std::string PluginHost::vendorOf(const std::string& classRaw) {
    if (const auto* f = paletteFactsFor(classRaw)) return f->vendor;
    if (auto d = descriptionFor(classRaw))
        if (d->manufacturerName.isNotEmpty()) return d->manufacturerName.toStdString();
    return "Unknown Vendor";
}

const std::vector<ParamDesc>& PluginHost::schemaFor(const std::string& classRaw) {
    static const std::vector<ParamDesc> empty;
    auto it = schemaCache_.find(classRaw);
    if (it != schemaCache_.end()) return it->second;

    auto desc = descriptionFor(classRaw);
    if (!desc) return empty;
    std::string err;
    auto probe = createInstance(*desc, kDefaultSampleRate, 512, err);
    if (!probe) return empty;
    const auto& schema = learnFrom(classRaw, *probe);
    retire(std::move(probe), classRaw);
    return schema;
}

const std::vector<ParamDesc>& PluginHost::learnFrom(const std::string& classRaw,
                                                    juce::AudioPluginInstance& instance) {
    if (auto it = schemaCache_.find(classRaw); it != schemaCache_.end()) return it->second;
    ioCache_[classRaw] = {std::max(0, instance.getTotalNumInputChannels()),
                          std::max(1, instance.getTotalNumOutputChannels()),
                          instance.acceptsMidi(), instance.producesMidi(), instance.hasEditor()};

    std::vector<ParamDesc> schema;
    const auto& params = instance.getParameters();
    const int n = std::min((int) params.size(), kMaxParams);
    std::unordered_map<std::string, int> seen;
    for (int i = 0; i < n; ++i) {
        auto* p = params[(size_t) i];
        std::string name = p->getName(64).trim().toStdString();
        if (name.empty()) name = "Param";
        if (++seen[name] > 1 || name == "Param")
            name += " #" + std::to_string(i);
        schema.push_back({name, 0.0, 1.0, (double) p->getDefaultValue()});
    }
    return schemaCache_.emplace(classRaw, std::move(schema)).first->second;
}

const PluginHost::PluginIoFacts& PluginHost::ioFactsFor(const std::string& classRaw) {
    static const PluginIoFacts fallback;
    schemaFor(classRaw);
    auto it = ioCache_.find(classRaw);
    return it == ioCache_.end() ? fallback : it->second;
}

std::vector<std::string> PluginHost::allPaletteClasses() {
    auto out = Registry::instance().classNames();
    for (auto& raw : instance().classRawList()) out.push_back(std::move(raw));
    return out;
}

std::string PluginHost::knownListToXml() const {
    if (auto xml = list_.createXml()) return xml->toString().toStdString();
    return {};
}

void PluginHost::restoreKnownListFromXml(const std::string& xml) {
    if (auto parsed = juce::parseXML(juce::String(xml)))
        list_.recreateFromXml(*parsed);
    invalidatePaletteFacts();
}

}
