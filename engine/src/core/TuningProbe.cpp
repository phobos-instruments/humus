#include "core/TuningProbe.h"

#include <cmath>
#include <vector>

#include "core/AudioGraph.h"
#include "core/GraphIo.h"
#include "core/PluginHost.h"
#include "hum/Registry.h"
#include "hum/dsp/DspMath.h"

namespace hum {

namespace {
constexpr double kSR = kDefaultSampleRate;
constexpr int kBlock = 4096;

struct DelayedNote : Organism, MidiNode {
    int note = 69, countdown = 4;
    bool sent = false;
    int numAudioInputs() const override { return 0; }
    int numAudioOutputs() const override { return 0; }
    int numMidiInputs() const override { return 0; }
    int numMidiOutputs() const override { return 1; }
    void prepare(double sr, int) override { sampleRate_ = sr; }
    void process(const float* const*, int, float* const*, int, int, const Transport&) override {}
    void deliverMidi(int, const MidiEvent*, int) override {}
    int collectMidi(int, MidiEvent* out, int cap) override {
        if (sent || cap < 1) return 0;
        if (countdown-- > 0) return 0;
        sent = true;
        out[0].data[0] = 0x90; out[0].data[1] = (unsigned char) note; out[0].data[2] = 100;
        out[0].size = 3; out[0].sampleOffset = 0;
        return 1;
    }
};

void setP(Organism& c, const char* name, double v) {
    if (auto* p = c.params.byName(name)) { p->value = v; p->rangeMin = p->rangeMax = v; return; }
    Parameter prm;
    prm.name = name;
    prm.value = v;
    c.params.add(prm);
}

double toneAt(const float* b, int n, double f) {
    double re = 0.0, im = 0.0, norm = 0.0;
    for (int i = 0; i < n; ++i) {
        const double w = 0.5 * (1.0 - std::cos(2.0 * kPi * (double) i / (double) (n - 1)));
        const double t = 2.0 * kPi * f * (double) i / kSR;
        re += (double) b[i] * w * std::cos(t);
        im += (double) b[i] * w * std::sin(t);
        norm += w;
    }
    return std::sqrt(re * re + im * im) / norm;
}

bool render(const std::string& classRaw, bool retuned,
            double& e440, double& e320, double& e311) {
    AudioGraph g;
    if (retuned) {
        auto t = Registry::instance().create("Tuning");
        if (!t) return false;
        setP(*t, "Preset", 0.0);
        setP(*t, "RootHz", 320.0);
        setP(*t, "Plugins", 0.0);
        t->setName("Tun");
        g.addNode(std::move(t));
    }
    auto note = std::make_unique<DelayedNote>();
    note->setName("Note");
    const int nn = g.addNode(std::move(note));
    auto plugin = PluginHost::createOrganism(classRaw);
    if (!plugin) return false;
    plugin->setName("P");
    const int pn = g.addNode(std::move(plugin));
    auto out = Registry::instance().create("SoundOut");
    out->setName("Out");
    const int so = g.addNode(std::move(out));
    g.connectMidi(nn, 0, pn, 0);
    g.connect(pn, 0, so, 0);
    g.connect(pn, 1, so, 1);
    g.prepare(kSR, kBlock, 120.0);

    const auto taps = findMasterTaps(g);
    std::vector<std::vector<float>> buf(2, std::vector<float>(kBlock, 0.0f));
    float* outs[2] = {buf[0].data(), buf[1].data()};
    std::vector<float> tail;
    for (int b = 0; b < 32; ++b) {
        for (auto& ch : buf) std::fill(ch.begin(), ch.end(), 0.0f);
        g.pumpPluginTunings();
        renderGraphBlock(g, nullptr, 0, outs, 2, kBlock, taps, {});
        if (b >= 24) tail.insert(tail.end(), buf[0].begin(), buf[0].end());
    }
    e440 = toneAt(tail.data(), (int) tail.size(), kA4Hz);
    e320 = toneAt(tail.data(), (int) tail.size(), 320.0);
    e311 = toneAt(tail.data(), (int) tail.size(), 311.127);
    return true;
}
}

TuningProbeVerdict probeTuning(const std::string& classRaw) {
    double c440 = 0, c320 = 0, c311 = 0;
    if (!render(classRaw, false, c440, c320, c311)) return TuningProbeVerdict::Inconclusive;
    if (!(c440 > 10.0 * c320) || c440 <= 1.0e-5) return TuningProbeVerdict::Inconclusive;

    double t440 = 0, t320 = 0, t311 = 0;
    if (!render(classRaw, true, t440, t320, t311)) return TuningProbeVerdict::Inconclusive;
    if (t320 > 10.0 * t440 && t320 > 3.0 * t311) return TuningProbeVerdict::SpeaksMts;
    return TuningProbeVerdict::NeedsBend;
}

const char* tuningProbeVerdictName(TuningProbeVerdict v) {
    switch (v) {
        case TuningProbeVerdict::SpeaksMts: return "mts";
        case TuningProbeVerdict::NeedsBend: return "bend";
        default: return "inconclusive";
    }
}

bool parseTuningProbeVerdict(const std::string& s, TuningProbeVerdict& out) {
    if (s == "mts") { out = TuningProbeVerdict::SpeaksMts; return true; }
    if (s == "bend") { out = TuningProbeVerdict::NeedsBend; return true; }
    if (s == "inconclusive") { out = TuningProbeVerdict::Inconclusive; return true; }
    return false;
}

}
