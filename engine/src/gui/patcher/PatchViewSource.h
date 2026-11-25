// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include <juce_graphics/juce_graphics.h>

#include "gui/host/ControlCord.h"
#include "core/packs/Categories.h"
#include "io/PatchDocument.h"

namespace hum {

inline bool hiddenInPatch(const OrganismModel& c) { return c.internal || isHiddenOrganism(c.displayClass); }

class Organism;

class PatchViewSource {
public:
    virtual ~PatchViewSource() = default;

    virtual const PatchDocumentModel& model() const = 0;
    virtual juce::Point<int> position(const std::string& name) const = 0;
    virtual int inletsOf(const std::string& name) = 0;
    virtual int outletsOf(const std::string& name) = 0;
    virtual int midiInletsOf(const std::string& name) = 0;
    virtual int midiOutletsOf(const std::string& name) = 0;
    virtual int videoInletsOf(const std::string& name) = 0;
    virtual int videoOutletsOf(const std::string& name) = 0;
    virtual int controlInletsOf(const std::string& name) = 0;
    virtual int controlOutletsOf(const std::string& name) = 0;
    virtual std::string controlInletParam(const std::string& name, int inlet) = 0;
    virtual std::string controlOutletValue(const std::string& name, int outlet) = 0;
    virtual std::vector<ControlCord> controlCordsInScope(const std::string& scope) = 0;
    virtual Organism* liveOrganism(const std::string& name) = 0;
    virtual bool bypassed(const std::string& name) = 0;
    virtual bool silent(const std::string& name) = 0;
    virtual void nodeActivity(const std::string& name, float& audioPeak, float& midiCount) = 0;
    virtual juce::uint64 changeStamp() const = 0;
};

}
