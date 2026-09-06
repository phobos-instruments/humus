#pragma once
#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

namespace hum {

class CoreMLNet {
public:
    virtual ~CoreMLNet() = default;

    static std::unique_ptr<CoreMLNet> load(const juce::File& package);

    virtual bool run(const float* in, int count, int side,
                     std::vector<std::vector<float>>& outs, int numOuts) = 0;

protected:
    CoreMLNet() = default;
};

}
