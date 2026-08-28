#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "core/AppPaths.h"
#include "core/AudioGraph.h"
#include "hum/Number.h"
#include "hum/Registry.h"
#include "io/PatchDocument.h"
#include "io/PatchLoader.h"
#include "io/WavWriter.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/PackLoader.h"
#include "core/PackRegistry.h"
#include "hum_addon_packs.h"
#include "core/PluginHost.h"
#include "core/PluginListStore.h"
#include "core/TuningProbe.h"
#include "core/TuningProbeStore.h"
#include "hum/Capabilities.h"

#include <HumBuildId.h>

#if HUM_LIVE_AUDIO
#include <juce_audio_devices/juce_audio_devices.h>
#endif

using namespace hum;

namespace {

struct Override { std::string organism, param; double value; };

struct Opts {
    double seconds = 10.0;
    double sampleRate = 44100.0;
    int block = 512;
    unsigned seed = 0;
    bool seedSet = false;
    std::vector<Override> sets;
};

Opts parseOpts(int argc, char** argv, int from) {
    Opts o;
    for (int i = from; i < argc; ++i) {
        std::string a = argv[i];
        auto nextStr = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        auto nextNum = [&] { return (i + 1 < argc) ? hum::scanDouble(argv[++i]) : 0.0; };
        if (a == "--seconds") o.seconds = nextNum();
        else if (a == "--sr") o.sampleRate = nextNum();
        else if (a == "--block") o.block = (int) nextNum();
        else if (a == "--seed") { o.seed = (unsigned) std::atoi(nextStr().c_str()); o.seedSet = true; }
        else if (a == "--set") {
            std::string spec = nextStr();
            auto dot = spec.find('.'), eq = spec.find('=');
            if (dot != std::string::npos && eq != std::string::npos && dot < eq)
                o.sets.push_back({spec.substr(0, dot), spec.substr(dot + 1, eq - dot - 1),
                                  hum::scanDouble(spec.substr(eq + 1).c_str())});
        }
    }
    return o;
}

void applyOverrides(AudioGraph& g, const std::vector<Override>& sets) {
    for (const auto& s : sets) {
        Organism* c = g.find(s.organism);
        if (!c) { std::cerr << "warn: no organism '" << s.organism << "'\n"; continue; }
        Parameter* p = c->params.byName(s.param);
        if (!p) { std::cerr << "warn: " << s.organism << " has no param '" << s.param << "'\n"; continue; }
        p->value = s.value;
        p->rangeMin = p->rangeMax = s.value;
    }
}

void restoreKnownPlugins() {
    hum::PluginHost::instance().restoreKnownListFromXml(hum::pluginListStore::load());
}

MasterTap* findSoundOut(AudioGraph& g) {
    for (int i = 0; i < g.nodeCount(); ++i)
        if (auto* so = dynamic_cast<MasterTap*>(g.organism(i))) return so;
    return nullptr;
}

int cmdInfo(const std::string& path) {
    PatchDocumentModel doc;
    std::string err;
    if (!parsePatchFile(path, doc, err)) { std::cerr << "error: " << err << "\n"; return 1; }
    int native = 0, au = 0;
    for (auto& c : doc.organisms) (c.kind == "au" ? au : native)++;
    std::cout << "version " << doc.version << "  tempo " << doc.clock.tempo << "\n"
              << "organisms: " << doc.organisms.size()
              << " (" << native << " native, " << au << " AU)\n"
              << "connections : " << doc.connections.size() << "\n";
    registerBuiltinOrganisms();
    PackLoader::instance().loadInstalledPacks();
    restoreKnownPlugins();
    for (auto& c : doc.organisms) {
        bool known = Registry::instance().isKnown(c.displayClass);
        std::cout << "  " << (known ? "[x] " : "[ ] ") << c.name
                  << "  (" << c.displayClass << ", " << c.kind << ")\n";
    }
    return 0;
}

int cmdRender(const std::string& path, const std::string& out, const Opts& o) {
    PatchDocumentModel doc;
    std::string err;
    if (!parsePatchFile(path, doc, err)) { std::cerr << "error: " << err << "\n"; return 1; }

    if (o.seedSet) setRenderSeed(o.seed);
    PackLoader::instance().loadInstalledPacks();
    restoreKnownPlugins();

    AudioGraph graph;
    if (!buildGraph(doc, graph, err)) { std::cerr << "error: " << err << "\n"; return 1; }
    applyOverrides(graph, o.sets);
    graph.prepare(o.sampleRate, o.block, doc.clock.tempo);

    MasterTap* so = findSoundOut(graph);
    if (!so) { std::cerr << "error: patch has no SoundOut\n"; return 1; }

    const int64_t total = (int64_t) (o.seconds * o.sampleRate);
    const int channels = so->channels();
    std::vector<std::vector<float>> outBuf((size_t) channels);

    for (const auto& cls : graph.pluginsNeedingVerdict()) {
        hum::TuningProbeVerdict v;
        if (!hum::tuningProbeStore::lookup(cls, 0, v)) {
            v = hum::probeTuning(cls);
            hum::tuningProbeStore::store(cls, 0, v);
        }
        graph.setBendVerdict(cls, v != hum::TuningProbeVerdict::SpeaksMts);
    }

    int64_t done = 0;
    while (done < total) {
        int n = (int) std::min<int64_t>(o.block, total - done);
        graph.pumpPluginTunings();
        graph.processBlock(n);
        for (int c = 0; c < channels; ++c) {
            const float* src = so->channelData(c);
            outBuf[(size_t) c].insert(outBuf[(size_t) c].end(), src, src + so->lastBlockLength());
        }
        done += n;
    }

    if (!writeWav(out, outBuf, o.sampleRate)) { std::cerr << "error: could not write " << out << "\n"; return 1; }
    std::cout << "rendered " << o.seconds << "s (" << channels << "ch @ "
              << o.sampleRate << ") -> " << out << "\n";
    if (graph.hasFeedback()) std::cout << "note: patch contains feedback loop(s)\n";
    return 0;
}

#if HUM_LIVE_AUDIO
int cmdPlay(const std::string& path, const Opts& o) {
    PatchDocumentModel doc; std::string err;
    if (!parsePatchFile(path, doc, err)) { std::cerr << "error: " << err << "\n"; return 1; }
    auto graph = std::make_shared<AudioGraph>();
    if (!buildGraph(doc, *graph, err)) { std::cerr << "error: " << err << "\n"; return 1; }
    applyOverrides(*graph, o.sets);

    struct Cb : juce::AudioIODeviceCallback {
        std::shared_ptr<AudioGraph> g; MasterTap* so = nullptr; int block = 512; double sr = 44100;
        void audioDeviceAboutToStart(juce::AudioIODevice* d) override {
            sr = d->getCurrentSampleRate(); block = d->getCurrentBufferSizeSamples();
            g->prepare(sr, block, g->transport().tempo());
            so = nullptr;
            for (int i = 0; i < g->nodeCount(); ++i) if (auto* s = dynamic_cast<MasterTap*>(g->organism(i))) so = s;
        }
        void audioDeviceIOCallbackWithContext(const float* const*, int, float* const* out,
                                              int numOut, int numSamples,
                                              const juce::AudioIODeviceCallbackContext&) override {
            g->processBlock(numSamples);
            for (int c = 0; c < numOut; ++c) {
                if (so && c < so->channels()) std::memcpy(out[c], so->channelData(c), sizeof(float) * (size_t) numSamples);
                else std::memset(out[c], 0, sizeof(float) * (size_t) numSamples);
            }
        }
        void audioDeviceStopped() override {}
    } cb;
    cb.g = graph; cb.block = o.block;
    graph->transport().prepare(o.sampleRate, doc.clock.tempo);

    juce::AudioDeviceManager dm;
    juce::String e = dm.initialiseWithDefaultDevices(0, 2);
    if (e.isNotEmpty()) { std::cerr << "audio init failed: " << e << "\n"; return 1; }
    dm.addAudioCallback(&cb);
    std::cout << "playing " << path << " - press Enter to stop...\n";
    std::cin.get();
    dm.removeAudioCallback(&cb);
    return 0;
}
#endif

void usage() {
    std::cout << "usage:\n"
              << "  hum info   <patch.hum|.amh>\n"
              << "  hum render <patch.hum|.amh> <out.wav> [--seconds N --sr 44100 --block 512]\n"
              << "             [--seed N] [--set Organism.Param=value ...]\n"
#if HUM_LIVE_AUDIO
              << "  hum play   <patch.hum|.amh>\n"
#endif
              << "  hum --version\n"
        ;
}

}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI juceInit;
#ifdef HUM_STATIC_ADDON_PACKS
    registerBuiltinOrganisms();
    registerStaticAddonPacks();
#endif
    if (argc >= 2 && (std::string(argv[1]) == "--version"
                      || std::string(argv[1]) == "version")) {
        std::cout << "hum " << HUM_VERSION << " (" << HUM_BUILD_ID << ") "
                  << PackLoader::platformTag() << "\n";
        return 0;
    }
    if (argc < 3) { usage(); return 2; }
    std::string cmd = argv[1];
    std::string file = argv[2];

    if (cmd == "info") return cmdInfo(file);
    if (cmd == "render") {
        if (argc < 4) { usage(); return 2; }
        return cmdRender(file, argv[3], parseOpts(argc, argv, 4));
    }
#if HUM_LIVE_AUDIO
    if (cmd == "play") return cmdPlay(file, parseOpts(argc, argv, 3));
#endif
    usage();
    return 2;
}
