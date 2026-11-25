// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <cmath>
#include <string>

#include "core/packs/PackRegistry.h"
#include "core/params/ParamBankSwitch.h"
#include "core/params/ParamSchema.h"
#include "gui/host/EngineHostPattern.h"
#include "hum/ParamBanks.h"

namespace hum {

int EngineHost::paramBankShown(const std::string& organism, const std::string& param) const {
    if (restoringState_) return -1;
    const auto* cm = model_.byName(organism);
    if (cm == nullptr) return -1;
    const auto* m = PackRegistry::instance().classManifest(cm->displayClass);
    if (m == nullptr || m->bankSelect.empty() || m->bankSelect != param) return -1;
    return parambanks::shownBank(*m, schemaFor(cm->classRaw), cm->properties);
}

void EngineHost::switchParamBank(const std::string& organism, int from, int to) {
    auto* cm = mutableByName(organism);
    const auto* m = cm != nullptr ? PackRegistry::instance().classManifest(cm->displayClass) : nullptr;
    if (m == nullptr || from == to || from < 0 || to < 0) return;
    const auto shown = parambanks::shownValues(*m, schemaFor(cm->classRaw), cm->properties);
    recordPatternRevert(organism);
    const auto load = parambanks::switchBank(cm->pattern, shown, from, to);
    ensurePatternProperty(*cm);
    {
        const DerivedControlScope derived(*this);
        for (const auto& [name, value] : load) setParam(organism, name, (double) value);
    }
    markPatternEdited();
    syncPattern(organism);
}

}

namespace hum {

void PatternHost::copyParamBankTo(const std::string& name, int bank) {
    const auto* cm = doc_.document().byName(name);
    const auto* m = cm != nullptr ? PackRegistry::instance().classManifest(cm->displayClass) : nullptr;
    if (m == nullptr || m->bankSelect.empty() || bank < 0) return;
    const auto& schema = schemaFor(cm->classRaw);
    if (parambanks::shownBank(*m, schema, cm->properties) == bank) return;
    const auto values = encodeBankValues(parambanks::shownValues(*m, schema, cm->properties));
    patternSync_.recordPatternRevert(name);
    auto* edit = doc_.mutableByName(name);
    parambanks::bankChannel(edit->pattern, bank).matrix = values;
    edit->pattern.present = true;
    ensurePatternProperty(*edit);
    patternSync_.markPatternEdited();
    patternSync_.syncPattern(name);
}

}
