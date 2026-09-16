// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <string>

#include "gui/host/BrickHost.h"

namespace hum {

class HostedPlugin;
class PluginNode;

class PluginsHost : public virtual BrickHost {
public:
    ~PluginsHost() override = default;

    virtual void batchLiveParamValues(const std::string& name,
                              const std::vector<hum::Parameter>& params,
                              std::vector<double>& out) const = 0;
    virtual HostedPlugin* hostedPluginFor(const std::string& name) = 0;
    virtual PluginNode* pluginNodeFor(const std::string& name) = 0;
    virtual void pokeLiveRefresh() = 0;
    virtual int inletsOf(const std::string& name) = 0;
    virtual int outletsOf(const std::string& name) = 0;
    virtual int midiInletsOf(const std::string& name) = 0;
    virtual int midiOutletsOf(const std::string& name) = 0;
};

}
