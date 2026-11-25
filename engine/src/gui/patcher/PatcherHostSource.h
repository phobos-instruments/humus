// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "core/packs/ClassString.h"
#include "core/plugins/PluginNode.h"
#include "gui/host/PatcherHost.h"
#include "gui/patcher/PatchViewSource.h"

namespace hum {

class PatcherHostSource final : public PatchViewSource {
public:
    explicit PatcherHostSource(PatcherHost& host) : host_(host) {}

    const PatchDocumentModel& model() const override { return host_.model(); }
    juce::Point<int> position(const std::string& n) const override { return host_.position(n); }
    int inletsOf(const std::string& n) override { return host_.inletsOf(n); }
    int outletsOf(const std::string& n) override { return host_.outletsOf(n); }
    int midiInletsOf(const std::string& n) override { return host_.midiInletsOf(n); }
    int midiOutletsOf(const std::string& n) override { return host_.midiOutletsOf(n); }
    int videoInletsOf(const std::string& n) override { return host_.videoInletsOf(n); }
    int videoOutletsOf(const std::string& n) override { return host_.videoOutletsOf(n); }
    int controlInletsOf(const std::string& n) override { return host_.controlInletsOf(n); }
    int controlOutletsOf(const std::string& n) override { return host_.controlOutletsOf(n); }
    std::string controlInletParam(const std::string& n, int i) override { return host_.controlInletParam(n, i); }
    std::string controlOutletValue(const std::string& n, int o) override { return host_.controlOutletValue(n, o); }
    std::vector<ControlCord> controlCordsInScope(const std::string& scope) override {
        return host_.controlCordsInScope(scope);
    }
    Organism* liveOrganism(const std::string& n) override { return host_.liveOrganism(n); }
    bool bypassed(const std::string& n) override { return host_.bypassed(n); }
    bool silent(const std::string& n) override {
        if (const auto* cm = host_.model().byName(n); cm && isPluginKind(cm->kind)) {
            auto* pn = host_.pluginNodeFor(n);
            return pn == nullptr || !pn->responding();
        }
        return !host_.missingClassNote(n).empty();
    }
    void nodeActivity(const std::string& n, float& peak, float& midi) override { host_.nodeActivity(n, peak, midi); }
    juce::uint64 changeStamp() const override { return host_.changeStamp(); }

private:
    PatcherHost& host_;
};

}
