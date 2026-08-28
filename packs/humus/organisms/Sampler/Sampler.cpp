#include "Sampler/Sampler.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

#include "hum/Registry.h"
#include "hum/dsp/Sf2File.h"
#include "hum/dsp/SoundFileBuffer.h"

namespace hum {

namespace {
constexpr float kEnvFloor = 1.0e-4f;

float segCoef(double ms, double sr) {
    const double samples = std::max(1.0, ms * 0.001 * sr / 3.0);
    return (float) std::exp(-1.0 / samples);
}
}

namespace { constexpr double kMaxCaptureSeconds = 30.0; }

void Sampler::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    rootSeed_.fill(-1);
    capture_.setSize(2, (int) (kMaxCaptureSeconds * std::max(8000.0, sampleRate)));
    capture_.clear();
    inMeter_.prepare(sampleRate);
    loadFromFile({});
    applyPending();
    reset();
}

void Sampler::reset() {
    for (auto& v : voices_) v = Voice{};
    stagedCount_ = 0;
}

static void crunch(juce::AudioBuffer<float>& buf, double& srcRate, int bits, double rateHz) {
    if (buf.getNumSamples() < 2) return;
    if (rateHz > 0.0 && srcRate > rateHz) {
        const double step = srcRate / rateHz;
        const int n = std::max(2, (int) (buf.getNumSamples() / step));
        juce::AudioBuffer<float> thin(buf.getNumChannels(), n);
        for (int c = 0; c < buf.getNumChannels(); ++c) {
            const float* src = buf.getReadPointer(c);
            float* dst = thin.getWritePointer(c);
            for (int i = 0; i < n; ++i)
                dst[i] = src[std::min(buf.getNumSamples() - 1, (int) (i * step))];
        }
        buf = std::move(thin);
        srcRate = rateHz;
    }
    if (bits >= 24) return;
    const float steps = (float) (1 << (bits - 1));
    for (int c = 0; c < buf.getNumChannels(); ++c) {
        float* w = buf.getWritePointer(c);
        for (int i = 0; i < buf.getNumSamples(); ++i)
            w[i] = std::round(std::clamp(w[i], -1.0f, 1.0f) * steps) / steps;
    }
}

void Sampler::loadFromFile(const std::string&) {
    const int bits = std::clamp((int) params.get("Bits", 24.0), 8, 24);
    const int rate = std::clamp((int) params.get("Rate", 48.0), 4, 48);
    if (bits != lastBits_ || rate != lastRate_) {
        lastBits_ = bits;
        lastRate_ = rate;
        slotUri_.fill(std::string{});
        bankUri_.clear();
    }
    for (int i = 0; i < kSlots; ++i) {
        const std::string uri = resolvePath(
            params.getText("File" + std::to_string(i + 1)), "Sampler");
        if (uri == slotUri_[(size_t) i]) continue;
        slotUri_[(size_t) i] = uri;
        juce::AudioBuffer<float> buf;
        SoundFileInfo info;
        if (!uri.empty()) loadSoundFile(uri, buf, info);
        slotCache_[(size_t) i].buf = std::move(buf);
        slotCache_[(size_t) i].srcRate = info.sampleRate > 0.0 ? info.sampleRate : sampleRate_;
        crunch(slotCache_[(size_t) i].buf, slotCache_[(size_t) i].srcRate,
               bits, rate * 1000.0);
        slotCache_[(size_t) i].uri = uri;
        slotInfo_[(size_t) i] = info;
        if (info.rootKey >= 0) rootSeed_[(size_t) i] = info.rootKey;
    }

    const std::string bankRef =
        resolvePath(params.getText("FileBank"), "Sampler");
    if (bankRef != bankUri_) {
        bankUri_ = bankRef;
        bankCache_ = Kit{};
        if (sf2::isSf2Path(bankRef)) {
            buildBank(bankRef, bankCache_);
            for (auto& sd : bankCache_.samples) crunch(sd.buf, sd.srcRate, bits, rate * 1000.0);
        }
    }
    if (!bankCache_.presetNames.empty()) {
        publish(Kit(bankCache_));
        return;
    }

    Kit kit;
    for (int i = 0; i < kSlots; ++i) {
        if (slotCache_[(size_t) i].buf.getNumSamples() == 0) continue;
        kit.samples.push_back(slotCache_[(size_t) i]);
        Zone z;
        z.sample = (int) kit.samples.size() - 1;
        z.slot = i;
        z.loopStart = slotInfo_[(size_t) i].loopStart;
        z.loopEnd = slotInfo_[(size_t) i].loopEnd;
        kit.zones.push_back(z);
    }
    publish(std::move(kit));
}

