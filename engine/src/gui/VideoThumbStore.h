#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

#include <juce_cryptography/juce_cryptography.h>
#include <juce_graphics/juce_graphics.h>

#include "core/AppPaths.h"
#include "gui/AppSettings.h"

namespace hum::thumbstore {

inline constexpr const char* kBudgetKey = "video.thumbCacheMB";
inline constexpr int kDefaultBudgetMB = 256;
inline constexpr int kMinBudgetMB = 0;
inline constexpr int kMaxBudgetMB = 8192;

struct Strip {
    std::vector<juce::Image> frames;
    double interval = 0.0;
    double seconds = 0.0;
    int width = 0, height = 0;
};

inline juce::File& rootForTesting() {
    static juce::File f;
    return f;
}

inline int& budgetForTesting() {
    static int mb = -1;
    return mb;
}

inline juce::File dir() {
    if (const auto& t = rootForTesting(); t != juce::File()) return t;
    return appDataDir().getChildFile("Thumbnails");
}

inline int budgetMB() {
    if (const int mb = budgetForTesting(); mb >= 0) return mb;
    return juce::jlimit(kMinBudgetMB, kMaxBudgetMB,
                        AppSettings::instance().getInt(kBudgetKey, kDefaultBudgetMB));
}

inline void setBudgetMB(int mb) {
    AppSettings::instance().set(kBudgetKey, juce::jlimit(kMinBudgetMB, kMaxBudgetMB, mb));
}

inline juce::int64 bytesUsed() {
    juce::int64 total = 0;
    for (const auto& f : dir().findChildFiles(juce::File::findFiles, false, "*.strip"))
        total += f.getSize();
    return total;
}

inline void clear() {
    for (const auto& f : dir().findChildFiles(juce::File::findFiles, false, "*.strip"))
        f.deleteFile();
}

inline void prune() {
    const juce::int64 budget = (juce::int64) budgetMB() * 1024 * 1024;
    auto files = dir().findChildFiles(juce::File::findFiles, false, "*.strip");
    juce::int64 total = 0;
    for (const auto& f : files) total += f.getSize();
    if (total <= budget) return;
    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b) {
        return a.getLastModificationTime() < b.getLastModificationTime();
    });
    for (const auto& f : files) {
        if (total <= budget) return;
        total -= f.getSize();
        f.deleteFile();
    }
}

inline juce::File fileFor(const juce::File& tape, int thumbHeight) {
    const auto key = tape.getFullPathName() + "|" + juce::String(tape.getSize()) + "|"
                   + juce::String(tape.getLastModificationTime().toMilliseconds()) + "|"
                   + juce::String(thumbHeight);
    return dir().getChildFile(juce::String(juce::SHA256(key.toUTF8()).toHexString().substring(0, 32)) + ".strip");
}

inline bool read(const juce::File& tape, int thumbHeight, Strip& out) {
    if (budgetMB() <= 0) return false;
    const auto f = fileFor(tape, thumbHeight);
    juce::FileInputStream in(f);
    if (!in.openedOk() || in.readInt() != 0x48545331) return false;
    const int count = in.readInt(), w = in.readInt(), h = in.readInt();
    out.interval = in.readDouble();
    out.seconds = in.readDouble();
    if (count <= 0 || w <= 0 || h <= 0 || out.interval <= 0.0) return false;
    std::vector<juce::uint8> live((size_t) count);
    if (in.read(live.data(), count) != count) return false;
    juce::MemoryBlock png;
    in.readIntoMemoryBlock(png);
    const auto sheet = juce::ImageFileFormat::loadFrom(png.getData(), png.getSize());
    if (!sheet.isValid() || sheet.getWidth() < count * w || sheet.getHeight() < h) return false;
    out.width = w;
    out.height = h;
    out.frames.clear();
    for (int i = 0; i < count; ++i)
        out.frames.push_back(live[(size_t) i] != 0
                                 ? sheet.getClippedImage({i * w, 0, w, h}).createCopy()
                                 : juce::Image());
    f.setLastModificationTime(juce::Time::getCurrentTime());
    return true;
}

inline void write(const juce::File& tape, int thumbHeight, const Strip& strip) {
    if (budgetMB() <= 0 || strip.frames.empty() || strip.width <= 0 || strip.height <= 0) return;
    const int count = (int) strip.frames.size();
    juce::Image sheet(juce::Image::RGB, count * strip.width, strip.height, true);
    {
        juce::Graphics g(sheet);
        for (int i = 0; i < count; ++i)
            if (strip.frames[(size_t) i].isValid())
                g.drawImageAt(strip.frames[(size_t) i], i * strip.width, 0);
    }
    juce::MemoryOutputStream png;
    if (!juce::PNGImageFormat().writeImageToStream(sheet, png)) return;
    dir().createDirectory();
    const auto f = fileFor(tape, thumbHeight);
    juce::TemporaryFile tmp(f);
    {
        juce::FileOutputStream out(tmp.getFile());
        if (!out.openedOk()) return;
        out.writeInt(0x48545331);
        out.writeInt(count);
        out.writeInt(strip.width);
        out.writeInt(strip.height);
        out.writeDouble(strip.interval);
        out.writeDouble(strip.seconds);
        for (const auto& img : strip.frames) out.writeByte(img.isValid() ? 1 : 0);
        out.write(png.getData(), png.getDataSize());
    }
    tmp.overwriteTargetFileWithTemporary();
    prune();
}

}
