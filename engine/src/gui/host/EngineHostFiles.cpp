// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/AppPaths.h"
#include "core/graph/AudioGraph.h"
#include "gui/host/EngineHostRecord.h"
#include "io/PatchDocument.h"
#include "gui/host/EngineHostFiles.h"

#include <cstdint>
#include <string>
#include <vector>

#include "hum/caps/Files.h"

namespace hum {

std::int64_t FileHost::playbackPosition(const std::string& name) {
    if (audio_.graph())
        if (auto* fp = dynamic_cast<FileTransportCap*>(audio_.graph()->find(name)))
            return fp->playbackPositionSamples();
    return 0;
}

std::int64_t FileHost::playbackLength(const std::string& name) {
    if (audio_.graph())
        if (auto* fp = dynamic_cast<FileTransportCap*>(audio_.graph()->find(name)))
            return fp->fileLengthSamples();
    return 0;
}

double FileHost::playbackSampleRate(const std::string& name) {
    if (audio_.graph())
        if (auto* fp = dynamic_cast<FileTransportCap*>(audio_.graph()->find(name)))
            return fp->playbackSampleRate();
    return audio_.sampleRate();
}

void FileHost::seek(const std::string& name, std::int64_t sample) {
    if (audio_.graph())
        if (auto* fp = dynamic_cast<FileTransportCap*>(audio_.graph()->find(name)))
            fp->requestSeekSamples(sample);
}

void FileHost::setRecorderActive(const std::string& name, bool on) {
    if (!audio_.graph()) return;
    auto* fr = dynamic_cast<Recorder*>(audio_.graph()->find(name));
    if (!fr) return;
    if (!on) {
        { const juce::ScopedLock sl(audio_.graphLock()); fr->stopRecording(); }
        nodes_.pullVoiceTexts(name);
        return;
    }
    if (fr->isRecording()) return;
    host_.ensureAudio();
    auto* cm = doc_.document().byName(name);
    if (!cm) return;
    auto num = [&](const std::string& n, double def) {
        for (auto& p : cm->properties) if (p.name == n) return p.value;
        return def;
    };
    auto text = [&](const std::string& n) -> std::string {
        for (auto& p : cm->properties) if (p.name == n) return p.text;
        return {};
    };
    auto strip = [](std::string p) { return p.rfind("file://", 0) == 0 ? p.substr(7) : p; };

    const auto takesDir = recording_.recorder().recordingsDir().getChildFile("samples");
    const auto now = juce::Time::getCurrentTime();
    const auto stamped = [&](int track) {
        juce::String stem = juce::String(juce::CharPointer_UTF8(name.c_str())) + "-"
                            + now.formatted("%Y%m%d-%H%M%S") + "-"
                            + juce::String(now.getMilliseconds()).paddedLeft('0', 3);
        if (track > 1) stem += "-" + juce::String(track);
        return takesDir.getChildFile(stem + ".wav")
            .getNonexistentSibling()
            .getFullPathName()
            .toStdString();
    };
    const auto ourOwn = [&](const std::string& path) {
        const juce::File f(juce::String(juce::CharPointer_UTF8(strip(path).c_str())));
        return f.getParentDirectory() == takesDir
               && f.getFileName().startsWith(juce::String(juce::CharPointer_UTF8(name.c_str()))
                                             + "-");
    };

    const int totalCh = fr->channels();
    std::vector<Recorder::RecordTarget> targets;
    std::vector<std::pair<std::string, std::string>> named;
    for (int i = 1; i <= totalCh; ++i) {
        const int cnt = (int) num("RequestedChannelCounts_" + std::to_string(i),
                                  i == 1 ? totalCh : 0);
        if (cnt <= 0) continue;
        const std::string slot = "File_" + std::to_string(i);
        std::string path = text(slot);
        if (path.empty() && i == 1) path = text("File");
        if (path.empty() || ourOwn(path)) {
            path = stamped(i);
            named.push_back({slot, path});
        }
        targets.push_back({strip(path), cnt});
    }
    if (targets.empty()) {
        LiveControlHold live(doc_);
        host_.setParam(name, "Record", 0.0);
        return;
    }
    if (!named.empty()) {
        takesDir.createDirectory();
        for (const auto& [slot, path] : named) host_.setParamText(name, slot, path);
    }
    const int punch = (int) num("PunchMode", 0);
    const double durSec = num("RecordDuration", 0.0) / 1000.0;
    const bool append = num("FileMode", 0) >= 0.5;
    bool began = false;
    {
        const juce::ScopedLock sl(audio_.graphLock());
        began = fr->startRecording(targets, punch, durSec, audio_.sampleRate(), append);
    }
    if (!began) {
        LiveControlHold live(doc_);
        host_.setParam(name, "Record", 0.0);
    }
}

bool FileHost::isRecorderActive(const std::string& name) {
    if (!audio_.graph()) return false;
    auto* fr = dynamic_cast<Recorder*>(audio_.graph()->find(name));
    return fr && fr->isRecording();
}

void FileHost::pollRecorders() {
    std::vector<std::string> stopped;
    for (auto& [n, fr] : recording_.recorders())
        if (fr && fr->consumeAutoStop()) {
            const juce::ScopedLock sl(audio_.graphLock());
            fr->stopRecording();
            stopped.push_back(n);
        }
    if (stopped.empty()) return;
    for (const auto& n : stopped) nodes_.pullVoiceTexts(n);
    LiveControlHold live(doc_);
    for (const auto& n : stopped) host_.setParam(n, "Record", 0.0);
}

void FileHost::liveLooperTracks(const std::string& name, int& recording, int& armed) {
    recording = armed = -1;
    if (audio_.graph())
        if (auto* ll = dynamic_cast<LoopTrackStatus*>(audio_.graph()->find(name))) {
            recording = ll->uiRecordingTrack();
            armed = ll->uiArmedTrack();
        }
}

}
