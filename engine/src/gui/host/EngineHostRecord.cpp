// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/app/AppPaths.h"
#include "gui/common/UiTicker.h"
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
#include "core/project/ProjectFolder.h"
#include "io/WavWriter.h"
#include "hum/caps/Files.h"

#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr const char* kStopTrigger = "Stop";
constexpr const char* kCompanionSound = "FileSound";
}

bool RecordHost::armed() const { return host_.automation().isMasterRecord(); }

void RecordHost::setCapturing(bool on) {
    if (on && !capture_.capturing()) capture_.beginCapturePass();
    capture_.setCapturing(on);
}
bool RecordHost::capturing() const { return capture_.capturing(); }

void RecordHost::setArmed(bool on) { host_.automation().setMasterRecord(on); }

void RecordHost::sweepEmptyTakeFolders() const {
    const auto shelf = userLibraryRoot().getChildFile("Recordings");
    if (shelf.isDirectory())
        project::pruneEmptyFolders(shelf.getFullPathName().toStdString());
    if (const auto inProject = project::recordingsDirFor(host_.documentPath()); !inProject.empty())
        project::pruneEmptyFolders(inProject);
}

juce::File RecordHost::recordingsDir() const {
    if (const auto inProject = project::recordingsDirFor(host_.documentPath()); !inProject.empty())
        return fileAt(inProject);
    const auto root = userLibraryRoot().getChildFile("Recordings");
    if (!host_.documentPath().empty()) {
        unsaved_.clear();
        return root.getChildFile(fileAt(host_.documentPath()).getFileNameWithoutExtension());
    }
    if (unsaved_.isEmpty()) {
        const auto now = juce::Time::getCurrentTime();
        unsaved_ = juce::String(project::stampedFolderName(now.getYear(), now.getMonth() + 1,
                                                           now.getDayOfMonth(), now.getHours(),
                                                           now.getMinutes(), now.getSeconds()));
    }
    return root.getChildFile(unsaved_);
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
    UiTicker::instance().hold();
    takes_.clear();
    host_.beginUndoGroup();
    host_.pushUndo();

    auto* graph = audio_.graph();
    if (!graph) return;
    const auto dir = recordingsDir();
    dir.createDirectory();
    std::vector<std::string> existing;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        existing.push_back(f.getFileName().toStdString());

    for (const auto& node : nodes_.recorderNodes()) {
        if (!recordArmed(node) || rollingLoose(node)) continue;
        if (nodes_.nodeRecordsVideo(node)) beginVideoTake(node, dir, existing);
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
    armWiredTracks();
}

void RecordHost::armWiredTracks() {
    if (host_.midi().anyInletArmed()) return;
    std::vector<std::string> arm;
    std::vector<InputReach> played;
    for (const auto& r : nodes_.rowsReachedByInput()) {
        if (r.node.empty()) played.push_back(r);
        else if (nodes_.nodeIsMidiTrack(r.node)) arm.push_back(r.node);
    }
    for (const auto& track : nodes_.ensureTracksFeeding(played)) arm.push_back(track);
    for (const auto& track : arm) host_.midi().armInlet(track, true);
}

bool RecordHost::recordArmed(const std::string& node) const {
    const auto* cm = doc_.document().byName(node);
    if (cm == nullptr) return false;
    for (const auto& p : cm->properties)
        if (p.name == "Record") return p.value >= 0.5;
    return false;
}

juce::File RecordHost::freeTakeFile(const juce::File& dir, const std::string& node,
                                    const std::string& ext, std::vector<std::string>& existing) {
    for (int guard = 0; guard < kTakeNameTries; ++guard) {
        const auto name = rec::nextTakeName(existing, node, ext);
        existing.push_back(name);
        const auto file = dir.getChildFile(juce::String(name));
        if (!file.exists()) return file;
    }
    return {};
}

std::shared_ptr<VideoTakeRecorder> RecordHost::openVideoTake(const std::string& node,
                                                             const juce::File& dir,
                                                             std::vector<std::string>& existing,
                                                             bool freeRunning) {
    if (nodes_.videoSourceInto(node, 0).empty()) {
        videoLog("video take on " + juce::String(node) + " skipped: nothing reaches its inlet");
        return nullptr;
    }
    const std::string ext = std::string(".") + videoKindExtension(VideoKind::H264);
    const auto file = freeTakeFile(dir, node, ext, existing);
    if (file == juce::File()) {
        videoLog("video take on " + juce::String(node) + " found no free name to write to");
        return nullptr;
    }
    const int height = videotake::heightSetting();
    const int width = videotake::widthFor(height);
    auto recorder = std::make_shared<VideoTakeRecorder>(file, width, height, videotake::kFps,
                                                        audio_.sampleRate(), freeRunning);
    if (!recorder->ok()) {
        videoLog("video take on " + juce::String(node) + " could not open its writer");
        return nullptr;
    }
    VideoTakeStore::instance().open(node, recorder, width, height);
    return recorder;
}

void RecordHost::beginVideoTake(const std::string& node, const juce::File& dir,
                                std::vector<std::string>& existing) {
    if (auto recorder = openVideoTake(node, dir, existing, false))
        videoTakes_.push_back({node, std::move(recorder)});
}

bool RecordHost::recordsItsOwnPicture(const std::string& node) const {
    if (!nodes_.nodeRecordsVideo(node)) return false;
    auto* live = audio_.graph() ? audio_.graph()->find(node) : nullptr;
    return live != nullptr && dynamic_cast<ClipArrangement*>(live) == nullptr;
}

bool RecordHost::rollingLoose(const std::string& node) const {
    return std::any_of(loose_.begin(), loose_.end(),
                       [&](const VideoTake& t) { return t.node == node; });
}

void RecordHost::noteRecordSwitch(const std::string& node, bool on) {
    if (sessionActive_ || !recordsItsOwnPicture(node)) return;
    if (!on) {
        finishLooseTake(node);
        return;
    }
    if (rollingLoose(node)) return;
    const auto dir = recordingsDir();
    dir.createDirectory();
    const std::string ext = std::string(".") + videoKindExtension(VideoKind::H264);
    std::vector<std::string> existing;
    for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*" + juce::String(ext)))
        existing.push_back(f.getFileName().toStdString());
    auto recorder = openVideoTake(node, dir, existing, true);
    auto* live = audio_.graph() ? audio_.graph()->find(node) : nullptr;
    std::string sound;
    if (auto* clips = dynamic_cast<ClipRecorder*>(live)) {
        std::vector<std::string> heard;
        for (const auto& f : dir.findChildFiles(juce::File::findFiles, false, "*.wav"))
            heard.push_back(f.getFileName().toStdString());
        const auto wav = freeTakeFile(dir, node, ".wav", heard);
        if (wav != juce::File()
            && clips->startTake(wav.getFullPathName().toStdString(), audio_.sampleRate()))
            sound = wav.getFullPathName().toStdString();
    }
    if (recorder == nullptr && sound.empty()) {
        videoLog("take on " + juce::String(node) + ": nothing to record, disarming");
        host_.setParam(node, "Record", 0.0);
        return;
    }
    loose_.push_back({node, std::move(recorder), std::move(sound)});
    if (auto* cap = dynamic_cast<FileTransportCap*>(live))
        if (const auto play = cap->playSwitch(); !play.empty())
            host_.setParam(node, play, 1.0);
}

void RecordHost::noteTransport(const std::string& node, const std::string& param, double value) {
    const auto held = std::find_if(loose_.begin(), loose_.end(),
                                   [&](const VideoTake& t) { return t.node == node; });
    if (held == loose_.end()) return;
    auto* live = audio_.graph() ? audio_.graph()->find(node) : nullptr;
    if (auto* cap = dynamic_cast<FileTransportCap*>(live))
        if (const auto play = cap->playSwitch(); !play.empty() && param == play) {
            if (held->recorder != nullptr) held->recorder->setPaused(value < 0.5);
            return;
        }
    if (param == kStopTrigger && value >= 0.5) {
        finishLooseTake(node);
        host_.setParam(node, "Record", 0.0);
    }
}

void RecordHost::finishLooseTake(const std::string& node) {
    for (auto it = loose_.begin(); it != loose_.end(); ++it) {
        if (it->node != node) continue;
        VideoTakeStore::instance().close(node);
        std::string picture;
        if (it->recorder != nullptr) {
            it->recorder->stop();
            if (it->recorder->framesWritten() > 0)
                picture = it->recorder->file().getFullPathName().toStdString();
            else
                it->recorder->file().deleteFile();
            videoLog("deck take on " + juce::String(node) + ": "
                     + juce::String((int) it->recorder->frames()) + " frames, "
                     + juce::String(it->recorder->framesWritten()) + " written, "
                     + juce::String(it->recorder->framesDropped()) + " dropped at the queue");
        }
        auto* live = audio_.graph() ? audio_.graph()->find(node) : nullptr;
        std::string sound = std::move(it->sound);
        std::int64_t heard = 0;
        if (auto* clips = dynamic_cast<ClipRecorder*>(live); clips != nullptr && !sound.empty()) {
            heard = clips->takeLengthSamples();
            clips->stopTake();
        }
        videoLog("deck take sound on " + juce::String(node) + ": "
                 + juce::String((int) heard) + " frames heard, wav "
                 + juce::String(sound.empty() ? "not opened" : "open"));
        loose_.erase(it);
        if (auto* cap = dynamic_cast<FileTransportCap*>(live))
            if (const auto play = cap->playSwitch(); !play.empty())
                host_.setParam(node, play, 0.0);
        if (!sound.empty() && heard <= 0) {
            fileAt(sound).deleteFile();
            sound.clear();
        }
        if (!picture.empty()) {
            host_.setParamText(node, "File", picture);
            if (!sound.empty()) host_.setParamText(node, kCompanionSound, sound);
        } else if (!sound.empty()) {
            host_.setParamText(node, "File", sound);
        }
        sweepEmptyTakeFolders();
        return;
    }
}

void RecordHost::endVideoTakes(double samplesPerBeat) {
    pictured_.clear();
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
        if (dynamic_cast<ClipArrangement*>(audio_.graph() ? audio_.graph()->find(take.node)
                                                          : nullptr) == nullptr) {
            host_.setParamText(take.node, "File", path);
            pictured_.push_back(take.node);
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
    recording_.flushMidiRecording(true);
    sessionActive_ = false;
    UiTicker::instance().release();
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
        if (dynamic_cast<ClipArrangement*>(graph->find(take.node)) == nullptr) {
            const bool hasPicture = std::find(pictured_.begin(), pictured_.end(), take.node)
                                    != pictured_.end();
            host_.setParamText(take.node, hasPicture ? "FileSound" : "File", take.path);
            continue;
        }
        for (const auto& lap : rec::splitLaps(startBeat, len, lapBeats, lapSamples, nLaps)) {
            const int startTick = (int) std::llround(lap.startBeat * Pattern::kTicksPerBeat);
            const int lenTicks = std::max(1, rec::ticksFromSamples(lap.lengthSamples, spb));
            host_.clips().addAudio(take.node, startTick, lenTicks, take.path, lap.offsetSamples);
        }
    }
    takes_.clear();

    host_.midi().disarmInstruments();
    capture_.endCapturePasses();
    capture_.setCapturing(false);
    for (const auto& node : pictured_) host_.setParam(node, "Record", 0.0);
    host_.endUndoGroup();
    sweepEmptyTakeFolders();
    if (onSessionEnded) onSessionEnded();
}

}
