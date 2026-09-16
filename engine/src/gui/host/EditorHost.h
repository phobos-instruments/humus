// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "gui/host/BrickHost.h"

namespace hum {

class HostedPlugin;

class EditorHost : public virtual BrickHost {
public:
    ~EditorHost() override = default;

    virtual std::string replaceOrganism(const std::string& name, const std::string& newClass) = 0;
    virtual bool isLiveTracked(const std::string& organism, const std::string& param) const = 0;
    virtual bool isExternallyControlled(const std::string& organism,
                                        const std::string& param) const = 0;
    virtual void noteTopologyChanged() = 0;
    virtual HostedPlugin* hostedPluginFor(const std::string& name) = 0;
    virtual std::vector<std::pair<int, std::string>> choiceItems(const std::string& source,
                                                         const std::string& organism = {}) = 0;
};

}
