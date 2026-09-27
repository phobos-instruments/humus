// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <vector>

#include "gui/host/ModEdits.h"
#include "gui/host/HostCore.h"
#include "gui/host/BrickHost.h"
#include "core/graph/ModControl.h"

namespace hum {


class ModHost : public ModEdits {
public:
    ModHost(BrickHost& host, HostCore& core) : host_(host), doc_(core), nodes_(core) {}

    void mapRoute(const std::string& source, const std::string& value,
                  const std::string& organism, const std::string& param,
                  double min, double max) override;
    void clearRoute(const std::string& source, const std::string& value,
                    const std::string& organism, const std::string& param) override;
    void clearForOrganism(const std::string& organism) override;
    void renameOrganism(const std::string& oldName, const std::string& newName) override;
    const ModControlMap& map() const override { return map_; }
    void setShape(const std::string& source, const std::string& value,
                  const std::string& organism, const std::string& param,
                  const ControlShape& shape) override;

    std::vector<ModParamUpdate> tick(double dt);

    std::vector<std::pair<std::string, std::string>> availableSources() const override;

    void syncMapFromModel();
    void syncMapToModel();

private:
    BrickHost& host_;
    HostDocument& doc_;
    HostNodes& nodes_;
    ModControlMap map_;
};

}
