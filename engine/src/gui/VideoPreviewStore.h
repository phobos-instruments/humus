#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

class VideoPreviewStore {
public:
    static VideoPreviewStore& instance() {
        static VideoPreviewStore s;
        return s;
    }

    void want(const std::string& node, bool on) {
        const juce::ScopedLock sl(lock_);
        if (on) ++slots_[node].watchers;
        else if (auto it = slots_.find(node);
                 it != slots_.end() && --it->second.watchers <= 0) slots_.erase(it);
    }
    bool wanted(const std::string& node) const {
        const juce::ScopedLock sl(lock_);
        const auto it = slots_.find(node);
        return it != slots_.end() && it->second.watchers > 0;
    }
    std::vector<std::string> wantedNodes() const {
        const juce::ScopedLock sl(lock_);
        std::vector<std::string> out;
        for (const auto& [n, s] : slots_)
            if (s.watchers > 0) out.push_back(n);
        return out;
    }

    void publish(const std::string& node, int w, int h, const std::uint8_t* rgba) {
        if (w <= 0 || h <= 0 || rgba == nullptr) return;
        const juce::ScopedLock sl(lock_);
        auto it = slots_.find(node);
        if (it == slots_.end()) return;
        auto& s = it->second;
        s.w = w;
        s.h = h;
        s.rgba.assign(rgba, rgba + (size_t) w * (size_t) h * 4u);
        ++s.generation;
    }

    bool take(const std::string& node, unsigned& generation, juce::Image& out) const {
        const juce::ScopedLock sl(lock_);
        const auto it = slots_.find(node);
        if (it == slots_.end() || it->second.generation == generation) return false;
        const auto& s = it->second;
        generation = s.generation;
        if (s.w <= 0 || s.h <= 0 || s.rgba.empty()) { out = {}; return true; }
        juce::Image img(juce::Image::ARGB, s.w, s.h, false);
        {
            const juce::Image::BitmapData bm(img, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < s.h; ++y) {
                const std::uint8_t* src = s.rgba.data() + (size_t) y * (size_t) s.w * 4u;
                for (int x = 0; x < s.w; ++x, src += 4)
                    bm.setPixelColour(x, y, juce::Colour(src[0], src[1], src[2], src[3]));
            }
        }
        out = img;
        return true;
    }

private:
    struct Slot {
        int watchers = 0;
        int w = 0, h = 0;
        unsigned generation = 1;
        std::vector<std::uint8_t> rgba;
    };
    juce::CriticalSection lock_;
    std::map<std::string, Slot> slots_;
};

}
