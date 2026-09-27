// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/PatchHistory.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "core/app/AppPaths.h"
#include "core/xml/Xml.h"
#include "io/HistoryRules.h"
#include "io/PatchFormat.h"

namespace hum {

namespace {

constexpr int kStemChars = 40;

std::string utf8(const juce::String& s) { return s.toStdString(); }

juce::String fromUtf8(const std::string& s) { return juce::String::fromUTF8(s.c_str()); }

}

PatchHistory::PatchHistory(juce::File root) : root_(std::move(root)) {}

juce::File PatchHistory::defaultRoot() { return patchHistoryDir(); }

juce::String PatchHistory::keyFor(const juce::String& documentPath) {
    const juce::File doc(documentPath);
    const auto stem = juce::File::createLegalFileName(doc.getFileNameWithoutExtension()).substring(0, kStemChars);
    const auto hash = juce::String::toHexString((juce::int64) documentPath.hashCode64());
    return (stem.isEmpty() ? juce::String("patch") : stem) + "-" + hash;
}

juce::String PatchHistory::untitledKey(std::int64_t now) { return "untitled-" + juce::String(now); }

juce::File PatchHistory::folderOf(const juce::String& key) const { return root_.getChildFile(key); }

juce::File PatchHistory::metaFor(const juce::File& snapshot) { return snapshot.withFileExtension("meta.xml"); }

void PatchHistory::writeMeta(const Entry& e) {
    xml::Element meta("history-entry");
    meta.setAttribute("at", std::to_string(e.at));
    meta.setAttribute("reason", utf8(e.reason));
    meta.setAttribute("summary", utf8(e.summary));
    if (e.kept) {
        meta.setAttribute("kept", "true");
        meta.setAttribute("name", utf8(e.name));
    }
    xml::writeFile(metaFor(e.file).getFullPathName().toStdString(), meta);
}

std::vector<PatchHistory::Entry> PatchHistory::entries(const juce::String& key) const {
    std::vector<Entry> out;
    if (key.isEmpty()) return out;
    const auto pattern = juce::String("*.") + kPatchExt;
    for (const auto& f : folderOf(key).findChildFiles(juce::File::findFiles, false, pattern)) {
        Entry e;
        e.file = f;
        e.at = f.getFileNameWithoutExtension().getLargeIntValue();
        if (e.at <= 0) continue;
        if (auto meta = xml::parseFile(metaFor(f).getFullPathName().toStdString())) {
            e.reason = fromUtf8(meta->attribute("reason"));
            e.summary = fromUtf8(meta->attribute("summary"));
            e.name = fromUtf8(meta->attribute("name"));
            e.kept = meta->boolAttribute("kept");
        }
        out.push_back(std::move(e));
    }
    std::sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) { return a.at > b.at; });
    return out;
}

bool PatchHistory::add(const juce::String& key, std::int64_t at, const Writer& write, const juce::String& reason,
                       const juce::String& summary) {
    if (key.isEmpty() || !write) return false;
    const auto folder = folderOf(key);
    if (!folder.createDirectory()) return false;
    auto file = folder.getChildFile(juce::String(at) + "." + kPatchExt);
    while (file.exists()) file = folder.getChildFile(juce::String(++at) + "." + kPatchExt);
    if (!write(file) || !file.existsAsFile()) {
        file.deleteFile();
        return false;
    }
    Entry e;
    e.file = file;
    e.at = at;
    e.reason = reason;
    e.summary = summary;
    writeMeta(e);
    return true;
}

bool PatchHistory::keep(const juce::String& key, std::int64_t at, const juce::String& name) {
    for (auto e : entries(key))
        if (e.at == at) {
            e.kept = true;
            e.name = name;
            writeMeta(e);
            return true;
        }
    return false;
}

bool PatchHistory::release(const juce::String& key, std::int64_t at) {
    for (auto e : entries(key))
        if (e.at == at) {
            e.kept = false;
            e.name = {};
            writeMeta(e);
            return true;
        }
    return false;
}

int PatchHistory::thin(const juce::String& key, std::int64_t now) {
    const auto all = entries(key);
    std::vector<history::Stamp> stamps;
    for (const auto& e : all)
        stamps.push_back({e.at, e.kept, (std::uint64_t) e.file.getSize()});
    const auto drop = history::toDrop(stamps, now);
    for (auto i : drop) {
        all[i].file.deleteFile();
        metaFor(all[i].file).deleteFile();
    }
    return (int) drop.size();
}

int PatchHistory::clear(const juce::String& key) {
    int removed = 0;
    for (const auto& e : entries(key)) {
        if (e.kept) continue;
        e.file.deleteFile();
        metaFor(e.file).deleteFile();
        ++removed;
    }
    return removed;
}

void PatchHistory::adopt(const juce::String& from, const juce::String& to) {
    if (from.isEmpty() || to.isEmpty() || from == to) return;
    const auto source = folderOf(from);
    if (!source.isDirectory()) return;
    const auto target = folderOf(to);
    target.createDirectory();
    for (const auto& f : source.findChildFiles(juce::File::findFiles, false))
        f.moveFileTo(target.getChildFile(f.getFileName()));
    source.deleteRecursively();
}

}
