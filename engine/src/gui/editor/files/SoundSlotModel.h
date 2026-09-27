// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "gui/editor/Words.h"
#include "gui/editor/files/FilePick.h"
#include "gui/editor/files/MediaLibrary.h"
#include "gui/host/ModelHost.h"
#include "io/PatchDocument.h"

namespace hum::files {

inline constexpr Words kMissing{"sound-file-slot.missing", "missing: "};
inline constexpr Words kMissingTip{"sound-file-slot.missing-tip",
                                   "Not found - File > Locate Missing Media, or pick it again here: "};
inline constexpr Words kWrittenHere{"sound-file-slot.written-here", "The take will be written here: "};
inline constexpr Words kNameToWrite{"sound-file-slot.name-a-sound-file-to-write-2", "Name a sound file to write"};
inline constexpr Words kSelectSound{"sound-file-slot.select-a-sound-file-2", "Select a sound file"};

class SoundSlotModel {
public:
    struct Shown {
        std::string path;
        std::string text;
        std::string tooltip;
        bool missing = false;
        bool clearable = false;
        bool revealable = false;
    };

    enum class VideoOffer { No, Also, Only };

    void offersVideos(VideoOffer m) { vids_ = m; }

    SoundSlotModel(ModelHost& host, MediaLibrary& media, std::string organism, std::string param, std::string patterns,
                   std::string title, std::string kind, bool saving)
        : host_(host), media_(media), organism_(std::move(organism)), param_(std::move(param)),
          patterns_(std::move(patterns)), title_(std::move(title)), kind_(std::move(kind)), saving_(saving) {}

    bool saving() const { return saving_; }
    const std::string& param() const { return param_; }
    std::string raw() const { return host_.liveParamText(organism_, param_); }
    std::string resolved(const std::string& ref) const { return media_.resolve(ref, displayClass()); }

    Shown shown() const {
        const auto ref = raw();
        Shown out;
        out.path = stripScheme(ref);
        const bool named = !out.path.empty();
        const bool there = named && media_.exists(resolved(ref));
        out.missing = !saving_ && named && !there;
        const bool awaited = saving_ && named && !there;
        const auto name = fileNameOf(out.path);
        out.text = !named ? "(no file)"
                 : out.missing ? say(host_, kMissing) + name
                               : name;
        out.tooltip = out.missing ? say(host_, kMissingTip) + out.path
                    : awaited ? say(host_, kWrittenHere) + out.path
                              : out.path;
        out.clearable = named;
        const auto here = resolved(out.path);
        out.revealable = named && (media_.isFile(here) || media_.isDirectory(parentOf(here)));
        return out;
    }

    std::string folderKey() const { return vids_ == VideoOffer::Only ? std::string("Videos") : kind_; }

    FilePick request() const {
        FilePick pick;
        const auto cur = stripScheme(raw());
        pick.startDir = !cur.empty() ? parentOf(cur) : saving_ ? media_.recordingDir() : media_.musicDir();
        if (cur.empty() && !saving_) {
            if (const auto d = media_.kindStartDir(kind_); media_.isDirectory(d)) pick.startDir = d;
            if (const auto d = media_.lastFolder(folderKey()); media_.isDirectory(d)) pick.startDir = d;
        }
        pick.patterns = patterns_;
        if (pick.patterns.empty() && !kind_.empty()) pick.patterns = media_.kindPatterns(kind_);
        if (vids_ == VideoOffer::Only) pick.patterns = media_.videoPatterns();
        else {
            if (pick.patterns.empty()) pick.patterns = media_.audioPatterns();
            if (vids_ == VideoOffer::Also) pick.patterns += ";" + media_.videoPatterns();
        }
        pick.title = !title_.empty() ? title_
                   : saving_ ? say(host_, kNameToWrite)
                             : say(host_, kSelectSound);
        pick.save = saving_;
        if (!saving_) pick.current = cur;
        pick.kind = folderKey();
        pick.directories = !saving_ && pick.patterns.find("synScene") != std::string::npos;
        return pick;
    }

    bool chosen(const std::vector<std::string>& paths) {
        if (paths.empty()) return false;
        auto path = paths.front();
        if (saving_ && !hasExtension(path)) path += ".wav";
        if (saving_) media_.rememberRecording(path);
        else media_.rememberFolder(folderKey(), path);
        host_.setParamText(organism_, param_, path);
        return true;
    }

    bool accepts(const std::string& path) const {
        if (vids_ == VideoOffer::Only) return matchesPatterns(fileNameOf(path), media_.videoPatterns());
        const auto want = (patterns_.empty() ? media_.audioPatterns() : patterns_)
                          + (vids_ == VideoOffer::Also ? ";" + media_.videoPatterns() : std::string());
        return matchesPatterns(fileNameOf(path), want);
    }

    void take(const std::string& path) { host_.setParamText(organism_, param_, path); }
    void clear() { host_.setParamText(organism_, param_, ""); }

private:
    std::string displayClass() const {
        const auto* cm = host_.model().byName(organism_);
        return cm != nullptr ? cm->displayClass : std::string();
    }

    ModelHost& host_;
    MediaLibrary& media_;
    std::string organism_, param_;
    std::string patterns_, title_, kind_;
    VideoOffer vids_ = VideoOffer::No;
    bool saving_ = false;
};

}
