// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/HostHistory.h"

#include <cstdint>
#include <utility>

#include "gui/host/EngineHost.h"
#include "io/PatchChanges.h"

namespace hum {

HostHistory::HostHistory(EngineHost& host, PatchHistory store) : host_(host), store_(std::move(store)) {}

void HostHistory::rebase(std::int64_t now) {
    base_ = host_.model();
    stamp_ = host_.changeStamp();
    lastTick_ = now;
}

void HostHistory::begin(const juce::String& documentPath, bool opened, std::int64_t now) {
    key_ = documentPath.isEmpty() ? juce::String() : PatchHistory::keyFor(documentPath);
    rebase(now);
    if (!opened || key_.isEmpty()) return;
    const auto existing = store_.entries(key_);
    const auto fileTime = juce::File(documentPath).getLastModificationTime().toMilliseconds();
    if (existing.empty() || existing.front().at < fileTime) snapshot(documentPath, "opened", now, true);
}

bool HostHistory::snapshot(const juce::String& documentPath, const juce::String& reason, std::int64_t now,
                           bool evenIfUnchanged) {
    if (!evenIfUnchanged && host_.changeStamp() == stamp_) return false;
    if (key_.isEmpty())
        key_ = documentPath.isEmpty() ? PatchHistory::untitledKey(now) : PatchHistory::keyFor(documentPath);
    const auto summary = history::summaryOf(history::changesBetween(base_, host_.model()));
    const bool written = store_.add(
        key_, now,
        [this](const juce::File& f) {
            std::string err;
            return host_.writeSnapshot(f.getFullPathName().toStdString(), err);
        },
        reason, juce::String::fromUTF8(summary.c_str()));
    if (!written) return false;
    rebase(now);
    store_.thin(key_, now);
    return true;
}

bool HostHistory::tick(const juce::String& documentPath, std::int64_t now) {
    if (intervalMs <= 0 || now - lastTick_ < intervalMs) return false;
    lastTick_ = now;
    return snapshot(documentPath, "edited", now);
}

void HostHistory::saved(const juce::String& previousPath, const juce::String& documentPath, std::int64_t now) {
    const auto savedKey = PatchHistory::keyFor(documentPath);
    if (previousPath.isEmpty() && key_.isNotEmpty() && key_ != savedKey) store_.adopt(key_, savedKey);
    key_ = savedKey;
    snapshot(documentPath, "saved", now);
}

int HostHistory::clear(const juce::String& documentPath, std::int64_t now) {
    const int removed = store_.clear(key_);
    rebase(now);
    snapshot(documentPath, "cleared", now, true);
    return removed;
}

bool HostHistory::restore(const PatchHistory::Entry& entry, const juce::String& documentPath, std::int64_t now,
                          std::string& error) {
    snapshot(documentPath, "before restoring", now);
    if (!host_.loadFileAs(entry.file.getFullPathName().toStdString(), documentPath.toStdString(), error))
        return false;
    host_.markDirty();
    rebase(now);
    return true;
}

bool HostHistory::openCopy(const PatchHistory::Entry& entry, const juce::String& originalPath, std::int64_t now,
                           std::string& error) {
    if (!host_.loadFileAs(entry.file.getFullPathName().toStdString(), originalPath.toStdString(), error))
        return false;
    host_.markDirty();
    key_ = {};
    rebase(now);
    return true;
}

}
