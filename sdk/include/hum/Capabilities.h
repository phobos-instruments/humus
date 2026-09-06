#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hum/Tuning.h"

namespace hum {

class MasterTap {
public:
    virtual ~MasterTap() = default;
    virtual int channels() const = 0;
    virtual const float* channelData(int channel) const = 0;
    virtual int lastBlockLength() const = 0;
    virtual int firstChannel() const { return 0; }
};

class HardwareOut {
public:
    virtual ~HardwareOut() = default;
    virtual int channel() const = 0;
    virtual const float* channelData() const = 0;
    virtual int blockLength() const = 0;
};

class HardwareIn {
public:
    virtual ~HardwareIn() = default;
    virtual int channel() const = 0;
    virtual int channelCount() const { return 1; }
};

class LevelMeterSource {
public:
    virtual ~LevelMeterSource() = default;
    virtual int meterChannels() const = 0;
    virtual float meterLevel(int channel) const = 0;
};

class StereoFieldSource {
public:
    static constexpr int kFieldPairs = 256;
    virtual ~StereoFieldSource() = default;
    virtual int fieldRead(float* lr, int maxPairs) const = 0;
    virtual unsigned fieldStamp() const = 0;
    virtual float fieldCorrelation() const = 0;
};

class VuSource {
public:
    virtual ~VuSource() = default;
    virtual int vuChannels() const = 0;
    virtual float vuRms(int channel) const = 0;
    virtual float vuPeak(int channel) const = 0;
};

class ScopeSource {
public:
    static constexpr int kScopeSamples = 2048;
    virtual ~ScopeSource() = default;
    virtual int scopeRead(float* dst, int maxSamples) const = 0;
    virtual unsigned scopeStamp() const = 0;
    virtual double scopeRate() const = 0;
};

class PitchDetectSource {
public:
    virtual ~PitchDetectSource() = default;
    virtual float detectedHz() const = 0;
    virtual float detectClarity() const = 0;
    virtual float detectLevel() const = 0;
    virtual int detectedNote() const = 0;
};

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

class VoiceSource {
public:
    virtual ~VoiceSource() = default;
    virtual bool voiceParams(std::vector<std::pair<std::string, double>>& out) const = 0;
};

class WantsNullInlets {
public:
    virtual ~WantsNullInlets() = default;
};

class ReloadOnParam {
public:
    virtual ~ReloadOnParam() = default;
    virtual bool reloadsOn(const std::string& param) const = 0;
};

class TextVoiceSource {
public:
    virtual ~TextVoiceSource() = default;
    virtual bool takeVoiceTexts(std::vector<std::pair<std::string, std::string>>& out) = 0;
};

class LiveParamRange {
public:
    virtual ~LiveParamRange() = default;
    virtual bool liveParamRange(const std::string& param, double& lo, double& hi) const = 0;
    virtual bool rangeFollowsFile(const std::string&, const std::string&) const { return true; }
};

class PatchBank {
public:
    virtual ~PatchBank() = default;
    virtual int patchCount() const = 0;
    virtual std::string patchNameAt(int index) const = 0;
};

class CamPreviewSource {
public:
    struct Frame { int width = 0, height = 0; std::vector<std::uint8_t> rgba; };
    virtual ~CamPreviewSource() = default;
    virtual bool camActive() const = 0;
    virtual unsigned camGeneration() const = 0;
    virtual Frame camFrame() const = 0;
    virtual void camSignals(float& x, float& y, float& motion, float& brightness) const = 0;

    struct Skeleton {
        bool supported = false;
        int points = 0;
        int groupSize = 0;
        std::array<std::array<float, 2>, 48> pt{};
        const int (*bones)[2] = nullptr;
        int boneCount = 0;
    };
    virtual Skeleton camSkeleton() const { return {}; }

    virtual std::string camUnavailable() const { return {}; }

    virtual bool camSourceHeld() const { return false; }

    struct NativePicture {
        void* buffer = nullptr;
        int width = 0, height = 0;
        bool mirrored = false;
        std::shared_ptr<const void> hold;
    };
    virtual NativePicture camNativePicture() const { return {}; }
};

class SigilSource {
public:
    static constexpr int kMaxPoints = 16;
    struct Prim {
        unsigned char kind = 0;
        unsigned char role = 1;
        bool closed = false;
        bool filled = false;
        float alpha = 1.0f;
        float size = 0.02f;
        int points = 0;
        float pt[kMaxPoints][2] = {};
    };
    virtual ~SigilSource() = default;
    virtual int sigil(Prim* out, int capacity, double timeSeconds) = 0;
};

class SoundMapSource {
public:
    struct MapPoint { float x = 0.0f, y = 0.0f; int file = 0; };
    virtual ~SoundMapSource() = default;
    virtual unsigned mapGeneration() const = 0;
    virtual const std::vector<MapPoint>& mapPoints() const = 0;
};

class SliceSource {
public:
    virtual ~SliceSource() = default;
    virtual unsigned sliceGeneration() const = 0;
    virtual const std::vector<float>& slicePeaks() const = 0;
    virtual std::vector<float> sliceStarts() const = 0;
    virtual int playingSlice() const = 0;
};

class OscValueSource {
public:
    struct OscVal { const char* suffix; float value; };
    virtual ~OscValueSource() = default;
    virtual bool oscEnabled() const = 0;
    virtual int oscValues(OscVal* out, int capacity) const = 0;
};

class GestureFeatureSource {
public:
    static constexpr int kMaxFeatures = 8;
    virtual ~GestureFeatureSource() = default;
    virtual int gestureFeatureCount() const = 0;
    virtual bool gestureFeaturesLive(float* out) const = 0;
    virtual const char* gestureHoldPrompt() const { return "hold the pose..."; }
    virtual const char* gestureAbsentPrompt() const { return "step into view..."; }
};

class ControlSource {
public:
    struct ControlVal { const char* name; float value; };
    virtual ~ControlSource() = default;
    virtual int controlValues(ControlVal* out, int capacity) const = 0;
};

class VisualSource {
public:
    virtual ~VisualSource() = default;
    virtual void setVisualTapEnabled(bool on) = 0;
    virtual int readVisualTap(float* dest, int maxSamples) const = 0;
};

class VideoNode {
public:
    virtual ~VideoNode() = default;
    virtual int numVideoInputs() const = 0;
    virtual int numVideoOutputs() const = 0;
    virtual unsigned videoLaunchCount() const { return 0; }
};

class VideoFxNode {
public:
    virtual ~VideoFxNode() = default;
};

class VideoTimelineSource {
public:
    struct Cue {
        int clip = -1;
        double seconds = 0.0;
        double rate = 1.0;
        float level = 1.0f;
        bool rolling = false;
    };
    virtual ~VideoTimelineSource() = default;
    virtual Cue cue() const = 0;
    virtual Cue upcoming() const = 0;
    virtual std::string cueFile(int clip) const = 0;