void Sampler::publish(Kit&& kit) {
    delete pending_.exchange(new Kit(std::move(kit)));
}

void Sampler::buildBank(const std::string& uri, Kit& out) {
    auto path = uri;
    if (path.rfind("file://", 0) == 0) path = path.substr(7);
    const auto bank = sf2::loadFile(juce::File(juce::String(juce::CharPointer_UTF8(path.c_str()))));
    if (!bank.parsed) return;

    std::vector<int> mapped((size_t) bank.samples.size(), -1);
    auto sampleFor = [&](int index) {
        if (index < 0 || index >= (int) bank.samples.size()) return -1;
        if (mapped[(size_t) index] >= 0) return mapped[(size_t) index];
        const auto& src = bank.samples[(size_t) index];
        const int len = (int) src.end - (int) src.start;
        if (len <= 1 || (std::size_t) src.end > bank.pcm.size()) return -1;
        SampleData sd;
        sd.srcRate = src.sampleRate > 0 ? (double) src.sampleRate : sampleRate_;
        sd.uri = uri + "#" + std::to_string(index);
        sd.buf.setSize(1, len);
        float* w = sd.buf.getWritePointer(0);
        for (int i = 0; i < len; ++i)
            w[i] = (float) bank.pcm[(std::size_t) src.start + (std::size_t) i] / 32768.0f;
        out.samples.push_back(std::move(sd));
        mapped[(size_t) index] = (int) out.samples.size() - 1;
        return mapped[(size_t) index];
    };

    for (int pi = 0; pi < (int) bank.presets.size(); ++pi) {
        out.presetNames.push_back(bank.presets[(size_t) pi].name);
        for (const auto& sz : bank.presets[(size_t) pi].zones) {
            const int sample = sampleFor(sz.sampleIndex);
            if (sample < 0) continue;
            const auto& src = bank.samples[(size_t) sz.sampleIndex];
            Zone z;
            z.sample = sample;
            z.preset = pi;
            z.keyLo = sz.keyLo; z.keyHi = sz.keyHi;
            z.velLo = sz.velLo; z.velHi = sz.velHi;
            z.rootKey = sz.rootKey >= 0 ? sz.rootKey : src.originalKey;
            z.tuneCents = sz.coarseTune * 100.0 + sz.fineTune + src.correction;
            z.loopMode = sz.sampleModes;
            z.loopStart = (int) src.loopStart - (int) src.start;
            z.loopEnd = (int) src.loopEnd - (int) src.start;
            z.gain = (float) sf2::centibelsToGain(sz.attenuationCb);
            const double pos = std::clamp(sz.panTenthPct / 500.0, -1.0, 1.0);
            z.panL = (float) std::sqrt(std::clamp(0.5 - pos * 0.5, 0.0, 1.0)) * 1.41421356f;
            z.panR = (float) std::sqrt(std::clamp(0.5 + pos * 0.5, 0.0, 1.0)) * 1.41421356f;
            z.attackMs = sf2::timecentsToSeconds(sz.attackTc) * 1000.0;
            z.decayMs = sf2::timecentsToSeconds(sz.decayTc) * 1000.0;
            z.releaseMs = sf2::timecentsToSeconds(sz.releaseTc) * 1000.0;
            z.sustain = (float) sf2::centibelsToGain(sz.sustainCb);
            out.zones.push_back(z);
        }
    }
    if (out.zones.empty()) out = Kit{};
}

bool Sampler::voiceParams(std::vector<std::pair<std::string, double>>& out) const {
    for (int i = 0; i < kSlots; ++i) {
        if (rootSeed_[(size_t) i] < 0) continue;
        out.push_back({"Root" + std::to_string(i + 1), (double) rootSeed_[(size_t) i]});
        rootSeed_[(size_t) i] = -1;
    }
    return !out.empty();
}

int Sampler::selectedPreset() const {
    const int n = (int) kit_.presetNames.size();
    return n == 0 ? -1 : std::clamp((int) params.get("Preset", 1.0) - 1, 0, n - 1);
}

