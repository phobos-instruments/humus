#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "gui/BounceWants.h"
#include "gui/Localisation.h"
#include "io/Mp3Writer.h"

#include "hum/dsp/DspMath.h"

namespace hum::receipt {

struct Made {
    juce::String file;
    juce::String what;
};

struct Summary {
    std::vector<Made> files;
    juce::String duration;
    juce::String size;
    juce::String time;
};

inline double seconds(const BounceWants& w, double tempo) {
    const double bpm = tempo > 0.0 ? tempo : 120.0;
    return std::max(0.0, w.toBeat - w.fromBeat) * kSecondsPerMinute / bpm;
}

inline double pictureBytesPerSecond(const BounceWants& w) {
    return movieBitsPerSecond(w.width, w.height, w.fps, w.quality) / 8.0;
}

struct SoundKind {
    const char* extension;
    juce::String label;
    double bytesPerSecond;
};

inline std::vector<SoundKind> soundKinds() {
    return {{"wav", tr("bounce.wav", "WAV, 24-bit"), 288.0e3},
            {"flac", tr("bounce.flac", "FLAC, 24-bit"), 170.0e3},
            {"ogg", tr("bounce.ogg", "Ogg Vorbis"), 20.0e3},
            {"mp3", tr("bounce.mp3", "MP3, 245 kbit/s"), mp3BytesPerSecond()}};
}

inline const SoundKind& soundKindOf(const std::string& extension) {
    static const auto all = soundKinds();
    for (const auto& kind : all)
        if (extension == kind.extension) return kind;
    return all[0];
}

inline double soundBytesPerSecond(const BounceWants& w) {
    if (w.video) return 32.0e3;
    return soundKindOf(w.soundKind).bytesPerSecond;
}

inline juce::String bytesText(double bytes) {
    if (bytes >= 1.0e9) return juce::String(bytes / 1.0e9, 1) + " GB";
    if (bytes >= 1.0e6) return juce::String((int) std::lround(bytes / 1.0e6)) + " MB";
    return juce::String(std::max(1, (int) std::lround(bytes / 1.0e3))) + " KB";
}

inline juce::String sizeText(double bytes) {
    if (bytes <= 0.0) return {};
    return "~ " + bytesText(bytes);
}

inline juce::String clockText(double span) {
    const int whole = (int) std::lround(span);
    if (whole < 60) return juce::String(std::max(1, whole)) + " "
                           + tr("receipt.seconds", "seconds");
    const int minutes = whole / 60, rest = whole % 60;
    juce::String out = juce::String(minutes) + " "
                       + (minutes == 1 ? tr("receipt.minute", "minute")
                                       : tr("receipt.minutes", "minutes"));
    if (rest > 0) out += " " + juce::String(rest) + " " + tr("receipt.seconds", "seconds");
    return out;
}

inline juce::String barsText(const BounceWants& w) {
    const double bars = (w.toBeat - w.fromBeat) / 4.0;
    if (bars < 0.5 || std::abs(bars - std::lround(bars)) > 0.01) return {};
    return " (" + juce::String((int) std::lround(bars)) + " "
           + tr("receipt.bars", "bars") + ")";
}

inline juce::String sizeName(const BounceWants& w) {
    return juce::String(w.width) + " x " + juce::String(w.height) + ", "
           + juce::String((int) std::lround(w.fps)) + " fps";
}

inline Summary summarise(const BounceWants& w, double tempo) {
    Summary out;
    const double span = seconds(w, tempo);
    double bytes = 0.0;

    if (w.video) {
        Made movie;
        movie.file = w.movieFile().getFileName();
        movie.what = (w.audio ? tr("receipt.both", "Picture and sound in one file.")
                              : tr("receipt.picture", "Picture only, no sound."))
                     + " H.264, " + sizeName(w);
        out.files.push_back(std::move(movie));
        bytes += span * pictureBytesPerSecond(w);
        if (w.audio) bytes += span * soundBytesPerSecond(w);
    } else if (w.audio) {
        Made sound;
        sound.file = w.soundFile().getFileName();
        sound.what = tr("receipt.sound", "Sound only.") + " "
                     + soundKindOf(w.soundKind).label;
        out.files.push_back(std::move(sound));
        bytes += span * soundBytesPerSecond(w);
    }

    if (w.midi) {
        Made notes;
        notes.file = w.notesFile().getFileName();
        notes.what = tr("receipt.notes", "The notes, as a MIDI file.");
        out.files.push_back(std::move(notes));
    }

    if (out.files.empty()) return out;
    out.duration = clockText(span) + barsText(w);
    const double takes = span * (w.video ? (double) w.width * w.height / (1280.0 * 720.0) / 3.0
                                         : 0.05);
    out.size = sizeText(bytes);
    out.time = clockText(std::max(1.0, takes));
    return out;
}

}
