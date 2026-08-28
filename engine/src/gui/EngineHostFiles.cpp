#include "core/AppPaths.h"
#include "gui/EngineHost.h"

#include <cstdint>
#include <string>
#include <vector>

#include "hum/Capabilities.h"

namespace hum {

std::int64_t FileHost::playbackPosition(const std::string& name) {
    if (host_.graph_)
        if (auto* fp = dynamic_cast<FileTransportCap*>(host_.graph_->find(name)))
            return fp->playbackPositionSamples();
    return 0;
}

std::int64_t FileHost::playbackLength(const std::string& name) {
    if (host_.graph_)
        if (auto* fp = dynamic_cast<FileTransportCap*>(host_.graph_->find(name)))
            return fp->fileLengthSamples();
    return 0;
}

double FileHost::playbackSampleRate(const std::string& name) {
    if (host_.graph_)
        if (auto* fp = dynamic_cast<FileTransportCap*>(host_.graph_->find(name)))
            return fp->playbackSampleRate();
    return host_.sampleRate_;
}

void FileHost::seek(const std::string& name, std::int64_t sample) {
    if (host_.graph_)
        if (auto* fp = dynamic_cast<FileTransportCap*>(host_.graph_->find(name)))
            fp->requestSeekSamples(sample);
}

void FileHost::setRecorderActive(const std::string& name, bool on) {
    if (!host_.graph_) return;
    auto* fr = dynamic_cast<Recorder*>(host_.graph_->find(name));
    if (!fr) return;
    if (!on) {
        { const juce::ScopedLock sl(host_.lock_); fr->stopRecording(); }
        host_.pullVoiceTexts(name);
        return;
    }
    auto* cm = host_.model_.byName(name);
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

    const int totalCh = fr->channels();
    std::vector<Recorder::RecordTarget> targets;
    for (int i = 1; i <= totalCh; ++i) {
        std::string path = text("File_" + std::to_string(i));
        if (path.empty() && i == 1) path = text("File");
        if (path.empty()) continue;
        const int cnt = (int) num("RequestedChannelCounts_" + std::to_string(i), i == 1 ? totalCh : 0);
        if (cnt > 0) targets.push_back({strip(path), cnt});
    }
    if (targets.empty()) {
        const bool namesItsOwn = std::any_of(
            cm->properties.begin(), cm->properties.end(),
            [](const Parameter& p) { return p.name.rfind("File_", 0) == 0 || p.name == "File"; });
        if (namesItsOwn) return;
        const auto dir = host_.record().recordingsDir().getChildFile("samples");
        dir.createDirectory();
        const auto stem = juce::String(juce::CharPointer_UTF8(name.c_str()))
                          + "-" + juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
        targets.push_back({dir.getChildFile(stem + ".wav").getFullPathName().toStdString(),
                           fr->channels()});
    }
    const int punch = (int) num("PunchMode", 0);
    const double durSec = num("RecordDuration", 0.0) / 1000.0;
    const bool append = num("FileMode", 0) >= 0.5;
    const juce::ScopedLock sl(host_.lock_);
    fr->startRecording(targets, punch, durSec, host_.sampleRate_, append);
}

bool FileHost::isRecorderActive(const std::string& name) {
    if (!host_.graph_) return false;
    auto* fr = dynamic_cast<Recorder*>(host_.graph_->find(name));
    return fr && fr->isRecording();
}

void FileHost::pollRecorders() {
    std::vector<std::string> stopped;
    for (auto& [n, fr] : host_.recorders_)
        if (fr && fr->consumeAutoStop()) {
            const juce::ScopedLock sl(host_.lock_);
            fr->stopRecording();
            stopped.push_back(n);
        }
    if (stopped.empty()) return;
    for (const auto& n : stopped) host_.pullVoiceTexts(n);
    EngineHost::LiveControlScope live(host_);
    for (const auto& n : stopped) host_.setParam(n, "Record", 0.0);
}

void FileHost::liveLooperTracks(const std::string& name, int& recording, int& armed) {
    recording = armed = -1;
    if (host_.graph_)
        if (auto* ll = dynamic_cast<LoopTrackStatus*>(host_.graph_->find(name))) {
            recording = ll->uiRecordingTrack();
            armed = ll->uiArmedTrack();
        }
}

}
