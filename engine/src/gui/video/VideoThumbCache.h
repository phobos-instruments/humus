// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/video/FrameImage.h"
#include "gui/video/VideoThumbStore.h"
#include "gui/video/VideoLayer.h"
#include "gui/video/VideoProbe.h"

namespace hum {

class VideoThumbCache {
public:
    static constexpr int kThumbH = 36;
    static constexpr int kMaxThumbs = 240;

    static VideoThumbCache& instance() { static VideoThumbCache c; return c; }

    struct Strip {
        std::vector<juce::Image> frames;
        double interval = 0.0;
        double seconds = 0.0;
        int width = 0, height = kThumbH;
        bool ready = false;
        bool trouble = false;
    };

    const Strip* get(const std::string& path, std::function<void()> onReady) {
        if (path.empty()) return nullptr;
        const std::int64_t mtime = fileOf(path).getLastModificationTime().toMilliseconds();
        auto& e = cache_[path];
        if (e.strip && e.strip->ready && e.mtime == mtime) return e.strip.get();
        if (!e.loading || e.mtime != mtime) {
            e.mtime = mtime;
            e.loading = true;
            if (onReady) readyCbs_.push_back(std::move(onReady));
            worker_.enqueue(path, mtime);
        } else if (onReady) {
            readyCbs_.push_back(std::move(onReady));
        }
        return nullptr;
    }

    const Strip* prime(const std::string& path) {
        if (path.empty()) return nullptr;
        auto& e = cache_[path];
        e.mtime = fileOf(path).getLastModificationTime().toMilliseconds();
        auto strip = std::make_shared<Strip>();
        compute(path, *strip);
        e.strip = strip;
        e.loading = false;
        return e.strip.get();
    }

    static const juce::Image* frameAt(const Strip& s, double seconds) {
        if (s.frames.empty() || s.interval <= 0.0) return nullptr;
        const int i = (int) std::floor(seconds / s.interval);
        const auto& img = s.frames[(size_t) std::clamp(i, 0, (int) s.frames.size() - 1)];
        return img.isValid() ? &img : nullptr;
    }

    void forget() { cache_.clear(); }

    ~VideoThumbCache() { worker_.stopThread(4000); }

private:
    struct Entry {
        std::int64_t mtime = 0;
        std::shared_ptr<Strip> strip;
        bool loading = false;
    };

    static juce::File fileOf(const std::string& path) {
        std::string uri = path;
        if (uri.rfind("file://", 0) == 0) uri = uri.substr(7);
        return juce::File(juce::String(juce::CharPointer_UTF8(uri.c_str())));
    }

    static void compute(const std::string& path, Strip& out) {
        out.ready = true;
        const auto tape = fileOf(path);
        if (thumbstore::Strip stored; thumbstore::read(tape, kThumbH, stored)) {
            out.frames = std::move(stored.frames);
            out.interval = stored.interval;
            out.seconds = stored.seconds;
            out.width = stored.width;
            return;
        }
        auto layer = VideoLayer::create(true);
        if (layer == nullptr) {
            out.trouble = true;
            return;
        }
        const double len = probeVideoSeconds(tape);
        if (len <= 0.0) {
            out.trouble = true;
            return;
        }
        layer->setPaused(true);
        layer->load(tape.getFullPathName());
        out.seconds = len;
        const int n = std::clamp((int) std::ceil(len), 1, kMaxThumbs);
        out.interval = len / n;
        const double slack = std::max(out.interval * 0.5, 0.06);
        for (int i = 0; i < n; ++i) {
            const double t = (i + 0.5) * out.interval;
            layer->chase(t, 0.0);
            std::shared_ptr<const VideoLayer::Frame> hit;
            for (int r = 0; r < 150 && hit == nullptr; ++r) {
                auto fr = layer->latestFrame();
                if (fr != nullptr && fr->pts >= 0.0 && std::abs(fr->pts - t) <= slack) hit = fr;
                else juce::Thread::sleep(10);
            }
            juce::Image img;
            if (hit != nullptr) {
                const int w = out.width > 0 ? out.width
                            : std::max(8, (int) std::lround((double) kThumbH * hit->width
                                                            / std::max(1, hit->height)));
                img = imageOfFrame(*hit, w, kThumbH);
                if (img.isValid()) out.width = w;
            }
            if (img.isValid()) out.frames.push_back(std::move(img));
            else out.frames.push_back(out.frames.empty() ? juce::Image() : out.frames.back());
        }
        thumbstore::Strip keep;
        keep.frames = out.frames;
        keep.interval = out.interval;
        keep.seconds = out.seconds;
        keep.width = out.width;
        keep.height = kThumbH;
        thumbstore::write(tape, kThumbH, keep);
    }

    class Worker : public juce::Thread {
    public:
        Worker() : juce::Thread("video-thumbs") {}

        void enqueue(const std::string& path, std::int64_t mtime) {
            {
                const juce::ScopedLock sl(lock_);
                jobs_.emplace_back(path, mtime);
            }
            if (!isThreadRunning()) startThread();
            notify();
        }

        void run() override {
            while (!threadShouldExit()) {
                std::pair<std::string, std::int64_t> job;
                {
                    const juce::ScopedLock sl(lock_);
                    if (!jobs_.empty()) {
                        job = jobs_.front();
                        jobs_.pop_front();
                    }
                }
                if (job.first.empty()) {
                    wait(200);
                    continue;
                }
                auto strip = std::make_shared<Strip>();
                compute(job.first, *strip);
                const auto path = job.first;
                const auto mtime = job.second;
                juce::MessageManager::callAsync([path, mtime, strip] {
                    auto& self = instance();
                    auto& e = self.cache_[path];
                    if (e.mtime != mtime) return;
                    e.strip = strip;
                    e.loading = false;
                    auto cbs = std::move(self.readyCbs_);
                    self.readyCbs_.clear();
                    for (auto& cb : cbs) cb();
                });
            }
        }

    private:
        juce::CriticalSection lock_;
        std::deque<std::pair<std::string, std::int64_t>> jobs_;
    };

    std::map<std::string, Entry> cache_;
    std::vector<std::function<void()>> readyCbs_;
    Worker worker_;
};

}
