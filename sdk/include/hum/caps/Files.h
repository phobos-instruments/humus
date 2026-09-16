// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hum {

class Recorder {
public:
    struct RecordTarget { std::string path; int channels = 1; };
    virtual ~Recorder() = default;
    virtual bool startRecording(const std::vector<RecordTarget>& targets, int punchMode,
                                double durationSeconds, double sampleRate, bool append) = 0;
    virtual void stopRecording() = 0;
    virtual bool isRecording() const = 0;
    virtual bool consumeAutoStop() = 0;
    virtual int channels() const = 0;
};

class FileLoader {
public:
    virtual ~FileLoader() = default;
    virtual void loadFromFile(const std::string& uri) = 0;
};

class LiveCaptureSource {
public:
    struct Take { int slot = 0; std::string path; };
    virtual ~LiveCaptureSource() = default;
    virtual bool fetchCompletedTake(Take& out) = 0;
};

class FileTransportCap : public FileLoader {
public:
    virtual std::int64_t playbackPositionSamples() const = 0;
    virtual std::int64_t fileLengthSamples() const = 0;
    virtual double playbackSampleRate() const = 0;
    virtual void requestSeekSamples(std::int64_t sample) = 0;
};

class DeckControl : public FileTransportCap {
public:
    virtual double effectiveBpm() const = 0;
    virtual void setBendPercent(double percent) = 0;
    virtual void setScrub(bool active, double targetSample) = 0;
    virtual const std::vector<float>& waveformPeaks() const = 0;
    virtual void beginLoopRoll() = 0;
    virtual void endLoopRoll() = 0;
};

class LoopTrackStatus {
public:
    virtual ~LoopTrackStatus() = default;
    virtual int uiRecordingTrack() const = 0;
    virtual int uiArmedTrack() const = 0;
};

class StrandStatus {
public:
    enum { kEmpty = 0, kRecord = 1, kPlay = 2, kDub = 3, kStopped = 4 };
    virtual ~StrandStatus() = default;
    virtual int strandCount() const = 0;
    virtual int strandState(int strand) const = 0;
    virtual float strandPhase(int strand) const = 0;
    virtual int strandLayers(int strand) const = 0;
    virtual bool strandPending(int strand) const = 0;
};

class ClipArrangement {
public:
    virtual ~ClipArrangement() = default;
};

class SessionAudio {
public:
    virtual ~SessionAudio() = default;
    virtual bool storeSessionAudio(const std::string& pathPrefix,
                                   std::vector<std::pair<std::string, std::string>>& out) = 0;
    virtual void loadSessionAudio() = 0;
};

class ClipRecorder {
public:
    static constexpr int kMaxLaps = 64;
    virtual ~ClipRecorder() = default;
    virtual bool startTake(const std::string& wavPath, double sampleRate) = 0;
    virtual void stopTake() = 0;
    virtual bool takeActive() const = 0;
    virtual double takeStartBeat() const = 0;
    virtual std::int64_t takeLengthSamples() const = 0;
    virtual int takeLaps(double* beats, std::int64_t* samples, int maxLaps) const = 0;
    virtual void ensureClipsLoaded() = 0;
};

}