void Sampler::applyPending() {
    Kit* fresh = pending_.exchange(nullptr);
    if (fresh == nullptr) return;

    std::array<int, (size_t) kMaxVoices> moved{};
    for (size_t i = 0; i < voices_.size(); ++i) {
        moved[i] = -1;
        const auto& v = voices_[i];
        if (v.stage == 0 || v.zone < 0 || v.zone >= (int) kit_.zones.size()) continue;
        const auto& was = kit_.zones[(size_t) v.zone];
        for (int z = 0; z < (int) fresh->zones.size(); ++z) {
            const auto& now = fresh->zones[(size_t) z];
            if (now.slot != was.slot) continue;
            if (fresh->samples[(size_t) now.sample].uri
                == kit_.samples[(size_t) was.sample].uri)
                moved[i] = z;
            break;
        }
    }
    kit_ = std::move(*fresh);
    delete fresh;
    for (size_t i = 0; i < voices_.size(); ++i) {
        if (voices_[i].stage == 0) continue;
        if (moved[i] < 0) voices_[i] = Voice{};
        else voices_[i].zone = moved[i];
    }
}

int Sampler::rootOf(const Zone& z) const {
    return z.slot >= 0 ? (int) params.get("Root" + std::to_string(z.slot + 1), 60.0)
                       : z.rootKey;
}

bool Sampler::startRecording(const std::vector<RecordTarget>& targets, int,
                             double durationSeconds, double sampleRate, bool) {
    if (targets.empty() || targets[0].path.empty()) return false;
    capturePath_ = targets[0].path;
    captureSlot_ = std::clamp((int) params.get("RecSlot", 1.0), 1, kSlots);
    const double sr = sampleRate > 0.0 ? sampleRate : sampleRate_;
    const double want = durationSeconds > 0.0 ? durationSeconds : kMaxCaptureSeconds;
    captureLimit_.store(std::min(capture_.getNumSamples(),
                                 (int) (std::min(want, kMaxCaptureSeconds) * sr)));
    capturePos_.store(0);
    captureReady_ = false;
    waitingForLevel_.store(params.get("Threshold", 0.0) > 0.0);
    capturing_.store(true);
    return true;
}

void Sampler::stopRecording() {
    waitingForLevel_.store(false);
    capturing_.store(false);
    captureReady_ = capturePos_.load() > 0;
}

bool Sampler::takeVoiceTexts(std::vector<std::pair<std::string, std::string>>& out) {
    if (!captureReady_) return false;
    captureReady_ = false;
    const int n = std::min(capturePos_.load(), capture_.getNumSamples());
    if (n <= 0 || capturePath_.empty()) return false;

    if (!writeSoundFile(capturePath_, capture_, sampleRate_, n)) return false;

    out.push_back({"File" + std::to_string(captureSlot_), capturePath_});
    return true;
}

void Sampler::deliverMidi(int, const MidiEvent* events, int count) {
    stagedCount_ = std::min(count, (int) staged_.size());
    for (int i = 0; i < stagedCount_; ++i) staged_[(size_t) i] = events[i];
}

void Sampler::pushLiveMidi(const MidiEvent& e) {
    std::lock_guard<std::mutex> g(liveLock_);
    if (liveCount_ < (int) liveQ_.size()) liveQ_[(size_t) liveCount_++] = e;
}

int Sampler::pickZone(int note, bool kit) const {
    int best = -1, bestDist = 1 << 30;
    for (int z = 0; z < (int) kit_.zones.size(); ++z) {
        const int root = rootOf(kit_.zones[(size_t) z]);
        const int dist = std::abs(note - root);
        if (kit ? dist != 0 : dist >= bestDist) continue;
        best = z;
        bestDist = dist;
        if (kit) break;
    }
    return best;
}

void Sampler::noteOn(int note, float velocity) {
    if (bankLoaded()) {
        const int sel = selectedPreset();
        const int vel = std::clamp((int) std::lround(velocity * 127.0f), 0, 127);
        for (int z = 0; z < (int) kit_.zones.size(); ++z) {
            const Zone& zone = kit_.zones[(size_t) z];
            if (zone.preset != sel) continue;
            if (note < zone.keyLo || note > zone.keyHi) continue;
            if (vel < zone.velLo || vel > zone.velHi) continue;
            startVoice(zone, z, note, velocity);
        }
        return;
    }
    const bool kit = params.get("Mode", 0.0) >= 0.5;
    const int z = pickZone(note, kit);
    if (z < 0) return;
    startVoice(kit_.zones[(size_t) z], z, note, velocity);
}

