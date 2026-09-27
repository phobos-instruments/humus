// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace hum {

class Organism;
struct PatchDocumentModel;

class ModelHost {
public:
    virtual ~ModelHost() = default;

    virtual const PatchDocumentModel& model() const = 0;
    virtual Organism* liveOrganism(const std::string& name) = 0;

    virtual double liveParamValue(const std::string& organism, const std::string& param) = 0;
    virtual bool isLiveTracked(const std::string& organism, const std::string& param) const = 0;
    virtual bool isExternallyControlled(const std::string& organism,
                                        const std::string& param) const = 0;
    virtual bool rollLocked(const std::string& organism, const std::string& param) const = 0;
    virtual std::string controlSummary(const std::string& organism, const std::string& param) const {
        (void) organism;
        (void) param;
        return {};
    }
    virtual bool firedRecently(const std::string& organism, const std::string& param) const {
        (void) organism;
        (void) param;
        return false;
    }

    virtual void editParam(const std::string& organism, const std::string& param, double value) = 0;
    virtual void beginParamDrag(const std::string& organism, const std::string& param) = 0;
    virtual void endParamDrag() = 0;

    virtual std::string liveParamText(const std::string& organism, const std::string& param) = 0;
    virtual void setParam(const std::string& organism, const std::string& param, double value) = 0;
    virtual void setParamText(const std::string& organism, const std::string& param,
                              const std::string& text) = 0;
    virtual int nodeMeter(const std::string& name, float* levels, int maxCh) = 0;
    virtual unsigned nodePictureGeneration(const std::string& name) {
        (void) name;
        return 0;
    }
    virtual int nodeInletMeter(const std::string& name, float* levels, int maxCh) {
        (void) name;
        (void) levels;
        (void) maxCh;
        return 0;
    }

    virtual std::vector<std::pair<int, std::string>> choiceItems(const std::string& source,
                                                                 const std::string& organism = {}) = 0;
    virtual std::string replaceOrganism(const std::string& name, const std::string& newClass) = 0;
    virtual void noteTopologyChanged() = 0;

    virtual std::string translated(const std::string& key, const std::string& written) const = 0;
};

}
