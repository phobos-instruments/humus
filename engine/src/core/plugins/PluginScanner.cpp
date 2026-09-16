// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/plugins/PluginScanner.h"

#include <algorithm>
#include <map>

#include "core/plugins/PluginHost.h"

namespace hum {

namespace {
constexpr int kSingleTimeoutMs = 240000;
constexpr int kBatchTimeoutMs = 240000;
constexpr int kFallbackTimeoutMs = 60000;
constexpr int kEnumerateTimeoutMs = 30000;
constexpr int kAuBatch = 8;

bool isPathIdentifier(const juce::String& f) { return juce::File::isAbsolutePath(f); }

juce::int64 diskStamp(const juce::String& file) {
    return juce::File(file).getLastModificationTime().toMilliseconds();
}
}

bool PluginScanner::formatCanSearch(const juce::AudioPluginFormat& format, const juce::File& dir) {
    if (format.getName() != "LV2") return true;
    if (dir.hasFileExtension("lv2")) return true;
    return !dir.findChildFiles(juce::File::findDirectories, false, "*.lv2").isEmpty();
}

void PluginScanner::start(const juce::StringArray& extraPaths, const juce::File& deadMansPedal,
                          bool includeDefaultPaths) {
    items_.clear();
    failed_.clear();
    index_ = 0;
    mergedFiles_ = 0;
    if (deadMansPedal != juce::File{}) deadMansPedal.create();

    for (auto* format : host_.formats().getFormats()) {
        if (format->getName() == "AudioUnit") {
            if (!includeDefaultPaths) continue;
            juce::String lines;
            if (exe_ != juce::File{}) {
                if (!childToFile("--scan-enumerate-out", { format->getName() },
                                 lines, kEnumerateTimeoutMs))
                    continue;
            } else {
                lines = enumerateLines(format->getName());
            }
            for (const auto& id : juce::StringArray::fromLines(lines))
                if (id.isNotEmpty()) items_.push_back({ format->getName(), id });
            continue;
        }
        juce::FileSearchPath paths;
        if (includeDefaultPaths) paths = format->getDefaultLocationsToSearch();
        for (const auto& p : extraPaths)
            if (p.isNotEmpty() && formatCanSearch(*format, juce::File(p)))
                paths.addIfNotAlreadyThere(juce::File(p).getFullPathName());
        for (const auto& file : format->searchPathsForPlugins(paths, true))
            items_.push_back({ format->getName(), file });
    }
}

juce::String PluginScanner::skipEntryFor(const juce::String& file) {
    if (!isPathIdentifier(file) || !juce::File(file).exists()) return file;
    return file + "|" + juce::String::toHexString(diskStamp(file));
}

void PluginScanner::skipKnownFiles(const juce::StringArray& alsoSkip) {
    std::map<juce::String, juce::int64> known;
    for (const auto& d : host_.knownPlugins().getTypes()) {
        auto& t = known[d.fileOrIdentifier];
        t = std::max(t, d.lastFileModTime.toMilliseconds());
    }

    juce::StringArray skipIds;
    std::map<juce::String, juce::int64> skipStamped;
    for (const auto& e : alsoSkip) {
        const int bar = e.lastIndexOfChar('|');
        if (bar > 0) skipStamped[e.substring(0, bar)] = e.substring(bar + 1).getHexValue64();
        else if (!isPathIdentifier(e)) skipIds.addIfNotAlreadyThere(e);
    }

    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [&](const Item& it) {
                                    if (auto k = skipStamped.find(it.file); k != skipStamped.end()
                                        && k->second == diskStamp(it.file))
                                        return true;
                                    if (skipIds.contains(it.file)) return true;
                                    auto k = known.find(it.file);
                                    if (k == known.end()) return false;
                                    if (!isPathIdentifier(it.file) || k->second == 0
                                        || k->second == diskStamp(it.file))
                                        return true;
                                    dropKnownTypesFor(it.file);
                                    return false;
                                }),
                 items_.end());
}

void PluginScanner::dropKnownTypesFor(const juce::String& file) {
    auto& kp = host_.knownPlugins();
    for (const auto& d : kp.getTypes())
        if (d.fileOrIdentifier == file) kp.removeType(d);
    host_.invalidatePaletteFacts();
}

juce::String PluginScanner::probeFileXml(const juce::String& format, const juce::String& file) {
    juce::StringArray one;
    one.add(file);
    return probeFilesXml(format, one);
}

juce::String PluginScanner::probeFilesXml(const juce::String& format,
                                          const juce::StringArray& files) {
    juce::XmlElement root("plugins");
    for (auto* fmt : PluginHost::instance().formats().getFormats()) {
        if (fmt->getName() != format) continue;
        for (const auto& file : files) {
            juce::OwnedArray<juce::PluginDescription> descs;
            fmt->findAllTypesForFile(descs, file);
            for (auto* d : descs)
                if (auto xml = d->createXml()) root.addChildElement(xml.release());
        }
        break;
    }
    return root.toString(juce::XmlElement::TextFormat().singleLine());
}

void PluginScanner::mergeXml(const juce::String& xml) {
    if (auto root = juce::parseXML(xml))
        for (auto* el : root->getChildIterator()) {
            juce::PluginDescription d;
            if (d.loadFromXml(*el)) host_.knownPlugins().addType(d);
        }
}

void PluginScanner::noteFailed(const juce::String& file) {
    failed_.addIfNotAlreadyThere(skipEntryFor(file));
}

bool PluginScanner::probeViaSubprocess(const juce::String& format,
                                       const juce::StringArray& files, juce::String& xml,
                                       int timeoutMs) {
    juce::StringArray rest;
    rest.add(format);
    rest.addArray(files);
    return childToFile("--scan-plugin-out", rest, xml, timeoutMs);
}

