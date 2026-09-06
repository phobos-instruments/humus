#pragma once
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/MediaProbe.h"
#include "gui/EngineHost.h"
#include "gui/Localisation.h"
#include "gui/LookAndFeel.h"
#include "gui/VideoProbe.h"
#include "hum/dsp/DspMath.h"

namespace hum::mediainfo {

using Row = std::pair<juce::String, juce::String>;

struct Report {
    juce::String title;
    std::vector<Row> rows;
    juce::File file;
};

inline juce::String clock(double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    const int whole = (int) seconds;
    const int ms = (int) std::llround((seconds - whole) * 1000.0);
    return juce::String::formatted("%d:%02d.%03d", whole / 60, whole % 60, ms);
}

inline juce::String seconds(double s) { return juce::String(s, 3) + " s"; }

inline juce::String beats(double b) { return juce::String(b, 3); }

inline void heading(std::vector<Row>& rows, const juce::String& text) {
    if (!rows.empty()) rows.push_back({"", ""});
    rows.push_back({text, ""});
}

inline void movieRows(std::vector<Row>& rows, const media::Movie& m) {
    if (!m.brand.empty()) rows.push_back({tr("media-info.container", "Container"), juce::String(m.brand).trim()});
    if (m.seconds > 0.0) rows.push_back({tr("media-info.duration", "Duration"), clock(m.seconds) + "  (" + seconds(m.seconds) + ")"});
    int n = 0;
    for (const auto& p : m.pictures) {
        heading(rows, m.pictures.size() > 1 ? tr("media-info.picture-n", "Picture") + " " + juce::String(++n)
                                            : tr("media-info.picture", "Picture"));
        rows.push_back({tr("media-info.codec", "Codec"),
                        juce::String(p.codec.empty() ? p.tag : p.codec + " (" + p.tag + ")")});
        if (!p.profile.empty()) rows.push_back({tr("media-info.profile", "Profile"), juce::String(p.profile)});
        if (p.width > 0) rows.push_back({tr("media-info.size", "Size"), juce::String(p.width) + " x " + juce::String(p.height)});
        if (p.fps > 0.0) rows.push_back({tr("media-info.frame-rate", "Frame rate"), juce::String(p.fps, 3) + " fps"});
        if (p.frames > 0) rows.push_back({tr("media-info.frames", "Frames"), juce::String(p.frames)});
        if (p.seconds > 0.0) rows.push_back({tr("media-info.duration", "Duration"), clock(p.seconds)});
        if (p.frames > 0) {
            if (media::everyFrameStandsAlone(p))
                rows.push_back({tr("media-info.keyframes", "Keyframes"), tr("media-info.every-frame", "every frame stands alone")});
            else if (p.keyframes > 0) {
                const double every = (double) p.frames / (double) p.keyframes;
                rows.push_back({tr("media-info.keyframes", "Keyframes"),
                                juce::String(p.keyframes) + ", " + tr("media-info.one-every", "one every")
                                    + " " + juce::String(every, 1) + " " + tr("media-info.frames-unit", "frames")
                                    + (p.fps > 0.0 ? " (" + juce::String(every / p.fps, 2) + " s)" : juce::String())});
            }
        }
        if (p.bytes > 0 && p.seconds > 0.0)
            rows.push_back({tr("media-info.bit-rate", "Bit rate"), juce::String(p.bytes * 8.0 / p.seconds / 1.0e6, 2) + " Mbit/s"});
    }
    n = 0;
    for (const auto& s : m.sounds) {
        heading(rows, m.sounds.size() > 1 ? tr("media-info.sound-n", "Sound") + " " + juce::String(++n)
                                          : tr("media-info.sound", "Sound"));
        rows.push_back({tr("media-info.codec", "Codec"),
                        juce::String(s.codec.empty() ? s.tag : s.codec + " (" + s.tag + ")")});
        if (s.sampleRate > 0.0) rows.push_back({tr("media-info.sample-rate", "Sample rate"), juce::String(s.sampleRate, 0) + " Hz"});
        if (s.channels > 0) rows.push_back({tr("media-info.channels", "Channels"), juce::String(s.channels)});
        if (s.bits > 0) rows.push_back({tr("media-info.bit-depth", "Bit depth"), juce::String(s.bits)});
        if (s.seconds > 0.0) rows.push_back({tr("media-info.duration", "Duration"), clock(s.seconds)});
        if (s.bytes > 0 && s.seconds > 0.0)
            rows.push_back({tr("media-info.bit-rate", "Bit rate"), juce::String(s.bytes * 8.0 / s.seconds / 1.0e3, 0) + " kbit/s"});
    }
}

inline bool soundFileRows(std::vector<Row>& rows, const juce::File& f) {
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
    if (reader == nullptr) return false;
    heading(rows, tr("media-info.sound", "Sound"));
    rows.push_back({tr("media-info.format", "Format"), reader->getFormatName()});
    rows.push_back({tr("media-info.sample-rate", "Sample rate"), juce::String(reader->sampleRate, 0) + " Hz"});
    rows.push_back({tr("media-info.channels", "Channels"), juce::String((int) reader->numChannels)});
    if (reader->bitsPerSample > 0)
        rows.push_back({tr("media-info.bit-depth", "Bit depth"),
                        juce::String((int) reader->bitsPerSample) + (reader->usesFloatingPointData ? " float" : "")});
    rows.push_back({tr("media-info.samples", "Samples"), juce::String(reader->lengthInSamples)});
    if (reader->sampleRate > 0.0) {
        const double s = (double) reader->lengthInSamples / reader->sampleRate;
        rows.push_back({tr("media-info.duration", "Duration"), clock(s) + "  (" + seconds(s) + ")"});
    }
    int shown = 0;
    for (const auto& key : reader->metadataValues.getAllKeys()) {
        const auto value = reader->metadataValues[key].trim();
        if (value.isEmpty() || shown >= 12) continue;
        if (shown++ == 0) heading(rows, tr("media-info.tags", "Tags"));
        rows.push_back({key, value});
    }
    return true;
}

inline void fileRows(std::vector<Row>& rows, const juce::File& f) {
    heading(rows, tr("media-info.file", "File"));
    rows.push_back({tr("media-info.path", "Path"), f.getFullPathName()});
    if (!f.existsAsFile()) {
        rows.push_back({tr("media-info.status", "Status"), tr("media-info.missing", "missing")});
        return;
    }
    rows.push_back({tr("media-info.file-size", "Size"), juce::File::descriptionOfSizeInBytes(f.getSize())
                                                          + "  (" + juce::String(f.getSize()) + " bytes)"});
    rows.push_back({tr("media-info.modified", "Modified"), f.getLastModificationTime().toString(true, true, false, true)});
}

inline juce::String warpName(int mode) {
    switch (mode) {
        case 1: return tr("media-info.warp-beats", "Beats");
        case 2: return tr("media-info.warp-tone", "Tone");
        default: return tr("media-info.warp-off", "Off");
    }
}

inline void clipRows(std::vector<Row>& rows, EngineHost& host, const std::string& node,
                     const ClipEditor::ClipInfo& ci) {
    const double tpb = Pattern::kTicksPerBeat;
    const double bpm = host.tempo() > 0.0 ? host.tempo() : 120.0;
    const double spb = kSecondsPerMinute / bpm;
    const int num = std::max(1, host.automation().timeSigNumerator());
    const double startBeat = ci.startTick / tpb, lenBeats = ci.lengthTicks / tpb;
    heading(rows, tr("media-info.clip", "Clip"));
    if (!ci.name.empty()) rows.push_back({tr("media-info.name", "Name"), juce::String(ci.name)});
    rows.push_back({tr("media-info.track", "Track"), juce::String(node)});
    rows.push_back({tr("media-info.kind", "Kind"), ci.isVideo ? tr("media-info.video-clip", "video")
                                                            : tr("media-info.audio-clip", "audio")});
    rows.push_back({tr("media-info.starts", "Starts"),
                    tr("media-info.bar", "bar") + " " + juce::String((int) std::floor(startBeat / num) + 1)
                        + "." + juce::String((int) std::floor(std::fmod(startBeat, (double) num)) + 1)
                        + "  (" + tr("media-info.beat", "beat") + " " + beats(startBeat) + ", " + clock(startBeat * spb) + ")"});
    rows.push_back({tr("media-info.length", "Length"),
                    beats(lenBeats) + " " + tr("media-info.beats", "beats") + "  (" + clock(lenBeats * spb)
                        + " " + tr("media-info.at", "at") + " " + juce::String(bpm, 1) + " bpm)"});
    rows.push_back({tr("media-info.loop", "Loop"), ci.looped ? tr("media-info.on", "on") : tr("media-info.off", "off")});
    const auto range = host.clips().rangeOf(node, ci.id);
    rows.push_back({tr("media-info.in-point", "In point"), clock(range.inSeconds) + "  (" + seconds(range.inSeconds) + ")"});
    rows.push_back({tr("media-info.out-point", "Out point"), clock(range.outSeconds) + "  (" + seconds(range.outSeconds) + ")"});
    const double rate = ci.warpMode != 0 && ci.sourceBpm > 0.0 ? bpm / ci.sourceBpm : 1.0;
    rows.push_back({tr("media-info.warp", "Warp"),
                    warpName(ci.warpMode)
                        + (ci.sourceBpm > 0.0 ? ", " + tr("media-info.source-tempo", "source tempo") + " "
                                                    + juce::String(ci.sourceBpm, 2) + " bpm" : juce::String())
                        + (std::abs(rate - 1.0) > 1e-9 ? ", " + tr("media-info.plays-at", "plays at") + " x"
                                                             + juce::String(rate, 3) : juce::String())});
    if (ci.fadeInTicks > 0)
        rows.push_back({tr("media-info.fade-in", "Fade in"), beats(ci.fadeInTicks / tpb) + " " + tr("media-info.beats", "beats")});
    if (ci.fadeOutTicks > 0)
        rows.push_back({tr("media-info.fade-out", "Fade out"), beats(ci.fadeOutTicks / tpb) + " " + tr("media-info.beats", "beats")});
    const auto tape = host.clips().tape(ci.audioFile);
    if (tape.seconds > 0.0) {
        const double used = std::min(tape.seconds, range.outSeconds) - std::min(tape.seconds, range.inSeconds);
        rows.push_back({tr("media-info.uses", "Uses"),
                        clock(std::max(0.0, used)) + " " + tr("media-info.of-a", "of a") + " " + clock(tape.seconds)
                            + " " + tr("media-info.tape", "tape")
                            + (range.outSeconds > tape.seconds + 1e-6
                                   ? ", " + tr("media-info.runs-past-the-end", "runs past the end") : juce::String())});
    }
}

inline Report build(EngineHost& host, const std::string& node, const ClipEditor::ClipInfo& ci) {
    Report r;
    r.file = juce::File(juce::String(juce::CharPointer_UTF8(ci.audioFile.c_str())));
    r.title = ci.name.empty() ? r.file.getFileName() : juce::String(ci.name);
    clipRows(r.rows, host, node, ci);
    fileRows(r.rows, r.file);
    if (!r.file.existsAsFile()) return r;
    if (const auto movie = media::probeMovie(r.file); movie.ok) {
        movieRows(r.rows, movie);
    } else if (!soundFileRows(r.rows, r.file) && isVideoFile(r.file)) {
        heading(r.rows, tr("media-info.picture", "Picture"));
        if (const double s = probeVideoSeconds(r.file); s > 0.0)
            r.rows.push_back({tr("media-info.duration", "Duration"), clock(s)});
        r.rows.push_back({tr("media-info.codec", "Codec"),
                          tr("media-info.not-readable-here", "not readable from this container")});
    }
    return r;
}

inline juce::String text(const Report& r) {
    int width = 0;
    for (const auto& row : r.rows)
        if (!row.second.isEmpty()) width = std::max(width, row.first.length());
    juce::String out;
    for (const auto& row : r.rows) {
        if (row.first.isEmpty()) { out << "\n"; continue; }
        if (row.second.isEmpty()) { out << row.first << "\n"; continue; }
        out << row.first.paddedRight(' ', width) << "  " << row.second << "\n";
    }
    return out;
}

class Panel : public juce::Component {
public:
    explicit Panel(Report report) : report_(std::move(report)) {
        text_.setMultiLine(true);
        text_.setReadOnly(true);
        text_.setCaretVisible(false);
        text_.setScrollbarsShown(true);
        text_.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.5f, 0));
        text_.setColour(juce::TextEditor::backgroundColourId, Palette::panel);
        text_.setColour(juce::TextEditor::textColourId, Palette::text);
        text_.setColour(juce::TextEditor::outlineColourId, Palette::border);
        text_.setText(text(report_), false);
        addAndMakeVisible(text_);
        copy_.onClick = [this] { juce::SystemClipboard::copyTextToClipboard(text(report_)); };
        addAndMakeVisible(copy_);
        reveal_.setEnabled(report_.file.existsAsFile());
        reveal_.onClick = [this] { report_.file.revealToUser(); };
        addAndMakeVisible(reveal_);
        setSize(560, 520);
    }

    const Report& report() const { return report_; }

    void resized() override {
        auto area = getLocalBounds().reduced(12);
        auto buttons = area.removeFromBottom(28);
        copy_.setBounds(buttons.removeFromLeft(120));
        buttons.removeFromLeft(8);
        reveal_.setBounds(buttons.removeFromLeft(120));
        area.removeFromBottom(10);
        text_.setBounds(area);
    }

private:
    Report report_;
    juce::TextEditor text_;
    juce::TextButton copy_{tr("media-info.copy", "Copy")};
    juce::TextButton reveal_{tr("media-info.show-file", "Show File")};
};

inline void show(EngineHost& host, const std::string& node, const ClipEditor::ClipInfo& ci) {
    auto report = build(host, node, ci);
    juce::DialogWindow::LaunchOptions o;
    o.dialogTitle = tr("media-info.title", "Media Info") + " - " + report.title;
    o.content.setOwned(new Panel(std::move(report)));
    o.dialogBackgroundColour = Palette::background;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    o.resizable = true;
    o.launchAsync();
}

}
