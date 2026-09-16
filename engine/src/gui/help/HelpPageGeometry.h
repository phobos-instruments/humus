// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

struct HelpImageRow {
    juce::Rectangle<float> preview, node;
    int bottom = 0;
};

inline HelpImageRow helpImageRow(int previewW, int previewH, int nodeW, int nodeH,
                                 int width, int pad) {
    HelpImageRow row;
    const float p = (float) pad;
    if (previewW > 0 && previewH > 0) {
        const float s = juce::jmin(1.0f, ((float) width - 2.0f * p) / (float) previewW,
                                   220.0f / (float) previewH);
        row.preview = {p, p, (float) previewW * s, (float) previewH * s};
    }
    if (nodeW > 0 && nodeH > 0) {
        const float s = juce::jmin(1.0f, 170.0f / (float) nodeW, 110.0f / (float) nodeH);
        const float w = (float) nodeW * s, h = (float) nodeH * s;
        const float x = row.preview.isEmpty() ? p : row.preview.getRight() + p;
        if (x + w <= (float) width - p) {
            const float bottom = juce::jmax(row.preview.getBottom(), p + h);
            if (!row.preview.isEmpty()) row.preview.setY(bottom - row.preview.getHeight());
            row.node = {x, bottom - h, w, h};
        } else {
            row.node = {p, row.preview.getBottom() + p, w, h};
        }
    }
    row.bottom = (int) std::ceil(juce::jmax(row.preview.getBottom(), row.node.getBottom()));
    return row;
}

}
