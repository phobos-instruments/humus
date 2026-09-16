// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum {

enum class TuningProbeVerdict { SpeaksMts, NeedsBend, Inconclusive };

TuningProbeVerdict probeTuning(const std::string& classRaw);

const char* tuningProbeVerdictName(TuningProbeVerdict v);
bool parseTuningProbeVerdict(const std::string& s, TuningProbeVerdict& out);

}
