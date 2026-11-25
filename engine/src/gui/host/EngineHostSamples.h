// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "gui/host/BrickHost.h"
#include "gui/host/HostCore.h"
#include "gui/host/SampleEdits.h"
#include "hum/Pattern.h"

namespace hum {

class SampleEditor : public SampleEdits {
public:
    static constexpr double kLongestRepairSeconds = 0.05;

    SampleEditor(BrickHost& host, HostCore& core) : host_(host), doc_(core), patternSync_(core), recording_(core) {}

    bool drawSamples(const std::string& node, int clip, std::int64_t start,
                     const std::vector<float>& mono) override;
    bool repairSamples(const std::string& node, int clip, std::int64_t start, int length) override;
    bool restoreOriginal(const std::string& node, int clip) override;
    bool canRestore(const std::string& node, int clip) const override;
    unsigned contentStamp(const std::string& file) const override;
    void syncRevisions() override;

private:
    struct Edit {
        std::int64_t start = 0;
        std::vector<std::vector<float>> before, after;
    };
    struct Working {
        std::vector<std::vector<float>> pcm;
        double rate = 0.0;
        int bits = 24;
        std::vector<Edit> edits;
        int applied = 0;
    };

    const PatternChannel* clipOf(const std::string& node, int clip) const;
    Working* load(const std::string& file);
    std::string makeCopy(const std::string& original, const Working& source);
    std::string prepare(std::string file, Working*& out);
    bool commit(const std::string& copy, Working& w, Edit edit);
    static void put(Working& w, const Edit& e, bool forward);
    bool save(const std::string& copy, const Working& w);
    void repoint(const std::string& from, const std::string& to);

    BrickHost& host_;
    HostDocument& doc_;
    HostPatterns& patternSync_;
    HostRecording& recording_;
    std::map<std::string, Working> working_;
    std::map<std::string, unsigned> stamps_;
};

}
