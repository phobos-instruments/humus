// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

#include <algorithm>
#include <cmath>
#include "gui/tracks/ClipChrome.h"



namespace hum {

SongView::ClipHit SongView::rowClipHit(int row, juce::Point<int> p) const {
    bool l = false, r = false;
    const int c = clipAt(row, p, l, r);
    if (c < 0) return ClipHit::None;
    const auto clips = host().clips().list(rows_[(size_t) row]);
    if (c >= (int) clips.size()) return ClipHit::None;
    const auto& ci = clips[(size_t) c];
    const auto b = clipBounds(row, ci);
    using timelinechrome::FadeGrip;
    const auto fg = ci.hasMedia()
        ? timelinechrome::fadeGripAt(b, p, kFadeGrip, ci.fadeInTicks, ci.fadeOutTicks,
                                     ci.lengthTicks)
        : FadeGrip::None;
    if (fg == FadeGrip::Left) return ClipHit::FadeL;
    if (fg == FadeGrip::Right) return ClipHit::FadeR;
    const auto cg = ci.hasMedia()
        ? timelinechrome::fadeCurveGripAt(b, p, kFadeGrip, ci.fadeInTicks,
                                          ci.fadeOutTicks, ci.lengthTicks,
                                          ci.fadeInCurve, ci.fadeOutCurve)
        : FadeGrip::None;
    if (cg == FadeGrip::Left) return ClipHit::CurveL;
    if (cg == FadeGrip::Right) return ClipHit::CurveR;
    if (l) return ClipHit::EdgeL;
    if (r) return ClipHit::EdgeR;
    return ClipHit::Body;
}

void SongView::setClipboard(PatternChannel data, int spanTicks) {
    clipboard_ = {{0, 0, std::move(data)}};
    clipboardSpan_ = spanTicks;
}

bool SongView::pasteClipboardInto(const std::string& node, int atTick) {
    const auto it = std::find(rows_.begin(), rows_.end(), node);
    if (it == rows_.end()) return false;
    return pasteClips(atTick, (int) (it - rows_.begin()));
}

}
