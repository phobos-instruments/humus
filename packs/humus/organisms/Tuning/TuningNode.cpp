#include "Tuning/TuningNode.h"

#include <algorithm>
#include <cmath>

#include <juce_core/juce_core.h>

namespace hum {

namespace {
enum Preset {
    k12TET = 0, k19EDO, k24EDO, k31EDO, kBohlenPierce, kCustom,
    kJI5Limit, kJI7Limit, kJI17Limit, kHarmonics, kPythagorean, kSclFile,
};

constexpr double kTritaveCents = 1901.9550008653874;

struct R { double p, q; };

constexpr R kJI5[] = {{16, 15}, {9, 8}, {6, 5}, {5, 4}, {4, 3}, {45, 32},
                      {3, 2}, {8, 5}, {5, 3}, {9, 5}, {15, 8}, {2, 1}};

constexpr R kJI7[] = {{16, 15}, {9, 8}, {7, 6}, {5, 4}, {4, 3}, {7, 5},
                      {3, 2}, {8, 5}, {5, 3}, {7, 4}, {15, 8}, {2, 1}};

constexpr R kJI17[] = {{17, 16}, {9, 8}, {6, 5}, {5, 4}, {4, 3}, {17, 12},
                       {3, 2}, {13, 8}, {5, 3}, {7, 4}, {15, 8}, {2, 1}};

constexpr R kHarm[] = {{9, 8}, {5, 4}, {11, 8}, {3, 2}, {13, 8}, {7, 4},
                       {15, 8}, {2, 1}};

constexpr R kPyth[] = {{256, 243}, {9, 8}, {32, 27}, {81, 64}, {4, 3}, {729, 512},
                       {3, 2}, {128, 81}, {27, 16}, {16, 9}, {243, 128}, {2, 1}};

template <int N>
Tuning fromRatios(const R (&r)[N], double root, double rootHz) {
    double cents[N];
    for (int i = 0; i < N; ++i) cents[i] = 1200.0 * std::log2(r[i].p / r[i].q);
    return Tuning::fromCents(cents, N, root, rootHz);
}
}

void TuningNode::loadFromFile(const std::string& uri) {
    juce::String path(juce::CharPointer_UTF8(uri.c_str()));
    if (path.startsWith("file://")) path = path.substring(7);
    const juce::File f(path);

    const Tuning parsed = Tuning::fromSclText(f.existsAsFile()
                                                  ? f.loadFileAsString().toStdString()
                                                  : std::string());
    {
        std::lock_guard<std::mutex> g(sclLock_);
        sclStaged_ = parsed;
    }
    sclFresh_.store(true, std::memory_order_release);
}

void TuningNode::parseSclFromParam() {
    const std::string uri = params.getText("File");
    if (uri.empty()) return;
    juce::String path(juce::CharPointer_UTF8(uri.c_str()));
    if (path.startsWith("file://")) path = path.substring(7);
    const juce::File f(path);
    if (f.existsAsFile())
        scl_ = Tuning::fromSclText(f.loadFileAsString().toStdString());
}

void TuningNode::rebuild() {
    lastPreset_ = params.get("Preset", 0.0);
    lastDivisions_ = params.get("Divisions", 12.0);
    lastRoot_ = params.get("Root", 69.0);
    lastRootHz_ = params.get("RootHz", 440.0);
    lastMap_ = params.get("Map", 0.0);
    lastPlugins_ = params.get("Plugins", 0.0);

    const int preset = std::clamp((int) std::lround(lastPreset_), 0, (int) kSclFile);
    const double root = std::clamp(lastRoot_, 0.0, 127.0);
    const double rootHz = std::clamp(lastRootHz_, 20.0, 4000.0);

    switch (preset) {
        case k19EDO: tuning_ = Tuning::equalDivisions(19, 1200.0, root, rootHz); break;
        case k24EDO: tuning_ = Tuning::equalDivisions(24, 1200.0, root, rootHz); break;
        case k31EDO: tuning_ = Tuning::equalDivisions(31, 1200.0, root, rootHz); break;
        case kBohlenPierce:
            tuning_ = Tuning::equalDivisions(13, kTritaveCents, root, rootHz);
            break;
        case kCustom: {
            const int n = std::clamp((int) std::lround(lastDivisions_), 5, 64);
            tuning_ = Tuning::equalDivisions(n, 1200.0, root, rootHz);
            break;
        }
        case kJI5Limit:    tuning_ = fromRatios(kJI5, root, rootHz); break;
        case kJI7Limit:    tuning_ = fromRatios(kJI7, root, rootHz); break;
        case kJI17Limit:   tuning_ = fromRatios(kJI17, root, rootHz); break;
        case kHarmonics:   tuning_ = fromRatios(kHarm, root, rootHz); break;
        case kPythagorean: tuning_ = fromRatios(kPyth, root, rootHz); break;
        case kSclFile: {
            double cents[Tuning::kMaxDegrees];
            const int n = scl_.degreesPerPeriod();
            for (int k = 1; k <= n; ++k) cents[k - 1] = scl_.centsAtDegree(k);
            tuning_ = Tuning::fromCents(cents, n, root, rootHz);
            break;
        }
        case k12TET:
        default:
            tuning_ = Tuning::equalDivisions(12, 1200.0, root, rootHz);
            break;
    }
    tuning_.setKeyboardMap(lastMap_ >= 0.5);
    tuning_.setPluginDelivery((int) std::lround(lastPlugins_));
}

}