    virtual Cue cueAt(double beat, double tempo) const {
        (void) beat;
        (void) tempo;
        return cue();
    }
    virtual Cue upcomingAt(double beat, double tempo) const {
        (void) beat;
        (void) tempo;
        return upcoming();
    }
};

class VideoPadSource {
public:
    static constexpr int kMaxClips = 8;
    struct ClipState {
        int active = -1;
        int outgoing = -1;
        float phase = 1.0f;
        unsigned launches = 0;
    };
    virtual ~VideoPadSource() = default;
    virtual int clipCount() const = 0;
    virtual ClipState clipState() const = 0;
    virtual void noteClipLength(int, double) {}
};

class RollListener {
public:
    virtual ~RollListener() = default;
    virtual void rolled() = 0;
};

class VideoFrameSink {
public:
    struct Picture {
        int width = 0, height = 0;
        bool bgra = false;
        const std::uint8_t* pixels = nullptr;
        void* native = nullptr;
        std::shared_ptr<const void> hold;
        bool mirrored = false;
    };

    virtual ~VideoFrameSink() = default;
    virtual void pushVideoFrame(const Picture& picture) = 0;
    virtual void setVideoCordAttached(bool) {}
    virtual void setVideoSourceHeld(bool) {}
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

class GainReductionSource {
public:
    virtual ~GainReductionSource() = default;
    virtual float grDb() const = 0;
};

class LatencyReporting {
public:
    virtual ~LatencyReporting() = default;
    virtual int latencySamples() const = 0;
};

struct MidiEvent {
    int sampleOffset = 0;
    unsigned char data[3] = {0, 0, 0};
    int size = 0;
};

class MidiNode {
public:
    static constexpr int kMaxMidiEventsPerBlock = 512;
    virtual ~MidiNode() = default;
    virtual int numMidiInputs() const = 0;
    virtual int numMidiOutputs() const = 0;
    virtual void deliverMidi(int port, const MidiEvent* events, int count) = 0;
    virtual int collectMidi(int port, MidiEvent* out, int capacity) = 0;
};

class LiveMidiIn {
public:
    virtual ~LiveMidiIn() = default;
    virtual void pushLiveMidi(const MidiEvent& e) = 0;
    virtual int liveMidiPort() const { return 0; }
    virtual bool monitorsAllPorts() const { return false; }
    virtual void pushLiveMidi(const MidiEvent& e, int) { pushLiveMidi(e); }
};

class PendingMidiOut {
public:
    virtual ~PendingMidiOut() = default;
    virtual int consumeOutput(MidiEvent* out, int capacity) = 0;
    virtual int pendingMidiPort() const { return 0; }
};

class MidiLogSource {
public:
    struct Logged {
        MidiEvent event;
        unsigned char origin = 0;
        signed char port = -1;
    };
    virtual ~MidiLogSource() = default;
    virtual int consumeLog(Logged* dest, int maxEvents) = 0;
    virtual unsigned logGeneration() const = 0;
};

class OscLogSource {
public:
    struct Logged {
        bool out = false;
        char address[72] = {};
        char args[40] = {};
    };
    virtual ~OscLogSource() = default;
    virtual void pushOsc(bool out, const char* address, const char* args) = 0;
    virtual int consumeOscLog(Logged* dest, int maxEvents) = 0;
    virtual unsigned oscLogGeneration() const = 0;
};

class TuningProvider {
public:
    virtual ~TuningProvider() = default;
    virtual const Tuning& tuning() const = 0;
    virtual void refreshTuning() {}
};

inline std::atomic<unsigned>& renderSeedStore() {
    static std::atomic<unsigned> s{0};
    return s;
}
inline std::atomic<bool>& renderSeedSetStore() {
    static std::atomic<bool> s{false};
    return s;
}
inline void setRenderSeed(unsigned seed) {
    renderSeedStore().store(seed);
    renderSeedSetStore().store(true);
}
inline bool renderSeedIsSet() { return renderSeedSetStore().load(); }
inline unsigned renderSeed() { return renderSeedStore().load(); }

}
