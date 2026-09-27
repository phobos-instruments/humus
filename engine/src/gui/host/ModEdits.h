// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "core/graph/ModControl.h"
#include "core/net/ControlShape.h"

namespace hum {

class ModEdits {
public:
    virtual ~ModEdits() = default;

    virtual void mapRoute(const std::string& source, const std::string& value, const std::string& organism,
                          const std::string& param, double min, double max) = 0;
    virtual void clearRoute(const std::string& source, const std::string& value, const std::string& organism,
                            const std::string& param) = 0;
    virtual void clearForOrganism(const std::string& organism) = 0;
    virtual void renameOrganism(const std::string& oldName, const std::string& newName) = 0;
    virtual const ModControlMap& map() const = 0;
    virtual void setShape(const std::string& source, const std::string& value, const std::string& organism,
                          const std::string& param, const ControlShape& shape) = 0;
    virtual std::vector<std::pair<std::string, std::string>> availableSources() const = 0;
};

}
