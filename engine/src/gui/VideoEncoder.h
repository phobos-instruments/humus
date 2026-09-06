#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

#include <juce_core/juce_core.h>

namespace hum {

enum class MovieKind { Hap, H264 };

inline constexpr int kQualityFinest = 14, kQualityDefault = 19, kQualityCoarsest = 28;

inline constexpr double kBitsAtSevenTwenty = 4.64e6;

inline double movieBitsPerSecond(int width, int height, double fps, int quality) {
    const double pixels = (double) std::max(1, width) * std::max(1, height) / (1280.0 * 720.0);
    const double rate = (fps > 0.0 ? fps : 30.0) / 30.0;
    const double steps = (double) (kQualityDefault - quality) / 6.0;
    return juce::jlimit(200.0e3, 200.0e6,
                        pixels * rate * kBitsAtSevenTwenty * std::pow(2.0, steps));
}

class MovieThread {
public:
    MovieThread();
    ~MovieThread();

private:
    bool joined_ = false;
};

class VideoEncoder {
public:
    virtual ~VideoEncoder() = default;
    virtual bool ok() const = 0;
    virtual int frameCount() const = 0;
    virtual bool openSound(int channels, double sampleRate) = 0;
    virtual bool addSound(const float* const* channels, int count, int frames) = 0;
    virtual bool addFrame(const std::uint8_t* rgba) = 0;
    virtual bool close() = 0;
};

bool movieKindAvailable(MovieKind kind);
bool movieLogIsLoud();
const char* movieKindExtension(MovieKind kind);
std::unique_ptr<VideoEncoder> makeMovieWriter(MovieKind kind, const juce::File& file, int width,
                                              int height, double fps,
                                              int quality = kQualityDefault, bool live = false);

inline int keyframeInterval(double fps, bool live) {
    return std::max(1, (int) std::lround(fps * (live ? 0.5 : 2.0)));
}

}
