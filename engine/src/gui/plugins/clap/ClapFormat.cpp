// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/plugins/clap/ClapFormat.h"

#include <memory>
#include <string>

#include <clap/plugin-features.h>

#include "core/plugins/PluginHost.h"
#include "core/plugins/clap/ClapModule.h"
#include "gui/plugins/clap/ClapInstance.h"
#include "gui/plugins/clap/ClapSession.h"

namespace hum::claphost {

namespace {

constexpr int kSearchDepth = 8;

juce::String categoryOf(const Descriptor& d) {
    if (d.isInstrument()) return "Instrument";
    if (d.has(CLAP_PLUGIN_FEATURE_NOTE_EFFECT)) return "Note effect";
    if (d.has(CLAP_PLUGIN_FEATURE_ANALYZER)) return "Analyzer";
    return "Effect";
}

void collect(const juce::File& dir, bool recursive, int depth, juce::StringArray& out) {
    if (depth > kSearchDepth || !dir.isDirectory()) return;
    for (const auto& entry : juce::RangedDirectoryIterator(dir, false, "*", juce::File::findFilesAndDirectories)) {
        const auto f = entry.getFile();
        if (f.hasFileExtension("clap")) out.addIfNotAlreadyThere(f.getFullPathName());
        else if (recursive && entry.isDirectory() && !entry.isHidden()) collect(f, true, depth + 1, out);
    }
}

juce::StringArray clapPathFromEnvironment() {
    const auto value = juce::SystemStats::getEnvironmentVariable("CLAP_PATH", {});
#if JUCE_WINDOWS
    return juce::StringArray::fromTokens(value, ";", "");
#else
    return juce::StringArray::fromTokens(value, ":", "");
#endif
}

}

void Format::findAllTypesForFile(juce::OwnedArray<juce::PluginDescription>& results, const juce::String& file) {
    if (!fileMightContainThisPluginType(file)) return;
    std::string error;
    const std::string path = file.toStdString();
    auto module = Module::open(path, error);
    if (module == nullptr) return;
    const auto all = module->descriptors();
    const juce::File onDisk(file);
    for (const auto& d : all) {
        const uint32_t uid = uidFor(d.id);
        auto session = Session::open(path, uid, error);
        if (session == nullptr) continue;
        const auto ports = session->ports();
        auto desc = std::make_unique<juce::PluginDescription>();
        desc->name = juce::String(d.name);
        desc->descriptiveName = d.description.empty() ? desc->name : juce::String(d.description);
        desc->pluginFormatName = kName;
        desc->category = categoryOf(d);
        desc->manufacturerName = juce::String(d.vendor);
        desc->version = juce::String(d.version);
        desc->fileOrIdentifier = onDisk.getFullPathName();
        desc->lastFileModTime = onDisk.getLastModificationTime();
        desc->lastInfoUpdateTime = juce::Time::getCurrentTime();
        desc->uniqueId = desc->deprecatedUid = (int) uid;
        desc->isInstrument = d.isInstrument();
        desc->numInputChannels = ports.inChannels();
        desc->numOutputChannels = ports.outChannels();
        desc->hasSharedContainer = all.size() > 1;
        results.add(desc.release());
    }
}

bool Format::fileMightContainThisPluginType(const juce::String& file) {
    const juce::File f(file);
    return f.hasFileExtension("clap") && f.exists();
}

juce::String Format::getNameOfPluginFromIdentifier(const juce::String& file) {
    return juce::File(file).getFileNameWithoutExtension();
}

bool Format::pluginNeedsRescanning(const juce::PluginDescription& description) {
    return juce::File(description.fileOrIdentifier).getLastModificationTime() != description.lastFileModTime;
}

bool Format::doesPluginStillExist(const juce::PluginDescription& description) {
    return juce::File(description.fileOrIdentifier).exists();
}

juce::StringArray Format::searchPathsForPlugins(const juce::FileSearchPath& directories, bool recursive, bool) {
    juce::StringArray found;
    for (int i = 0; i < directories.getNumPaths(); ++i) {
        const auto dir = directories[i];
        if (dir.hasFileExtension("clap") && dir.exists()) found.addIfNotAlreadyThere(dir.getFullPathName());
        else collect(dir, recursive, 0, found);
    }
    return found;
}

juce::FileSearchPath Format::getDefaultLocationsToSearch() {
    juce::FileSearchPath paths;
#if JUCE_MAC
    paths.add(juce::File("~/Library/Audio/Plug-Ins/CLAP"));
    paths.add(juce::File("/Library/Audio/Plug-Ins/CLAP"));
#elif JUCE_WINDOWS
    const auto common = juce::SystemStats::getEnvironmentVariable("COMMONPROGRAMFILES", "C:\\Program Files\\Common Files");
    paths.add(juce::File(common).getChildFile("CLAP"));
    const auto local = juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA", {});
    if (local.isNotEmpty()) paths.add(juce::File(local).getChildFile("Programs").getChildFile("Common").getChildFile("CLAP"));
#else
    paths.add(juce::File("~/.clap"));
    paths.add(juce::File("/usr/lib/clap"));
#endif
    for (const auto& extra : clapPathFromEnvironment())
        if (juce::File::isAbsolutePath(extra)) paths.addIfNotAlreadyThere(juce::File(extra));
    return paths;
}

void Format::createPluginInstance(const juce::PluginDescription& description, double, int,
                                  PluginCreationCallback callback) {
    std::string error;
    auto session = Session::open(description.fileOrIdentifier.toStdString(), (uint32_t) description.uniqueId, error);
    if (session == nullptr) {
        callback(nullptr, juce::String(error));
        return;
    }
    callback(std::make_unique<Instance>(std::move(session), description), {});
}

void addClapFormat(juce::AudioPluginFormatManager& formats) {
    for (auto* f : formats.getFormats())
        if (f->getName() == Format::kName) return;
    formats.addFormat(new Format());
}

void enableClapHosting() { addClapFormat(PluginHost::instance().formats()); }

}
