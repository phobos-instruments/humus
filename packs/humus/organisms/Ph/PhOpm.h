#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include "opm.h"
}

#include "Ph/PhMods.h"
#include "Ph/PhVoice.h"

namespace hum {

class PhOpm {
public:
    static constexpr int kChannels = 8;
    static constexpr double kMasterClock = 3579545.0;
    static constexpr double kRate = kMasterClock / 64.0;

    struct Op {
        uint8_t mult = 1, dt1 = 3, dt2 = 0;
        uint8_t tl = 40;
        uint8_t ks = 0, ar = 31, d1r = 8, d2r = 0, rr = 10, d1l = 2, ame = 0;
    };
    struct Patch {
        uint8_t alg = 4, fb = 4;
        uint8_t pms = 0, ams = 0;
        uint8_t lfrq = 0, amd = 0, pmd = 0, wf = 0;
        std::array<Op, 4> ops{};
    };
    struct Instrument {
        std::string name;
        Patch patch;
    };

    PhOpm();
    void reset();
    void setVariant(bool opp);
    bool variant() const { return opp_; }

    struct KeyReg { int kc, kf; };
    static KeyReg keyRegisters(double hz);

    static constexpr int kMaxBank = 512;

    void useFactoryBank();
    bool loadOpm(const std::string& path);
    bool parseOpm(const std::string& text, const std::string& fallbackName);
    int patchCount() const { return (int) bank_.size(); }
    int currentSlot() const { return slot_; }
    std::string patchName(int slot) const;
    void selectPatch(int slot);
    void setMods(const PhMods& m);

    PhVoice selectedVoice() const { return voiceAt(slot_); }
    PhVoice voiceAt(int slot) const;
    void setVoice(const PhVoice& v);

    void noteOn(int midinote, int velocity, double hz);
    void noteOff(int midinote);
    void allOff();

    void render(float* outL, float* outR, int n);

private:
    void push(uint8_t reg, uint8_t data);
    void writeChannelPatch(int ch, int velocity);
    void writeKey(int ch, double hz);
    void writeLfo();
    void keyOn(int ch, bool on);
    const Patch& patch() const;

    opm_t chip_{};
    bool opp_ = false;
    std::vector<Instrument> bank_;
    int slot_ = 0;
    PhMods mods_;
    Patch edited_;
    bool hasEdit_ = false;
    std::array<int, kChannels> chanNote_{};
    int next_ = 0;

    struct Write { uint8_t bus, value; };
    std::array<Write, 2048> queue_{};
    int qHead_ = 0, qTail_ = 0;
};

}