void Sampler::startVoice(const Zone& zone, int zoneIndex, int note, float velocity) {
    const bool kit = !bankLoaded() && params.get("Mode", 0.0) >= 0.5;

    const int poly = std::clamp((int) params.get("Polyphony", 8.0), 1, kMaxVoices);
    Voice* slot = nullptr;
    int active = 0;
    for (auto& v : voices_) {
        if (v.stage == 0) { if (!slot) slot = &v; }
        else ++active;
    }
    if (!slot || active >= poly) {
        slot = &voices_[0];
        for (auto& v : voices_)
            if (v.stage != 0 && v.age < slot->age) slot = &v;
    }

    const int root = rootOf(zone);
    const double transpose =
        kit ? 1.0 : std::pow(2.0, (note - root) / 12.0 + zone.tuneCents / 1200.0);
    *slot = Voice{};
    slot->zone = zoneIndex;
    slot->note = note;
    slot->rate = transpose * (kit_.samples[(size_t) zone.sample].srcRate
                              / (sampleRate_ > 0.0 ? sampleRate_ : 44100.0));
    slot->gain = (0.1f + 0.9f * std::clamp(velocity, 0.0f, 1.0f)) * zone.gain;
    slot->panL = zone.panL;
    slot->panR = zone.panR;
    slot->stage = 1;
    slot->age = ++voiceClock_;
    updateVoice(*slot);
}

void Sampler::refreshVoiceEnvelopes() {
    for (auto& v : voices_) {
        if (v.stage == 0) continue;
        if (v.zone < 0 || v.zone >= (int) kit_.zones.size()) { v = Voice{}; continue; }
        updateVoice(v);
    }
}

void Sampler::updateVoice(Voice& v) {
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    const double aMul = params.get("Attack", 2.0) / 2.0;
    const double dMul = params.get("Decay", 120.0) / 120.0;
    const double rMul = params.get("Release", 150.0) / 150.0;
    const float sMul = (float) std::clamp(params.get("Sustain", 1.0), 0.0, 1.0);
    const bool knobLoop = params.get("Loop", 0.0) >= 0.5;
    {
        const Zone& z = kit_.zones[(size_t) v.zone];
        v.attackInc = (float) (1.0 / std::max(1.0, z.attackMs * aMul * 0.001 * sr));
        v.decayCoef = segCoef(z.decayMs * dMul, sr);
        v.releaseCoef = segCoef(z.releaseMs * rMul, sr);
        v.sustain = std::clamp(z.sustain * sMul, 0.0f, 1.0f);

        const int len = kit_.samples[(size_t) z.sample].buf.getNumSamples();
        v.endIdx = len - 1;
        const bool looping = z.loopMode < 0 ? knobLoop : (z.loopMode & 1) != 0;
        const bool hasPoints = z.loopEnd > z.loopStart;
        const int lo = hasPoints ? std::max(0, z.loopStart) : 0;
        const int hi = hasPoints ? std::min(z.loopEnd, len - 1) : len - 1;
        v.loopLen = looping && hi > lo ? hi - lo : 0;
        if (hasPoints && v.loopLen > 0) v.endIdx = hi;
    }
}

void Sampler::noteOff(int note) {
    for (auto& v : voices_)
        if (v.note == note && (v.stage == 1 || v.stage == 2)) v.stage = 3;
}

void Sampler::allOff(bool hard) {
    for (auto& v : voices_) {
        if (v.stage == 0) continue;
        if (hard) v = Voice{};
        else v.stage = 3;
    }
}

void Sampler::handleEvent(const MidiEvent& e) {
    if (e.size < 2) return;
    const unsigned char status = e.data[0] & 0xF0;
    if (status == 0x90 && e.size >= 3 && e.data[2] > 0)
        noteOn(e.data[1], (float) e.data[2] / 127.0f);
    else if (status == 0x80 || (status == 0x90 && e.size >= 3))
        noteOff(e.data[1]);
    else if (status == 0xB0 && e.size >= 3 && (e.data[1] == 123 || e.data[1] == 120))
        allOff(e.data[1] == 120);
}

