// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>

#include "core/params/ParamSchema.h"
#include "io/PatchDocument.h"

namespace hum {

using DefaultValue = std::function<double(const ParamDesc&)>;

std::string freeOrganismName(const PatchDocumentModel& doc, const std::string& plain);
OrganismModel newOrganismModel(const std::string& className, const std::string& name,
                               const DefaultValue& defaultValue);
OrganismModel reclassedOrganismModel(const OrganismModel& old, const std::string& newClass,
                                     const DefaultValue& defaultValue);

}
