#pragma once
#include <string>

namespace hum {

enum class TuningProbeVerdict { SpeaksMts, NeedsBend, Inconclusive };

TuningProbeVerdict probeTuning(const std::string& classRaw);

const char* tuningProbeVerdictName(TuningProbeVerdict v);
bool parseTuningProbeVerdict(const std::string& s, TuningProbeVerdict& out);

}
