#include "Ph/Ph.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace hum {

void Ph::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
    loadFromFile(params.getText("File"));
}

void Ph::reset() {
    six_.reset();
    chip_.allOff();
    ringL_.fill(0.0f);
    ringR_.fill(0.0f);
    ringWrite_ = 0;
    ringRead_ = 0.0;
}

namespace {

bool endsWithCI(const std::string& s, const char* ext) {
    const size_t n = std::strlen(ext);
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; ++i)
        if (std::tolower((unsigned char) s[s.size() - n + i]) != (unsigned char) ext[i])
            return false;
    return true;
}

}

Ph::Voice Ph::voiceOf(const std::string& bankRef) {
    if (bankRef == kFactoryChip) return Voice::Chip;
    if (bankRef.empty() || bankRef == kFactorySixOp) return Voice::SixOp;
    if (endsWithCI(bankRef, ".syx")) return Voice::SixOp;
    if (endsWithCI(bankRef, ".wopn") || endsWithCI(bankRef, ".tfi")
        || endsWithCI(bankRef, ".dmp"))
        return Voice::Chip;
    return Voice::SixOp;
}

int Ph::patchCount() const {
    return voiceOf(params.getText("File")) == Voice::Chip ? chip_.patchCount()
                                                          : PhSixOp::kSlots;
}

std::string Ph::patchNameAt(int index) const {
    return voiceOf(params.getText("File")) == Voice::Chip ? chip_.patchName(index)
                                                          : six_.patchName(index);
}

void Ph::loadFromFile(const std::string& uri) {
    if (uri == loadedUri_) return;
    loadedUri_ = uri;
    const juce::ScopedLock sl(ioLock_);
    patchSlot_ = -1;
    if (uri.empty() || uri == kFactorySixOp) { six_.useFactoryBank(); return; }
    if (uri == kFactoryChip) { chip_.useFactoryBank(); return; }
    if (endsWithCI(uri, ".syx")) six_.loadSyx(uri);
    else if (endsWithCI(uri, ".wopn")) chip_.loadWopn(uri);
    else if (endsWithCI(uri, ".tfi") || endsWithCI(uri, ".dmp")) chip_.loadTfi(uri);
}

namespace {
std::string opParam(int op, const char* field) {
    return "Op" + std::to_string(op) + "_" + field;
}
}

PhVoice Ph::readVoice(const PhVoice& fromBank) const {
    PhVoice v;
    v.algorithm = (int) params.get("Algorithm", (double) fromBank.algorithm);
    v.feedback = (int) params.get("Feedback", (double) fromBank.feedback);
    for (int i = 1; i <= 6; ++i) {
        const PhVoiceOp& b = fromBank.ops[(size_t) (i - 1)];
        PhVoiceOp& op = v.ops[(size_t) (i - 1)];
        const bool on = params.get(opParam(i, "On"), 1.0) >= 0.5;
        op.level = on ? (int) params.get(opParam(i, "Level"), (double) b.level) : 0;
        op.ratio = (int) params.get(opParam(i, "Ratio"), (double) b.ratio);
        op.detune = (int) params.get(opParam(i, "Detune"), (double) b.detune);
        op.attack = (int) params.get(opParam(i, "Attack"), (double) b.attack);
        op.decay = (int) params.get(opParam(i, "Decay"), (double) b.decay);
        op.sustain = (int) params.get(opParam(i, "Sustain"), (double) b.sustain);
        op.release = (int) params.get(opParam(i, "Release"), (double) b.release);
    }
    return v;
}

bool Ph::voiceParams(std::vector<std::pair<std::string, double>>& out) const {
    const int slot = (int) params.get("Patch", 1.0) - 1;
    const PhVoice v = voiceOf(params.getText("File")) == Voice::Chip ? chip_.voiceAt(slot)
                                                                    : six_.voiceAt(slot);
    out.clear();
    out.emplace_back("Algorithm", (double) v.algorithm);
    out.emplace_back("Feedback", (double) v.feedback);
    for (int i = 1; i <= 6; ++i) {
        const PhVoiceOp& op = v.ops[(size_t) (i - 1)];
        out.emplace_back(opParam(i, "Level"), (double) op.level);
        out.emplace_back(opParam(i, "Ratio"), (double) op.ratio);
        out.emplace_back(opParam(i, "Detune"), (double) op.detune);
        out.emplace_back(opParam(i, "Attack"), (double) op.attack);
        out.emplace_back(opParam(i, "Decay"), (double) op.decay);
        out.emplace_back(opParam(i, "Sustain"), (double) op.sustain);
        out.emplace_back(opParam(i, "Release"), (double) op.release);
    }
    return true;
}