bool PluginScanner::childToFile(const juce::String& flag, const juce::StringArray& rest,
                                juce::String& textOut, int timeoutMs) {
    const auto out = juce::File::getSpecialLocation(juce::File::tempDirectory)
                         .getChildFile("hum-scan-" + juce::Uuid().toString() + ".xml");
    juce::ChildProcess child;
    juce::StringArray args;
    args.add(exe_.getFullPathName());
    args.add(flag);
    args.add(out.getFullPathName());
    args.addArray(rest);
    if (!child.start(args, juce::ChildProcess::wantStdOut)) return false;
    {
        const std::lock_guard<std::mutex> g(childrenMutex_);
        for (auto*& slot : children_) if (slot == nullptr) { slot = &child; break; }
    }
    const bool finished = child.waitForProcessToFinish(timeoutMs);
    {
        const std::lock_guard<std::mutex> g(childrenMutex_);
        for (auto*& slot : children_) if (slot == &child) slot = nullptr;
    }
    if (!finished) { child.kill(); out.deleteFile(); return false; }
    const bool ok = child.getExitCode() == 0 && out.existsAsFile();
    if (ok) textOut = out.loadFileAsString();
    out.deleteFile();
    return ok;
}

juce::String PluginScanner::enumerateLines(const juce::String& format) {
    for (auto* fmt : PluginHost::instance().formats().getFormats())
        if (fmt->getName() == format)
            return fmt->searchPathsForPlugins(fmt->getDefaultLocationsToSearch(),
 true)
                .joinIntoString("\n");
    return {};
}

bool PluginScanner::scanNext(juce::String& current) {
    if (index_ >= items_.size()) return false;
    const Item it = items_[index_++];
    current = it.file;

    Result r;
    r.files.add(it.file);
    if (exe_ != juce::File{}) {
        if (children_.empty()) children_.assign(1, nullptr);
        r.ok = probeViaSubprocess(it.format, r.files, r.xml, kSingleTimeoutMs);
    } else {
        r.xml = probeFileXml(it.format, it.file);
        r.ok = true;
    }
    mergeResult(r);
    return index_ < items_.size();
}

void PluginScanner::beginAsync(int workers) {
    jassert(exe_ != juce::File{});
    cancel_.store(false);
    const int n = juce::jlimit(1, 16, workers);
    inflight_.assign((size_t) n, {});
    children_.assign((size_t) n, nullptr);
    activeWorkers_.store(n);
    for (int i = 0; i < n; ++i)
        workers_.emplace_back([this, i] { workerLoop(i); });
}

void PluginScanner::workerLoop(int slot) {
    for (;;) {
        if (cancel_.load()) break;
        juce::String format;
        juce::StringArray files;
        {
            const std::lock_guard<std::mutex> g(queueMutex_);
            if (index_ >= items_.size()) break;
            format = items_[index_].format;
            const int batch = format == "AudioUnit" ? kAuBatch : 1;
            while ((int) files.size() < batch && index_ < items_.size()
                   && items_[index_].format == format)
                files.add(items_[index_++].file);
            inflight_[(size_t) slot] = files[0];
        }

        juce::String xml;
        bool ok = probeViaSubprocess(format, files, xml,
                                     files.size() > 1 ? kBatchTimeoutMs : kSingleTimeoutMs);
        if (!ok && files.size() > 1 && !cancel_.load()) {
            for (const auto& f : files) {
                if (cancel_.load()) break;
                {
                    const std::lock_guard<std::mutex> g(queueMutex_);
                    inflight_[(size_t) slot] = f;
                }
                juce::String x1;
                juce::StringArray one;
                one.add(f);
                const bool ok1 = probeViaSubprocess(format, one, x1, kFallbackTimeoutMs);
                Result r;
                r.files = one;
                r.xml = x1;
                r.ok = ok1;
                const std::lock_guard<std::mutex> g(resultsMutex_);
                results_.push_back(std::move(r));
            }
        } else {
            Result r;
            r.files = files;
            r.xml = xml;
            r.ok = ok;
            const std::lock_guard<std::mutex> g(resultsMutex_);
            results_.push_back(std::move(r));
        }
        {
            const std::lock_guard<std::mutex> g(queueMutex_);
            inflight_[(size_t) slot] = {};
        }
    }
    activeWorkers_.fetch_sub(1);
}

void PluginScanner::mergeResult(const Result& r) {
    if (r.ok) {
        mergeXml(r.xml);
        for (const auto& f : r.files)
            if (!r.xml.contains(f)) noteFailed(f);
    } else {
        for (const auto& f : r.files) noteFailed(f);
    }
    mergedFiles_ += (size_t) r.files.size();
}

bool PluginScanner::pumpAsync(juce::String& current) {
    const bool workersDone = activeWorkers_.load() == 0;
    std::vector<Result> batch;
    {
        const std::lock_guard<std::mutex> g(resultsMutex_);
        batch.swap(results_);
    }
    for (const auto& r : batch) mergeResult(r);
    {
        const std::lock_guard<std::mutex> g(queueMutex_);
        for (const auto& f : inflight_)
            if (f.isNotEmpty()) { current = f; break; }
    }
    if (!workersDone && mergedFiles_ < items_.size()) return true;
    cancelAsync();
    return false;
}

void PluginScanner::cancelAsync() {
    cancel_.store(true);
    {
        const std::lock_guard<std::mutex> g(childrenMutex_);
        for (auto* c : children_) if (c != nullptr) c->kill();
    }
    for (auto& w : workers_) if (w.joinable()) w.join();
    workers_.clear();
}

float PluginScanner::progress() const {
    if (items_.empty()) return 1.0f;
    return juce::jlimit(0.0f, 1.0f, (float) mergedFiles_ / (float) items_.size());
}

}
