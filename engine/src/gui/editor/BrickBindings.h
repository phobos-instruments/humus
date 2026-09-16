// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <atomic>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gui/editor/LayoutCondition.h"
#include "hum/LayoutSpec.h"

namespace hum {

namespace bind {
inline constexpr std::string_view kBpm = "bpm";
inline constexpr std::string_view kCue = "cue";
inline constexpr std::string_view kFile = "file";
inline constexpr std::string_view kGridOffset = "grid-offset";
inline constexpr std::string_view kHotCuePrefix = "hot-cue-prefix";
inline constexpr std::string_view kHq = "hq";
inline constexpr std::string_view kKey = "key";
inline constexpr std::string_view kLoop = "loop";
inline constexpr std::string_view kLoopIn = "loop-in";
inline constexpr std::string_view kLoopOut = "loop-out";
inline constexpr std::string_view kLoopBeats = "loop-beats";
inline constexpr std::string_view kQuantize = "quantize";
inline constexpr std::string_view kPitch = "pitch";
inline constexpr std::string_view kPitchRange = "pitch-range";
inline constexpr std::string_view kGestures = "gestures";
inline constexpr std::string_view kTolerance = "tolerance";
inline constexpr std::string_view kNotePrefix = "note-prefix";
inline constexpr std::string_view kThresholdPrefix = "threshold-prefix";
inline constexpr std::string_view kFilePrefix = "file-prefix";
inline constexpr std::string_view kInPrefix = "in-prefix";
inline constexpr std::string_view kOutPrefix = "out-prefix";
inline constexpr std::string_view kLoopPrefix = "loop-prefix";
inline constexpr std::string_view kLaunchPrefix = "launch-prefix";
inline constexpr std::string_view kEdits = "edits";
inline constexpr std::string_view kMutePrefix = "mute-prefix";
inline constexpr std::string_view kSoloPrefix = "solo-prefix";
inline constexpr std::string_view kRowPrefix = "row-prefix";
inline constexpr std::string_view kChord = "chord";
inline constexpr std::string_view kMute = "mute";
inline constexpr std::string_view kEnablePrefix = "enable-prefix";
inline constexpr std::string_view kVelocityPrefix = "velocity-prefix";
inline constexpr std::string_view kActive = "active";
inline constexpr std::string_view kWaveform = "waveform";
inline constexpr std::string_view kAmplitude = "amplitude";
inline constexpr std::string_view kOffset = "offset";
inline constexpr std::string_view kNudge = "nudge";
inline constexpr std::string_view kTranspose = "transpose";
inline constexpr std::string_view kBars = "bars";
inline constexpr std::string_view kSwing = "swing";
inline constexpr std::string_view kSwingFollow = "swing-follow";
inline constexpr std::string_view kSwingUnit = "swing-unit";
inline constexpr std::string_view kX = "x-param";
inline constexpr std::string_view kY = "y-param";
inline constexpr std::string_view kHeight = "height-param";
inline constexpr std::string_view kSpray = "spray";
inline constexpr std::string_view kPosition = "position";
inline constexpr std::string_view kWarp = "warp";
inline constexpr std::string_view kWarpMode = "warp-mode";
inline constexpr std::string_view kSelect = "select";
inline constexpr std::string_view kBlur = "blur";
inline constexpr std::string_view kGate = "gate";
inline constexpr std::string_view kTilt = "tilt";
inline constexpr std::string_view kLevel = "level";
inline constexpr std::string_view kLowest = "lowest";
inline constexpr std::string_view kHighest = "highest";
inline constexpr std::string_view kWidth = "width-param";
inline constexpr std::string_view kXKnob = "x-knob";
inline constexpr std::string_view kYKnob = "y-knob";
inline constexpr std::string_view kZKnob = "z-knob";
inline constexpr std::string_view kWKnob = "w-knob";
inline constexpr std::string_view kFreq = "freq";
inline constexpr std::string_view kSource = "source";
inline constexpr std::string_view kChannel = "channel";
inline constexpr std::string_view kDirection = "direction";
inline constexpr std::string_view kChannelsPrefix = "channels-prefix";
inline constexpr std::string_view kPreview = "preview";
inline constexpr std::string_view kMasterVolume = "master-volume";
inline constexpr std::string_view kVolumePrefix = "volume-prefix";
}

struct BrickBinding {
    enum class Kind { Param, Family, Assignment };
    std::string_view key;
    Kind kind = Kind::Param;
    bool optional = false;
};

inline const std::vector<BrickBinding>& brickBindings(LayoutSpec::ControlType type) {
    using CT = LayoutSpec::ControlType;
    constexpr auto family = BrickBinding::Kind::Family;
    constexpr auto param = BrickBinding::Kind::Param;
    static const std::vector<BrickBinding> none;
    static const std::vector<BrickBinding> deck{
        {bind::kBpm}, {bind::kCue}, {bind::kFile}, {bind::kGridOffset}, {bind::kHotCuePrefix, family},
        {bind::kHq}, {bind::kKey}, {bind::kLoop}, {bind::kLoopIn}, {bind::kLoopOut}, {bind::kQuantize}};
    static const std::vector<BrickBinding> deckPitch{{bind::kPitch}, {bind::kPitchRange}};
    static const std::vector<BrickBinding> deckControls{
        {bind::kBpm}, {bind::kCue}, {bind::kGridOffset}, {bind::kHotCuePrefix, family}, {bind::kLoop},
        {bind::kLoopBeats}, {bind::kLoopIn}, {bind::kLoopOut}, {bind::kQuantize}};
    static const std::vector<BrickBinding> gestures{
        {bind::kGestures}, {bind::kTolerance}, {bind::kNotePrefix, family}, {bind::kThresholdPrefix, family}};
    static const std::vector<BrickBinding> clipGrid{
        {bind::kFilePrefix, family}, {bind::kInPrefix, family}, {bind::kOutPrefix, family},
        {bind::kLoopPrefix, family}, {bind::kLaunchPrefix, family}};
    static const std::vector<BrickBinding> sliceMap{{bind::kEdits}};
    static const std::vector<BrickBinding> strands{
        {bind::kFilePrefix, family}, {bind::kMutePrefix, family}, {bind::kSoloPrefix, family}};
    static const std::vector<BrickBinding> intervalRows{{bind::kRowPrefix, family}, {bind::kChord}};
    static const std::vector<BrickBinding> stepStrip{{bind::kMute}};
    static const std::vector<BrickBinding> sequenceGrid{
        {bind::kEnablePrefix, family}, {bind::kNotePrefix, family}, {bind::kVelocityPrefix, family}};
    static const std::vector<BrickBinding> fileTransport{{bind::kActive}, {bind::kLoop}};
    static const std::vector<BrickBinding> lfoScope{{bind::kWaveform}, {bind::kAmplitude}, {bind::kOffset}};
    static const std::vector<BrickBinding> stepGrid{{bind::kNudge, param, true}, {bind::kTranspose, param, true}};
    static const std::vector<BrickBinding> pianoRoll{
        {bind::kBars}, {bind::kSwing, param, true}, {bind::kSwingFollow, param, true},
        {bind::kSwingUnit, param, true}};
    static const std::vector<BrickBinding> pictureField{
        {bind::kBlur}, {bind::kGate}, {bind::kTilt}, {bind::kLevel}, {bind::kLowest}, {bind::kHighest},
        {bind::kX}, {bind::kY}, {bind::kWidth}, {bind::kHeight}};
    static const std::vector<BrickBinding> formula{
        {bind::kXKnob}, {bind::kYKnob}, {bind::kZKnob}, {bind::kWKnob}, {bind::kFreq}};
    static const std::vector<BrickBinding> midiLog{{bind::kSource}, {bind::kChannel}};
    static const std::vector<BrickBinding> oscLog{{bind::kDirection}};
    static const std::vector<BrickBinding> takeLane{{bind::kChannelsPrefix, family}};
    static const std::vector<BrickBinding> videoPreview{{bind::kPreview, param, true}};
    static const std::vector<BrickBinding> patternGrid{
        {bind::kMasterVolume}, {bind::kMute}, {bind::kVolumePrefix, family}, {bind::kEnablePrefix, family},
        {bind::kFilePrefix, family}, {bind::kGate, param, true}};
    static const std::vector<BrickBinding> soundMap{{bind::kSpray}, {bind::kFilePrefix, family}};
    static const std::vector<BrickBinding> videoTransport{{bind::kFile}};
    static const std::vector<BrickBinding> waveDraw{{bind::kPosition}, {bind::kWarp}, {bind::kWarpMode}};
    static const std::vector<BrickBinding> scaleFile{{bind::kSelect, BrickBinding::Kind::Assignment}};
    switch (type) {
        case CT::Deck: return deck;
        case CT::DeckPitch: return deckPitch;
        case CT::DeckControls: return deckControls;
        case CT::HandGestures: return gestures;
        case CT::ClipGrid: return clipGrid;
        case CT::SliceMap: return sliceMap;
        case CT::Strands: return strands;
        case CT::IntervalRows: return intervalRows;
        case CT::StepStrip: return stepStrip;
        case CT::SequenceGrid: return sequenceGrid;
        case CT::FileTransport: return fileTransport;
        case CT::LfoScope: return lfoScope;
        case CT::StepGrid: return stepGrid;
        case CT::PianoRoll: return pianoRoll;
        case CT::PictureField: return pictureField;
        case CT::SoundMap: return soundMap;
        case CT::VideoTransport: return videoTransport;
        case CT::WaveDraw: return waveDraw;
        case CT::ScaleFile: return scaleFile;
        case CT::Formula: return formula;
        case CT::MidiLog: return midiLog;
        case CT::OscLog: return oscLog;
        case CT::TakeLane: return takeLane;
        case CT::VideoPreview: return videoPreview;
        case CT::PatternGrid: return patternGrid;
        default: return none;
    }
}

inline std::string boundParam(const LayoutSpec& spec, const LayoutSpec::Control& s, std::string_view key) {
    const std::string k(key);
    if (const auto own = s.extraOr(k); !own.empty()) return own;
    const auto it = spec.bind.find(k);
    return it != spec.bind.end() ? it->second : std::string();
}

class Bindings {
public:
    Bindings(const LayoutSpec& spec, const LayoutSpec::Control& control) : spec_(spec), control_(control) {}

    std::string operator()(std::string_view key) const {
        for (const auto& declared : brickBindings(control_.type))
            if (declared.key == key) return boundParam(spec_, control_, key);
        undeclaredReads().fetch_add(1);
        return {};
    }

    std::pair<std::string, double> assignment(std::string_view key) const {
        for (const auto& clause : LayoutCondition::parse((*this)(key), spec_).clauses())
            if (clause.equals && !std::isnan(clause.value)) return {clause.name, clause.value};
        return {};
    }

    static std::atomic<int>& undeclaredReads() {
        static std::atomic<int> reads{0};
        return reads;
    }

private:
    const LayoutSpec& spec_;
    const LayoutSpec::Control& control_;
};

}