void Sampler::renderAdd(float* left, float* right, int numSamples) {
    const float master = (float) params.get("Gain", 1.0);
    for (auto& v : voices_) {
        if (v.stage == 0) continue;
        if (v.zone < 0 || v.zone >= (int) kit_.zones.size()) { v = Voice{}; continue; }
        const auto& buf = kit_.samples[(size_t) kit_.zones[(size_t) v.zone].sample].buf;
        const int len = buf.getNumSamples();
        if (len == 0) { v = Voice{}; continue; }
        const int chans = buf.getNumChannels();
        const float* srcL = buf.getReadPointer(0);
        const float* srcR = buf.getReadPointer(chans > 1 ? 1 : 0);

        for (int n = 0; n < numSamples; ++n) {
            if (v.stage == 1) {
                v.env += v.attackInc;
                if (v.env >= 1.0f) { v.env = 1.0f; v.stage = 2; }
            } else if (v.stage == 2) {
                v.env = v.sustain + v.decayCoef * (v.env - v.sustain);
            } else {
                v.env *= v.releaseCoef;
                if (v.env < kEnvFloor) { v = Voice{}; break; }
            }
            const int i0 = (int) v.pos;
            if (i0 >= v.endIdx) {
                if (v.loopLen > 0) { v.pos -= v.loopLen; continue; }
                v = Voice{};
                break;
            }
            const float frac = (float) (v.pos - i0);
            const float g = v.env * v.gain * master;
            left[n]  += g * v.panL * (srcL[i0] + frac * (srcL[i0 + 1] - srcL[i0]));
            right[n] += g * v.panR * (srcR[i0] + frac * (srcR[i0 + 1] - srcR[i0]));
            v.pos += v.rate;
        }
    }
}

void Sampler::process(const float* const* in, int numIn, float* const* out, int numOut,
                      int numSamples, const Transport&) {

    inMeter_.measure(in, std::min(numIn, 2), numSamples);

    if (capturing_.load() && waitingForLevel_.load()) {
        const float open = (float) params.get("Threshold", 0.0);
        float peak = 0.0f;
        for (int c = 0; c < std::min(numIn, 2); ++c)
            if (in && in[c])
                for (int i = 0; i < numSamples; ++i) peak = std::max(peak, std::fabs(in[c][i]));
        if (peak >= open) waitingForLevel_.store(false);
    }
    if (capturing_.load() && !waitingForLevel_.load()) {
        const int pos = capturePos_.load();
        const int room = std::min(captureLimit_.load(), capture_.getNumSamples()) - pos;
        const int n = std::min(numSamples, std::max(0, room));
        for (int c = 0; c < 2; ++c) {
            float* dst = capture_.getWritePointer(c) + pos;
            if (numIn > 0) std::memcpy(dst, in[std::min(c, numIn - 1)], sizeof(float) * (size_t) n);
            else std::memset(dst, 0, sizeof(float) * (size_t) n);
        }
        capturePos_.store(pos + n);
        if (n < numSamples) { capturing_.store(false); autoStop_.store(true); }
    }
    for (int c = 0; c < numOut; ++c) std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
    if (numOut == 0) { stagedCount_ = 0; return; }

    if (params.get("Monitor", 0.0) >= 0.5 && numIn > 0)
        for (int c = 0; c < numOut; ++c) {
            const float* src = in[std::min(c, numIn - 1)];
            if (src) juce::FloatVectorOperations::add(out[c], src, numSamples);
        }
    applyPending();

    int liveN = 0;
    MidiEvent live[128];
    {
        std::unique_lock<std::mutex> g(liveLock_, std::try_to_lock);
        if (g.owns_lock()) {
            liveN = liveCount_;
            for (int i = 0; i < liveN; ++i) { live[i] = liveQ_[(size_t) i]; live[i].sampleOffset = 0; }
            liveCount_ = 0;
        }
    }
    for (int i = 0; i < liveN; ++i) handleEvent(live[i]);
    refreshVoiceEnvelopes();

    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];
    int cursor = 0;
    for (int i = 0; i < stagedCount_; ++i) {
        const int at = std::clamp(staged_[(size_t) i].sampleOffset, 0, numSamples);
        if (at > cursor) { renderAdd(L + cursor, R + cursor, at - cursor); cursor = at; }
        handleEvent(staged_[(size_t) i]);
    }
    if (cursor < numSamples) renderAdd(L + cursor, R + cursor, numSamples - cursor);
    stagedCount_ = 0;
}

}
