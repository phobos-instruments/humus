// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include "core/app/AppPaths.h"
#include "core/packs/Catalogue.h"
#include "gui/app/StartDirs.h"
#include "gui/editor/files/MediaLibrary.h"
#include "io/FfmpegAudioFormat.h"
#include "gui/host/BrickHost.h"
#include "hum/PixelFieldImage.h"
#include "io/MediaRefs.h"

namespace hum {

inline juce::File juceFileAt(const std::string& path) {
    return path.empty() ? juce::File() : juce::File(juce::String::fromUTF8(path.c_str()));
}

inline juce::File kindDirFor(const std::string& kind) {
    return userContentRoot().getChildFile(kind.empty() ? "Samples" : kind);
}

inline juce::File kindStartDirFor(const std::string& kind) {
    const auto mine = kindDirFor(kind);
    if (mine.isDirectory() && !mine.findChildFiles(juce::File::findFilesAndDirectories, false, "*").isEmpty())
        return mine;
    for (const auto& d : assetSearchPath(kind.empty() ? "Samples" : kind))
        if (d != mine && d.isDirectory()) return d;
    return mine;
}

inline std::string juceAudioPatterns() {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    return fm.getWildcardForAllFormats().toStdString();
}

class JuceMediaLibrary : public files::MediaLibrary {
public:
    explicit JuceMediaLibrary(BrickHost& host) : host_(host) {}

    std::string resolve(const std::string& ref, const std::string& displayClass) const override {
        return media::resolveRef(ref, displayClass, juceFileAt(host_.documentFolder())).getFullPathName().toStdString();
    }
    bool exists(const std::string& path) const override { return juceFileAt(path).exists(); }
    bool isFile(const std::string& path) const override { return juceFileAt(path).existsAsFile(); }
    bool isDirectory(const std::string& path) const override { return juceFileAt(path).isDirectory(); }

    std::string audioPatterns() const override { return juceAudioPatterns(); }
    std::string videoPatterns() const override {
        auto all = files::MediaLibrary::videoPatterns();
        if (FfmpegAudioFormat::available()) all += ";" + FfmpegAudioFormat::extraContainers();
        return all;
    }
    std::string kindPatterns(const std::string& kind) const override {
        const auto* k = catalogue::find(kind);
        return k != nullptr ? k->wildcard : std::string();
    }
    std::string kindStartDir(const std::string& kind) const override {
        return kindStartDirFor(kind).getFullPathName().toStdString();
    }
    std::string recordingDir() const override {
        return startDirFor(DirPurpose::Recording).getFullPathName().toStdString();
    }
    std::string musicDir() const override {
        return juce::File::getSpecialLocation(juce::File::userMusicDirectory).getFullPathName().toStdString();
    }
    void rememberRecording(const std::string& path) override {
        rememberDirFor(DirPurpose::Recording, juceFileAt(path));
    }
    std::string lastFolder(const std::string& kind) const override {
        return keptFolder(mediaFolderKey(kind)).getFullPathName().toStdString();
    }
    void rememberFolder(const std::string& kind, const std::string& path) override {
        keepFolder(mediaFolderKey(kind), juceFileAt(path));
    }

    bool loadPicture(const std::string& path, PixelField& out, int maxWidth, int maxHeight) const override {
        return loadPixelField(path, out, maxWidth, maxHeight);
    }

private:
    BrickHost& host_;
};

}
