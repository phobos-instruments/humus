#pragma once
#include <algorithm>
#include <vector>

namespace hum {

class SoundHold {
public:
    void take(const float* const* channels, int count, int frames, int want) {
        held_.assign((size_t) std::max(1, want), {});
        for (int c = 0; c < (int) held_.size(); ++c) {
            const auto* src = channels[std::min(c, count - 1)];
            held_[(size_t) c].assign(src, src + frames);
        }
        sent_ = 0;
    }

    void clear() {
        held_.clear();
        sent_ = 0;
    }

    bool empty() const { return held_.empty(); }
    int channels() const { return (int) held_.size(); }
    int total() const { return held_.empty() ? 0 : (int) held_[0].size(); }
    int sent() const { return sent_; }

    int ready(int block, double upTo, double rate) const {
        if (held_.empty() || sent_ >= total() || rate <= 0.0) return 0;
        if ((double) sent_ / rate > upTo) return 0;
        return std::min(block, total() - sent_);
    }

    float at(int channel, int index) const { return held_[(size_t) channel][(size_t) index]; }
    void advance(int frames) { sent_ += frames; }

private:
    std::vector<std::vector<float>> held_;
    int sent_ = 0;
};

}
