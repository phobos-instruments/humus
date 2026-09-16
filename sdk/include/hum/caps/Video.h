// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hum {

class CamPreviewSource {
public:
    struct Frame { int width = 0, height = 0; std::vector<std::uint8_t> rgba; };
    virtual ~CamPreviewSource() = default;
    virtual bool camActive() const = 0;
    virtual unsigned camGeneration() const = 0;
    virtual Frame camFrame() const = 0;
    virtual void camSignals(float& x, float& y, float& motion, float& brightness) const = 0;

    struct Skeleton {
        bool supported = false;
        int points = 0;
        int groupSize = 0;
        std::array<std::array<float, 2>, 48> pt{};
        const int (*bones)[2] = nullptr;
        int boneCount = 0;
    };
    virtual Skeleton camSkeleton() const { return {}; }

    virtual std::string camUnavailable() const { return {}; }

    virtual bool camSourceHeld() const { return false; }

    struct NativePicture {
        void* buffer = nullptr;
        int width = 0, height = 0;
        bool mirrored = false;
        std::shared_ptr<const void> hold;
    };
    virtual NativePicture camNativePicture() const { return {}; }
};

class SigilSource {
public:
    static constexpr int kMaxPoints = 16;
    struct Prim {
        unsigned char kind = 0;
        unsigned char role = 1;
        bool closed = false;
        bool filled = false;
        float alpha = 1.0f;
        float size = 0.02f;
        int points = 0;
        float pt[kMaxPoints][2] = {};
    };
    virtual ~SigilSource() = default;
    virtual int sigil(Prim* out, int capacity, double timeSeconds) = 0;
};

class GestureFeatureSource {
public:
    static constexpr int kMaxFeatures = 8;
    virtual ~GestureFeatureSource() = default;
    virtual int gestureFeatureCount() const = 0;
    virtual bool gestureFeaturesLive(float* out) const = 0;
    virtual const char* gestureHoldPrompt() const { return "hold the pose..."; }
    virtual const char* gestureAbsentPrompt() const { return "step into view..."; }
};

class VideoNode {
public:
    virtual ~VideoNode() = default;
    virtual int numVideoInputs() const = 0;
    virtual int numVideoOutputs() const = 0;
    virtual unsigned videoLaunchCount() const { return 0; }
};

class VideoFxNode {
public:
    virtual ~VideoFxNode() = default;
};

class VideoTimelineSource {
public:
    struct Cue {
        int clip = -1;
        double seconds = 0.0;
        double rate = 1.0;
        float level = 1.0f;
        bool rolling = false;
    };
    virtual ~VideoTimelineSource() = default;
    virtual Cue cue() const = 0;
    virtual Cue upcoming() const = 0;
    virtual std::string cueFile(int clip) const = 0;

    virtual Cue cueAt(double beat, double tempo) const {
        (void) beat;
        (void) tempo;
        return cue();
    }
    virtual Cue upcomingAt(double beat, double tempo) const {
        (void) beat;
        (void) tempo;
        return upcoming();
    }
};

class VideoPadSource {
public:
    static constexpr int kMaxClips = 8;
    struct ClipState {
        int active = -1;
        int outgoing = -1;
        float phase = 1.0f;
        unsigned launches = 0;
    };
    virtual ~VideoPadSource() = default;
    virtual int clipCount() const = 0;
    virtual ClipState clipState() const = 0;
    virtual void noteClipLength(int, double) {}
};

struct PixelField;

class PixelFieldSource {
public:
    virtual ~PixelFieldSource() = default;
    virtual unsigned pixelFieldGeneration() const = 0;
    virtual bool copyPixelField(PixelField& out) const = 0;
};

class VideoFrameSink {
public:
    struct Picture {
        int width = 0, height = 0;
        bool bgra = false;
        const std::uint8_t* pixels = nullptr;
        void* native = nullptr;
        std::shared_ptr<const void> hold;
        bool mirrored = false;
    };

    virtual ~VideoFrameSink() = default;
    virtual void pushVideoFrame(const Picture& picture) = 0;
    virtual void setVideoCordAttached(bool) {}
    virtual void setVideoSourceHeld(bool) {}
};

}
