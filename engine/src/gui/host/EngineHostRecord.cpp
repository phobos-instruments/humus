// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/AppPaths.h"
#include "gui/host/EngineHostRecord.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include "core/graph/AudioGraph.h"
#include "core/graph/PerfBox.h"
#include "core/timeline/RecordTake.h"
#include "gui/host/EngineHost.h"
#include "gui/video/VideoLog.h"
#include "io/WavWriter.h"
#include "hum/caps/Files.h"

#include "hum/dsp/DspMath.h"

namespace hum {

bool RecordHost::armed() const { return host_.automation().isMasterRecord(); }

void RecordHost::setCapturing(bool on) {
    if (on && !capture_.capturing()) capture_.beginCapturePass();
    capture_.setCapturing(on);
}
bool RecordHost::capturing() const { return capture_.capturing(); }

void RecordHost::setArmed(bool on) { host_.automation().setMasterRecord(on); }

juce::File RecordHost::recordingsDir() const {
    const auto root = userLibraryRoot().getChildFile("Recordings");
    const auto project = host_.documentPath().empty()
        ? juce::String("Untitled")
        : juce::File(juce::String(host_.documentPath())).getFileNameWithoutExtension();
    return root.getChildFile(project);
}

void RecordHost::toggle() {
    if (armed() || pendingArm_) {
        if (sessionActive_) endSession();
        if (recording_.preRolling()) { recording_.cancelPreRoll(); recording_.stop(); }
        clearPending();
        setArmed(false);
    } else if (host_.isPlaying()) {
        setArmed(true);
        beginSession();
    } else {
        startRolling(false);
    }
}

bool RecordHost::preRolling() const { return recording_.preRolling() && pendingArm_; }

void RecordHost::startRolling(bool capture) {
    const auto mode = recording_.beforeRecord();
    const int bars = recording_.beforeRecordBars();
    if (mode == BeforeRecord::PreRoll) {
        const double punch = host_.positionBeats();
        const auto meters = host_.automation().meterMap();
        const double start = punch - bars * meters.at(punch).quarterNotesPerBar();
        if (start > 1.0e-6) {
            pendingCapture_ = capture;
            pendingArm_ = true;
            recording_.armPreRoll(punch);
            host_.setPositionBeats(start);
            if (onStartRolling) onStartRolling();
            return;
        }
        recording_.armCountIn(bars);
    } else if (mode == BeforeRecord::CountIn) {
        recording_.armCountIn(bars);
    }
    if (capture) {
        capture_.setCapturing(true);
        capture_.beginCapturePass();
    }
    setArmed(true);
    if (onStartRolling) onStartRolling();
}

void RecordHost::onPunchIn() {
    if (!pendingArm_) return;
    const bool capture = pendingCapture_;
    clearPending();
    if (capture) {
        capture_.setCapturing(true);
        capture_.beginCapturePass();
    }
    setArmed(true);
    if (!sessionActive_) beginSession();
}

void RecordHost::captureToggle() {
    if (capture_.capturing() || sessionActive_ || armed() || pendingArm_) {
        if (sessionActive_) endSession();
        if (recording_.preRolling()) { recording_.cancelPreRoll(); recording_.stop(); }
        clearPending();
        capture_.endCapturePasses();
        capture_.setCapturing(false);
        setArmed(false);
        return;
    }
    if (host_.isPlaying()) {
        capture_.setCapturing(true);
        capture_.beginCapturePass();
        setArmed(true);
        beginSession();
        return;
    }
    startRolling(true);
}

void RecordHost::onPlay() {
    if (armed() && !sessionActive_) beginSession();
}

void RecordHost::onStop() {
    if (sessionActive_) endSession();
}

void RecordHost::beginSession() {
    sessionActive_ = true;
    takes_.clear();
    host_.beginTransaction();
    host_.pushUndo();

    auto* graph = audio_.graph();
    if (!graph) return;
    const auto dir = recordingsDir();
    dir.createDirectory();
    std::vector<std::string> existing;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        existing.push_back(f.getFileName().toStdString());

    for (const auto& node : nodes_.arrangeableNodes()) {
        if (!recordArmed(node)) continue;
        if (nodes_.nodeRecordsVideo(node)) {
            beginVideoTake(node, dir, existing);
            continue;
        }
        auto* cr = dynamic_cast<ClipRecorder*>(graph->find(node));
        if (!cr) continue;
        const auto name = rec::nextTakeName(existing, node);
        const auto path = dir.getChildFile(juce::String(name)).getFullPathName().toStdString();
        existing.push_back(name);
        {
            const juce::ScopedLock sl(audio_.graphLock());
            if (cr->startTake(path, audio_.sampleRate())) takes_.push_back({node, path});
        }
    }

    autoArmed_.clear();
    if (!host_.midi().anyRecordTarget())
        if (const auto note = nodes_.noteCaptureTarget(); !note.empty()) {
            host_.midi().setRecordTarget(note, true);
            autoArmed_.push_back(note);
        }
}

bool RecordHost::recordArmed(const std::string& node) const {
    const auto* cm = doc_.document().byName(node);
    if (cm == nullptr) return false;
    for (const auto& p : cm->properties)
        if (p.name == "Record") return p.value >= 0.5;
    return false;
}

void RecordHost::beginVideoTake(const std::string& node, const juce::File& dir,
                                std::vector<std::string>& existing) {
    if (nodes_.videoSourceInto(node, 0).empty()) {
        videoLog("video take on " + juce::String(node) + " skipped: nothing reaches its inlet");
        return;
    }
    const std::string ext = std::string(".") + movieKindExtension(MovieKind::H264);
    const auto name = rec::nextTakeName(existing, node, ext);
    existing.push_back(name);
    const int height = videotake::heightSetting();
    const int width = videotake::widthFor(height);
    auto recorder = std::make_shared<VideoTakeRecorder>(dir.getChildFile(juce::String(name)),
                                                        width, height, videotake::kFps,
                                                        audio_.sampleRate());
    if (!recorder->ok()) {
        videoLog("video take on " + juce::String(node) + " could not open its writer");
        return;
    }
    VideoTakeStore::instance().open(node, recorder, width, height);
    videoTakes_.push_back({node, std::move(recorder)});
}

void RecordHost::endVideoTakes(double samplesPerBeat) {
    for (const auto& take : videoTakes_) {
        VideoTakeStore::instance().close(take.node);
        take.recorder->stop();
        const auto path = take.recorder->file().getFullPathName().toStdString();
        const double startBeat = take.recorder->takeStartBeat();
        const auto len = take.recorder->takeLengthSamples();
        videoLog("video take on " + juce::String(take.node) + ": "
                 + juce::String((int) take.recorder->frames()) + " frames, "
                 + juce::String(take.recorder->framesWritten()) + " written, "
                 + juce::String(take.recorder->framesDropped()) + " dropped at the queue");
        if (startBeat < 0.0 || len <= 0 || take.recorder->framesWritten() <= 0) {
            take.recorder->file().deleteFile();
            continue;
        }
        double lapBeats[ClipRecorder::kMaxLaps];
        std::int64_t lapSamples[ClipRecorder::kMaxLaps];
        const int nLaps = take.recorder->takeLaps(lapBeats, lapSamples, ClipRecorder::kMaxLaps);
        for (const auto& lap : rec::splitLaps(startBeat, len, lapBeats, lapSamples, nLaps)) {
            const int startTick = (int) std::llround(lap.startBeat * Pattern::kTicksPerBeat);
            const int lenTicks = std::max(1, rec::ticksFromSamples(lap.lengthSamples, samplesPerBeat));
            host_.clips().addVideo(take.node, startTick, lenTicks, path, lap.offsetSamples);
        }
    }
    videoTakes_.clear();
}

void RecordHost::endSession() {
    sessionActive_ = false;
    auto* graph = audio_.graph();
    const double spb = (host_.tempo() > 0.0 ? kSecondsPerMinute / host_.tempo() : 0.5) * audio_.sampleRate();
    endVideoTakes(spb);

    for (const auto& take : takes_) {
        auto* cr = graph ? dynamic_cast<ClipRecorder*>(graph->find(take.node)) : nullptr;
        if (!cr) continue;
        double startBeat = 0.0;
        std::int64_t len = 0;
        double lapBeats[ClipRecorder::kMaxLaps];
        std::int64_t lapSamples[ClipRecorder::kMaxLaps];
        int nLaps = 0;
        {
            const juce::ScopedLock sl(audio_.graphLock());
            cr->stopTake();
            startBeat = cr->takeStartBeat();
            len = cr->takeLengthSamples();
            nLaps = cr->takeLaps(lapBeats, lapSamples, ClipRecorder::kMaxLaps);
        }
        if (startBeat < 0.0 || len <= 0) continue;
        for (const auto& lap : rec::splitLaps(startBeat, len, lapBeats, lapSamples, nLaps)) {
            const int startTick = (int) std::llround(lap.startBeat * Pattern::kTicksPerBeat);
            const int lenTicks = std::max(1, rec::ticksFromSamples(lap.lengthSamples, spb));
            host_.clips().addAudio(take.node, startTick, lenTicks, take.path, lap.offsetSamples);
        }
    }
    takes_.clear();

    recording_.flushMidiRecording(true);
    for (const auto& n : autoArmed_) host_.midi().setRecordTarget(n, false);
    autoArmed_.clear();
    capture_.endCapturePasses();
    capture_.setCapturing(false);
    host_.endTransaction();
    if (onSessionEnded) onSessionEnded();
}

}
