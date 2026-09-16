// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "core/library/BankLibrary.h"
#include "core/timeline/ClipOps.h"
#include "core/library/UserLibrary.h"
#include "io/PatchDocument.h"

namespace hum::media {

struct Ref {
    std::string organism;
    std::string param;
    int channel = -1;
    std::string text;
    juce::File resolved;
    bool exists = false;

    bool isClip() const { return channel >= 0; }
    juce::String leaf() const {
        return resolved != juce::File() ? resolved.getFileName()
                                        : juce::String(juce::CharPointer_UTF8(text.c_str()));
    }
};

inline juce::String stripFileScheme(const std::string& text) {
    juce::String s(juce::CharPointer_UTF8(text.c_str()));
    return s.startsWith("file://") ? s.substring(7) : s;
}

inline bool looksLikeFileRef(const std::string& text) {
    if (text.empty()) return false;
    for (const char* scheme : {"asset:", "library:", "bank:", "file://"})
        if (text.rfind(scheme, 0) == 0) return true;
    const juce::String s(juce::CharPointer_UTF8(text.c_str()));
    if (juce::File::isAbsolutePath(s)) return true;
    if (!s.containsAnyOf("/\\")) return false;
    const auto leaf = s.fromLastOccurrenceOf("/", false, false).fromLastOccurrenceOf("\\", false, false);
    const auto ext = leaf.fromLastOccurrenceOf(".", false, false);
    if (ext == leaf || ext.isEmpty() || ext.length() > 5) return false;
    for (const auto c : ext) if (!juce::CharacterFunctions::isLetterOrDigit(c)) return false;
    return !leaf.containsAnyOf("% ");
}

inline bool bareFileName(const std::string& text) {
    const juce::String s(juce::CharPointer_UTF8(text.c_str()));
    if (s.isEmpty() || s.containsAnyOf("%\t\n\r ") || s.containsAnyOf("/\\")) return false;
    const auto ext = s.fromLastOccurrenceOf(".", false, false);
    if (ext == s || ext.isEmpty() || ext.length() > 5) return false;
    for (const auto c : ext) if (!juce::CharacterFunctions::isLetterOrDigit(c)) return false;
    return true;
}

inline juce::File resolveRef(const std::string& text, const std::string& className,
                             const juce::File& docDir) {
    if (text.empty()) return {};
    if (text.rfind(kAssetScheme, 0) == 0) return resolveAssetRef(juce::String(text));
    if (text.rfind(banks::kLegacyPrefix, 0) == 0) {
        const auto hit = banks::resolve(text, className);
        return hit == text ? juce::File() : juce::File(juce::String(juce::CharPointer_UTF8(hit.c_str())));
    }
    const juce::String s = stripFileScheme(library::resolve(text));
    if (juce::File::isAbsolutePath(s)) return juce::File(s);
    return docDir == juce::File() ? juce::File() : docDir.getChildFile(s);
}

inline std::vector<Ref> refsOf(const PatchDocumentModel& model, const juce::File& docDir) {
    std::vector<Ref> out;
    for (const auto& cm : model.organisms) {
        for (const auto& p : cm.properties) {
            if (p.type != "soundfile" && p.type != "text") continue;
            if (!looksLikeFileRef(p.text) && !(p.type == "soundfile" && bareFileName(p.text))) continue;
            Ref r;
            r.organism = cm.name;
            r.param = p.name;
            r.text = p.text;
            r.resolved = resolveRef(p.text, cm.displayClass, docDir);
            r.exists = r.resolved != juce::File() && r.resolved.exists();
            out.push_back(std::move(r));
        }
        for (int i = 0; i < (int) cm.pattern.channels.size(); ++i) {
            const auto& ch = cm.pattern.channels[(size_t) i];
            if (!(clipops::isAudioClip(ch) || clipops::isVideoClip(ch)) || ch.audioFile.empty()) continue;
            Ref r;
            r.organism = cm.name;
            r.channel = i;
            r.text = ch.audioFile;
            r.resolved = resolveRef(ch.audioFile, cm.displayClass, docDir);
            r.exists = r.resolved != juce::File() && r.resolved.exists();
            out.push_back(std::move(r));
        }
    }
    return out;
}

inline std::vector<Ref> missingOf(const PatchDocumentModel& model, const juce::File& docDir) {
    std::vector<Ref> out;
    for (auto& r : refsOf(model, docDir))
        if (!r.exists) out.push_back(std::move(r));
    return out;
}

inline bool sameRef(const Ref& a, const Ref& b) {
    return a.organism == b.organism && a.param == b.param && a.channel == b.channel;
}

inline std::vector<std::pair<Ref, juce::File>> relocationPlan(const std::vector<Ref>& missing,
                                                              const Ref& located,
                                                              const juce::File& found) {
    std::vector<std::pair<Ref, juce::File>> plan;
    if (found == juce::File() || !found.exists()) return plan;
    plan.push_back({located, found});
    const juce::File oldDir = located.resolved != juce::File() ? located.resolved.getParentDirectory()
                                                                : juce::File();
    const juce::File newDir = found.getParentDirectory();
    if (oldDir == juce::File()) return plan;
    for (const auto& r : missing) {
        if (sameRef(r, located) || r.resolved == juce::File()) continue;
        juce::File candidate;
        if (r.resolved.getParentDirectory() == oldDir)
            candidate = newDir.getChildFile(r.resolved.getFileName());
        else if (r.resolved.isAChildOf(oldDir))
            candidate = newDir.getChildFile(r.resolved.getRelativePathFrom(oldDir));
        if (candidate != juce::File() && candidate.exists()) plan.push_back({r, candidate});
    }
    return plan;
}

}
