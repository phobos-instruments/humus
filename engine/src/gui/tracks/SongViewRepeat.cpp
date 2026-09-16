// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/SongView.h"

#include <algorithm>

namespace hum {

static constexpr int kMaxRepeats = 128;

int SongView::clipIndexOfId(const std::string& node, int id) const {
    if (id <= 0) return -1;
    for (const auto& ci : host().clips().list(node))
        if (ci.id == id) return ci.index;
    return -1;
}

void SongView::applyRepeatFill(int endTick) {
    if (dragRow_ < 0 || dragRow_ >= (int) rows_.size() || repeatLen_ <= 0) return;
    const auto& node = rows_[(size_t) dragRow_];
    const int src = clipIndexOfId(node, repeatSrcId_);
    if (src < 0) return;

    const int want = std::clamp((endTick - repeatStart_) / repeatLen_ - 1, 0, kMaxRepeats);
    if (want == (int) repeatIds_.size()) return;

    while ((int) repeatIds_.size() > want) {
        if (const int i = clipIndexOfId(node, repeatIds_.back()); i >= 0)
            host().clips().remove(node, i);
        repeatIds_.pop_back();
    }
    while ((int) repeatIds_.size() < want) {
        const int at = repeatStart_ + (int) (repeatIds_.size() + 1) * repeatLen_;
        const int n = host().clips().duplicate(node, clipIndexOfId(node, repeatSrcId_), at);
        if (n < 0) break;
        repeatIds_.push_back(host().clips().list(node)[(size_t) n].id);
    }
    if (!repeatIds_.empty())
        selectClip(dragRow_, clipIndexOfId(node, repeatIds_.back()));
}

}
