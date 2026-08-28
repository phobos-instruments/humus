#pragma once
#include <memory>

namespace hum {

class TimeStretcher {
public:
    TimeStretcher();
    ~TimeStretcher();

    bool available() const;
    void prepare(double sampleRate, int channels, int maxBlock);
    void reset();

    void process(const float* const* in, int inN,
                 float* const* out, int outN, double tempoFactor);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
