// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

namespace hum {
namespace wizard {

enum Page { kAudio = 0, kMidi, kAppearance, kMenuStyle, kTelemetry, kFinish, kNumPages };

inline Page next(Page p) { return p >= kFinish ? kFinish : (Page) (p + 1); }
inline Page back(Page p) { return p <= kAudio ? kAudio : (Page) (p - 1); }
inline bool isFirst(Page p) { return p == kAudio; }
inline bool isLast(Page p) { return p == kFinish; }

inline constexpr int kNeverChosen = -1;

inline bool updatesStartTicked(int stored) { return stored != 0; }

inline int updatesToStore(bool sawPrivacyPage, bool ticked, int stored) {
    if (!sawPrivacyPage) return stored;
    return ticked ? 1 : 0;
}

}
}
