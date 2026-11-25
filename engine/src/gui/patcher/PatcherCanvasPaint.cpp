// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/style/Colours.h"
#include "gui/common/PerfLog.h"
#include "gui/patcher/PatcherCanvas.h"

#include <cmath>

#include "core/packs/Categories.h"
#include "core/packs/ClassString.h"
#include "core/plugins/PluginNode.h"
#include "core/graph/PodModel.h"
#include "gui/style/DropBadge.h"
#include "gui/style/LookAndFeel.h"
#include "gui/common/Localisation.h"

namespace hum {

void PatcherCanvas::CrumbBar::paint(juce::Graphics& g) {
    crumbs_.clear();
    const auto& scope = canvas_.scope_;
    if (scope.empty()) return;
    g.setColour(Palette::panelLight.withAlpha(alpha::nearOpaque));
    g.fillRect(getLocalBounds());
    g.setColour(Palette::border);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float) getWidth());
    g.setFont(juce::FontOptions(12.5f));
    int x = 8;
    auto segment = [&](const juce::String& label, const std::string& target, bool last) {
        juce::GlyphArrangement ga;
        ga.addLineOfText(g.getCurrentFont(), label, 0.0f, 0.0f);
        const int w = (int) std::ceil(ga.getBoundingBox(0, -1, true).getWidth()) + 12;
        const juce::Rectangle<int> r(x, 0, w, getHeight() - 1);
        g.setColour(last ? Palette::accent : Palette::textDim);
        g.drawText(label, r, juce::Justification::centred);
        if (!last) crumbs_.emplace_back(r, target);
        x = r.getRight();
        if (!last) {
            g.setColour(Palette::textDim);
            g.drawText(juce::String::fromUTF8("\xe2\x96\xb8"), x, 0, 14, getHeight() - 1,
                       juce::Justification::centred);
            x += 14;
        }
    };
    std::vector<std::string> parts;
    std::string acc;
    for (size_t i = 0, j; i <= scope.size(); i = j + 1) {
        j = scope.find('/', i);
        if (j == std::string::npos) j = scope.size();
        parts.push_back(scope.substr(i, j - i));
        if (j == scope.size()) break;
    }
    segment(tr("patcher-canvas-paint.home-patch", "{home} patch", {{"home", juce::String::fromUTF8("\xe2\x8c\x82")}}), "", false);
    for (size_t i = 0; i < parts.size(); ++i) {
        acc = acc.empty() ? parts[i] : acc + "/" + parts[i];
        segment(parts[i], acc, i + 1 == parts.size());
    }
}

}
