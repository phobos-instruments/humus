// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <utility>

#include "gui/editor/FaceplateView.h"
#include "gui/editor/juce/JuceBrickViews.h"
#include "gui/host/EditorHost.h"
#include "hum/LayoutSpec.h"

namespace hum {

class LayoutEditor : public FaceplateView {
public:
    LayoutEditor(EditorHost& host, std::string organism, LayoutSpec spec)
        : FaceplateView(host, std::move(organism), std::move(spec),
                        [&host](juce::Component& parent) { return std::make_unique<JuceBrickViews>(parent, host); }),
          host_(host) {}

private:
    void openMenu(const std::string& param, Point at) override;
    int coverTicks() const override;

    EditorHost& host_;
};

}
