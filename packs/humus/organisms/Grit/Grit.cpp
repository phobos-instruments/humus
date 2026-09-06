#include "Grit/GritImpl.h"

#include <cmath>

namespace hum {

Grit::Grit() : impl_(std::make_unique<Impl>()) {
    impl_->dmc.SetAPU(&impl_->apu);
    impl_->dmc.SetMemory(&impl_->rom);
}

Grit::~Grit() = default;

void Grit::prepare(double sampleRate, int) {
    sampleRate_ = sampleRate;
    reset();
}

void Grit::reset() {
    pal_ = params.get("Region", 0.0) >= 0.5;
    cachedCart_ = -1;
    cachedWave_ = -1;
    cachedPatch_ = -1;
    cachedDuty_ = -1;
    cachedSample_.clear();
    cycleAcc_ = 0.0;
    envAcc_ = 0.0;
    for (auto& v : voices_) v = Voice{};
    noiseEnv_ = Voice{};
    triNote_ = -1;
    triHi_ = -1;
    noiseNote_ = -1;
    age_ = 0;
    applyRegion();
}

void Grit::pokeApu(int reg, int value) {
    impl_->apu.Write((xgm::UINT32) (0x4000 + reg), (xgm::UINT32) value);
    impl_->dmc.Write((xgm::UINT32) (0x4000 + reg), (xgm::UINT32) value);
}

void Grit::applyRegion() {
    const double c = clock();
    const double sr = sampleRate_ > 0.0 ? sampleRate_ : 44100.0;
    for (xgm::ISoundChip* chip :
         {(xgm::ISoundChip*) &impl_->apu, (xgm::ISoundChip*) &impl_->dmc,
          (xgm::ISoundChip*) &impl_->vrc6, (xgm::ISoundChip*) &impl_->mmc5,
          (xgm::ISoundChip*) &impl_->fds, (xgm::ISoundChip*) &impl_->n163,
          (xgm::ISoundChip*) &impl_->fme7, (xgm::ISoundChip*) &impl_->vrc7}) {
        chip->SetClock(c);
        chip->SetRate(sr);
        chip->Reset();
    }
    impl_->dmc.SetPal(pal_);
    pokeApu(0x17, 0xC0);
    pokeApu(0x15, 0x0F);
    pokeApu(0x08, 0x80);
    applyCartSetup();
    loadSample();
}

void Grit::applyTriangle() {
    if (triNote_ < 0) {
        pokeApu(0x08, 0x80);
        pokeApu(0x0B, 0x00);
        return;
    }
    double hz = 0.0;
    for (const auto& v : voices_)
        if (v.note == triNote_) hz = v.hz;
    if (hz <= 0.0) return;
    const int t = std::clamp((int) std::lround(clock() / (32.0 * hz)) - 1, 0, 2047);
    pokeApu(0x08, 0xFF);
    pokeApu(0x0A, t & 255);
    const int hi = t >> 8;
    if (hi != triHi_) {
        triHi_ = hi;
        pokeApu(0x0B, hi);
    }
}

void Grit::noteOn(int note, int vel, const Tuning& tuning) {
    if (note >= kNoiseSplit && params.get("NoiseKeys", 1.0) >= 0.5) {
        noiseNote_ = note;
        noiseEnv_.vel = 0.25f + 0.75f * (float) vel / 127.0f;
        noiseEnv_.phase = 0;
        noiseEnv_.wait = 0;
        noiseEnv_.sounding = true;
        if ((int) params.get("Attack", 0.0) == 0) noiseEnv_.level = 15;
        const bool buzz = params.get("Buzz", 0.0) >= 0.5;
        const int period = std::clamp(15 - (note - kNoiseSplit), 0, 15);
        pokeApu(0x0C, 0x30
                          | std::clamp((int) std::lround(noiseEnv_.level * noiseEnv_.vel),
                                       0, 15));
        pokeApu(0x0E, (buzz ? 0x80 : 0x00) | period);
        pokeApu(0x0F, 0xF8);
        return;
    }
    if (note < kDpcmSplit && !impl_->rom.bytes.empty()) {
        const int rate = std::clamp(note - (kDpcmSplit - 16), 0, 15);
        const bool loop = params.get("DpcmLoop", 0.0) >= 0.5;
        pokeApu(0x15, 0x0F);
        pokeApu(0x12, 0x00);
        pokeApu(0x13, std::clamp((int) (impl_->rom.bytes.size() / 16), 1, 255));
        pokeApu(0x10, (loop ? 0x40 : 0x00) | rate);
        pokeApu(0x15, 0x1F);
        return;
    }

    const int n = voiceCount();
    int v = -1;
    for (int i = 0; i < n; ++i)
        if (voices_[(size_t) i].note < 0) { v = i; break; }
    if (v < 0) {
        v = 0;
        for (int i = 1; i < n; ++i)
            if (voices_[(size_t) i].age < voices_[(size_t) v].age) v = i;
        silenceVoice(v);
    }
    auto& vc = voices_[(size_t) v];
    vc.note = note;
    vc.hz = tuning.hz(note);
    vc.vel = 0.25f + 0.75f * (float) vel / 127.0f;
    vc.age = ++age_;
    vc.phase = 0;
    vc.wait = 0;
    vc.sounding = true;
    if ((int) params.get("Attack", 0.0) == 0)
        vc.level = std::clamp((int) params.get("Sustain", 12.0), 0, 15);
    applyPitch(v);
    applyLevel(v);
    triggerVoice(v);

    if (params.get("Triangle", 1.0) >= 0.5) {
        int lowest = -1;
        for (int i = 0; i < n; ++i)
            if (voices_[(size_t) i].note >= 0
                && (lowest < 0 || voices_[(size_t) i].note < lowest))
                lowest = voices_[(size_t) i].note;
        if (lowest != triNote_) {
            triNote_ = lowest;
            applyTriangle();
        }
    }
}

void Grit::noteOff(int note) {
    if (note == noiseNote_) {
        noiseNote_ = -1;
        noiseEnv_.phase = 3;
        noiseEnv_.wait = 0;
        return;
    }
    const int n = voiceCount();
    for (int v = 0; v < n; ++v)
        if (voices_[(size_t) v].note == note) {
            voices_[(size_t) v].note = -1;
            voices_[(size_t) v].phase = 3;
            voices_[(size_t) v].wait = 0;
        }
    if (note == triNote_) {
        int lowest = -1;
        for (int i = 0; i < n; ++i)
            if (voices_[(size_t) i].note >= 0
                && (lowest < 0 || voices_[(size_t) i].note < lowest))
                lowest = voices_[(size_t) i].note;
        triNote_ = lowest;
        applyTriangle();
    }
}

void Grit::envelopeTick() {
    const int a = std::clamp((int) params.get("Attack", 0.0), 0, 15);
    const int d = std::clamp((int) params.get("Decay", 4.0), 0, 15);
    const int sus = std::clamp((int) params.get("Sustain", 12.0), 0, 15);
    const int r = std::clamp((int) params.get("Release", 4.0), 0, 15);

    auto step = [&](Voice& v) {
        if (!v.sounding) return false;
        if (v.wait > 0) { --v.wait; return false; }
        int was = v.level;
        if (v.phase == 0) {
            if (v.level >= 15 || a == 0) { v.phase = 1; v.level = std::max(v.level, sus); }
            else { ++v.level; v.wait = a - 1; }
            if (v.level >= 15) v.phase = 1;
        } else if (v.phase == 1) {
            if (v.level <= sus || d == 0) { v.level = sus; v.phase = 2; }
            else { --v.level; v.wait = d - 1; }
        } else if (v.phase == 3) {
            if (v.level <= 0 || r == 0) { v.level = 0; v.sounding = false; }
            else { --v.level; v.wait = r - 1; }
        }
        return v.level != was || !v.sounding;
    };

    const int n = voiceCount();
    for (int v = 0; v < n; ++v)
        if (step(voices_[(size_t) v])) {
            applyLevel(v);
            if (!voices_[(size_t) v].sounding) silenceVoice(v);
        }
    if (step(noiseEnv_)) {
        const int lvl = std::clamp((int) std::lround(noiseEnv_.level * noiseEnv_.vel), 0,
                                   15);
        pokeApu(0x0C, 0x30 | lvl);
    }
}

void Grit::renderChunk(float* l, float* r, int n, float level) {
    const GritCart c = cart();
    xgm::ISoundChip* exp = nullptr;
    float expGain = 0.0f;
    switch (c) {
        case GritCart::kVrc6: exp = &impl_->vrc6; expGain = 0.9f; break;
        case GritCart::kMmc5: exp = &impl_->mmc5; expGain = 1.0f; break;
        case GritCart::kFds: exp = &impl_->fds; expGain = 0.7f; break;
        case GritCart::kN163: exp = &impl_->n163; expGain = 0.8f; break;
        case GritCart::k5B: exp = &impl_->fme7; expGain = 0.7f; break;
        case GritCart::kVrc7: exp = &impl_->vrc7; expGain = 0.9f; break;
        default: break;
    }
    const double cps = clock() / (sampleRate_ > 0.0 ? sampleRate_ : 44100.0);
    const double envHz = pal_ ? 200.0 : 240.0;
    const double eps = envHz / (sampleRate_ > 0.0 ? sampleRate_ : 44100.0);
    const float k = level / 4800.0f;
    const float rPole =
        1.0f - 75.0f / (float) (sampleRate_ > 0.0 ? sampleRate_ : 44100.0);
    for (int i = 0; i < n; ++i) {
        envAcc_ += eps;
        while (envAcc_ >= 1.0) {
            envAcc_ -= 1.0;
            envelopeTick();
        }
        cycleAcc_ += cps;
        const auto clocks = (xgm::UINT32) cycleAcc_;
        cycleAcc_ -= (double) clocks;
        impl_->apu.Tick(clocks);
        impl_->dmc.TickFrameSequence(clocks);
        impl_->dmc.Tick(clocks);
        if (exp != nullptr) exp->Tick(clocks);
        xgm::INT32 b[2] = {0, 0};
        float mix = 0.0f;
        impl_->apu.Render(b);
        mix += (float) (b[0] + b[1]) * 0.5f;
        impl_->dmc.Render(b);
        mix += (float) (b[0] + b[1]) * 0.5f;
        if (exp != nullptr) {
            exp->Render(b);
            mix += (float) (b[0] + b[1]) * 0.5f * expGain;
        }
        const float x = mix * k;
        dcOut_ = x - dcIn_ + rPole * dcOut_;
        dcIn_ = x;
        l[i] = dcOut_;
        r[i] = dcOut_;
    }
}

void Grit::process(const float* const*, int, float* const* out, int numOut,
                   int numSamples, const Transport& transport) {
    if (numOut == 0) return;
    float* l = out[0];
    float* r = numOut > 1 ? out[1] : out[0];

    if (const bool wantPal = params.get("Region", 0.0) >= 0.5; wantPal != pal_) {
        pal_ = wantPal;
        applyRegion();
    }
    if ((int) params.get("Cartridge", 0.0) != cachedCart_) applyCartSetup();
    const int duty = std::clamp((int) params.get("Duty", 2.0), 0, 3);
    const int wave = std::clamp((int) params.get("Wave", 0.0), 0, 4);
    const int patch = std::clamp((int) params.get("Patch", 1.0), 1, 15);
    if (duty != cachedDuty_ || wave != cachedWave_ || patch != cachedPatch_) {
        cachedDuty_ = duty;
        cachedPatch_ = patch;
        if (wave != cachedWave_) {
            cachedWave_ = wave;
            applyWavetables();
        }
        const int n = voiceCount();
        for (int v = 0; v < n; ++v)
            if (voices_[(size_t) v].sounding) {
                applyPitch(v);
                applyLevel(v);
            }
    }
    if (const auto* sp = params.byName("Sample");
        sp != nullptr && sp->text != cachedSample_)
        loadSample();

    MidiEvent ev[2 * MidiNode::kMaxMidiEventsPerBlock];
    int nEv = 0;
    {
        std::unique_lock<std::mutex> g(liveLock_, std::try_to_lock);
        if (g.owns_lock()) {
            for (int i = 0; i < liveCount_; ++i) {
                ev[nEv] = liveQ_[(size_t) i];
                ev[nEv++].sampleOffset = 0;
            }
            liveCount_ = 0;
        }
    }
    for (int i = 0; i < stagedCount_; ++i) ev[nEv++] = staged_[(size_t) i];
    stagedCount_ = 0;
    std::stable_sort(ev, ev + nEv, [](const MidiEvent& a, const MidiEvent& b) {
        return a.sampleOffset < b.sampleOffset;
    });

    const float level = (float) params.get("Level", 0.8);
    int at = 0;
    for (int i = 0; i <= nEv; ++i) {
        const int upTo = i < nEv ? std::clamp(ev[i].sampleOffset, at, numSamples)
                                 : numSamples;
        if (upTo > at) {
            renderChunk(l + at, r + at, upTo - at, level);
            at = upTo;
        }
        if (i < nEv) {
            const auto& e = ev[i];
            const int st = e.data[0] & 0xF0;
            if (st == 0x90 && e.data[2] > 0)
                noteOn(e.data[1], e.data[2], transport.tuning());
            else if (st == 0x80 || (st == 0x90 && e.data[2] == 0)) noteOff(e.data[1]);
        }
    }
}

}
