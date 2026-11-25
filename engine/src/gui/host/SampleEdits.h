// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace hum {

class SampleEdits {
public:
    virtual ~SampleEdits() = default;

    virtual bool drawSamples(const std::string& node, int clip, std::int64_t start,
                             const std::vector<float>& mono) = 0;
    virtual bool repairSamples(const std::string& node, int clip, std::int64_t start, int length) = 0;
    virtual bool restoreOriginal(const std::string& node, int clip) = 0;
    virtual bool canRestore(const std::string& node, int clip) const = 0;
    virtual unsigned contentStamp(const std::string& file) const = 0;
    virtual void syncRevisions() = 0;
};

}