PhMods Ph::readMods() const {
    PhMods m;
    m.bright = params.get("Bright", 0.5);
    m.attack = params.get("Attack", 0.5);
    m.release = params.get("Release", 0.5);
    m.detune = params.get("Detune", 0.0);
    m.vibrato = params.get("Vibrato", 0.0);
    m.speed = params.get("Speed", 0.5);
    return m;
}

void Ph::pumpSixOp() {
    float buf[PhSixOp::kBlock];
    six_.renderBlock(buf);
    for (int i = 0; i < PhSixOp::kBlock; ++i) {
        ringL_[(size_t) (ringWrite_ % kRing)] = buf[i];
        ringR_[(size_t) (ringWrite_ % kRing)] = buf[i];
        ++ringWrite_;
    }
}

void Ph::pumpChip() {
    constexpr int kChunk = 64;
    float l[kChunk], r[kChunk];
    chip_.render(l, r, kChunk);
    for (int i = 0; i < kChunk; ++i) {
        ringL_[(size_t) (ringWrite_ % kRing)] = l[i];
        ringR_[(size_t) (ringWrite_ % kRing)] = r[i];
        ++ringWrite_;
    }
}

void Ph::process(const float* const*, int, float* const* out, int numOut,
                 int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* L = out[0];
    float* R = out[numOut > 1 ? 1 : 0];

    const juce::ScopedTryLock sl(ioLock_);
    if (!sl.isLocked()) {
        std::fill(L, L + numSamples, 0.0f);
        if (R != L) std::fill(R, R + numSamples, 0.0f);
        return;
    }

    const Voice voice = voiceOf(params.getText("File"));
    if (voice != voice_) {
        voice_ = voice;
        patchSlot_ = -1;
        reset();
    }
    const int slots = std::max(1, patchCount());
    const int slot = std::clamp((int) params.get("Patch", 1.0) - 1, 0, slots - 1);
    if (slot != patchSlot_) {
        patchSlot_ = slot;
        if (voice_ == Voice::SixOp) six_.selectPatch(slot);
        else chip_.selectPatch(slot);
        voice_params_ = voice_ == Voice::Chip ? chip_.selectedVoice() : six_.selectedVoice();
    }
    if (const auto m = readMods(); m != mods_) {
        mods_ = m;
        six_.setMods(m);
        chip_.setMods(m);
    }
    if (const auto v = readVoice(voice_params_); v != voice_params_) {
        voice_params_ = v;
        if (voice_ == Voice::SixOp) six_.setVoice(v);
        else chip_.setVoice(v);
    }
    const float level = (float) params.get("Level", 0.8);
    const double bend = std::pow(2.0, params.get("Transpose", 0.0) / 12.0
                                          + params.get("Fine", 0.0) / 1200.0);

    {
        std::lock_guard<std::mutex> g(liveLock_);
        for (int i = 0; i < liveCount_; ++i)
            if (stagedCount_ < (int) staged_.size())
                staged_[(size_t) stagedCount_++] = liveQ_[(size_t) i];
        liveCount_ = 0;
    }
    const auto& tuning = transport.tuning();
    for (int i = 0; i < stagedCount_; ++i) {
        const auto& e = staged_[(size_t) i];
        const int st = e.data[0] & 0xF0;
        if (st == 0x90 && e.data[2] > 0) {
            const double hz = tuning.hz((double) e.data[1]) * bend;
            if (voice_ == Voice::SixOp) six_.noteOn(e.data[1], e.data[2], hz);
            else chip_.noteOn(e.data[1], e.data[2], hz);
        } else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) {
            if (voice_ == Voice::SixOp) six_.noteOff(e.data[1]);
            else chip_.noteOff(e.data[1]);
        }
    }
    stagedCount_ = 0;

    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    const double ratio = (voice_ == Voice::SixOp ? PhSixOp::kRate : PhChip::kRate) / sr;
    const double needUpTo = ringRead_ + (double) numSamples * ratio + 2.0;
    while ((double) ringWrite_ < needUpTo) {
        if (voice_ == Voice::SixOp) pumpSixOp();
        else pumpChip();
    }
    for (int i = 0; i < numSamples; ++i) {
        const auto idx = (long long) ringRead_;
        const float frac = (float) (ringRead_ - (double) idx);
        const auto i0 = (size_t) (idx % kRing);
        const auto i1 = (size_t) ((idx + 1) % kRing);
        const float l = (ringL_[i0] + (ringL_[i1] - ringL_[i0]) * frac) * level;
        const float r = (ringR_[i0] + (ringR_[i1] - ringR_[i0]) * frac) * level;
        if (R != L) { L[i] = l; R[i] = r; }
        else        { L[i] = (l + r) * 0.5f; }
        ringRead_ += ratio;
    }
    if (ringRead_ > 1.0e9) {
        const auto whole = (long long) ringRead_ - (long long) ringRead_ % kRing;
        ringRead_ -= (double) whole;
        ringWrite_ -= (int) whole;
    }
}

}
