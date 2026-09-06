#pragma once
#include <map>
#include <string>
#include <vector>

namespace hum {

struct LayoutSpec {
    enum class ControlType {
        Knob,
        VSlider,
        HSlider,
        RangeVSlider,
        Toggle,
        MiniToggle,
        Combo,
        IntSpinner,
        DoubleSpinner,
        RhythmicUnit,
        SoundFile,
        ScaleFile,
        BankFile,
        KnobGrid,
        FileTransport,
        Label,
        Waveform,
        FaderBank,
        Deck,
        DeckPitch,
        DeckControls,
        MidiKeyboard,
        Momentary,
        LooperTracks,
        StepGrid,
        PatternGrid,
        PianoRoll,
        SoundMap,
        Camera,
        MidiLog,
        OscLog,
        LevelBars,
        ThresholdMeter,
        PitchReadout,
        NoteField,
        GainShapeCurve,
        SequenceGrid,
        HandGestures,
        EnumButtons,
        Sigil,
        RotarySwitch,
        DnaBases,
        DnaStrand,
        NumberField,
        TextField,
        GainReduction,
        WaveDraw,
        TapTempo,
        VuMeter,
        FieldScope,
        SpectrumScope,
        VideoPreview,
        VideoTransport,
        Formula,
        SliceMap,
        PictureField,
        LfoScope,
        HelixStrands,
        ClipGrid,
        ScreenButton,
    };

    struct Control {
        ControlType type = ControlType::Knob;
        std::string param;
        std::string param2;
        std::string label;
        int x = 0, y = 0, w = 60, h = 80;
        int decimalPlaces = 2;
        bool logarithmic = false;
        std::vector<std::string> options;
        std::map<std::string, std::string> extra;

        std::string extraOr(const std::string& key, const std::string& fallback = {}) const {
            auto it = extra.find(key);
            return it == extra.end() ? fallback : it->second;
        }
    };

    enum class Resize { Scale, Stretch, Grow };

    int width = 280;
    int height = 220;
    Resize resize = Resize::Scale;
    std::vector<Control> controls;
};

constexpr int kKnobW = 60, kKnobH = 84, kKnobPitch = 64;

constexpr int kRackFullW = 600, kRackHalfW = 293;

struct ControlTypeName { const char* name; LayoutSpec::ControlType type; };

inline const std::vector<ControlTypeName>& controlTypeNames() {
    using CT = LayoutSpec::ControlType;
    static const std::vector<ControlTypeName> t = {
        {"knob", CT::Knob},                   {"vslider", CT::VSlider},
        {"hslider", CT::HSlider},             {"range-vslider", CT::RangeVSlider},
        {"toggle", CT::Toggle},               {"minitoggle", CT::MiniToggle},
        {"combo", CT::Combo},                 {"int-spinner", CT::IntSpinner},
        {"double-spinner", CT::DoubleSpinner},{"rhythmic-unit", CT::RhythmicUnit},
        {"soundfile", CT::SoundFile},         {"scale-file", CT::ScaleFile},
        {"bank-file", CT::BankFile},
        {"knob-grid", CT::KnobGrid},
        {"file-transport", CT::FileTransport},
        {"label", CT::Label},                 {"waveform", CT::Waveform},
        {"fader-bank", CT::FaderBank},        {"deck", CT::Deck},
        {"deck-pitch", CT::DeckPitch},        {"deck-controls", CT::DeckControls},
        {"midi-keyboard", CT::MidiKeyboard},  {"momentary", CT::Momentary},
        {"looper-tracks", CT::LooperTracks},  {"step-grid", CT::StepGrid},
        {"pattern-grid", CT::PatternGrid},    {"piano-roll", CT::PianoRoll},
        {"sound-map", CT::SoundMap},          {"camera", CT::Camera},
        {"midi-log", CT::MidiLog},            {"osc-log", CT::OscLog},
        {"level-meter", CT::LevelBars},       {"pitch-readout", CT::PitchReadout},
        {"note-field", CT::NoteField},        {"gain-shape", CT::GainShapeCurve},
        {"sequence-grid", CT::SequenceGrid},  {"hand-gestures", CT::HandGestures},
        {"enum-buttons", CT::EnumButtons},    {"sigil", CT::Sigil},
        {"rotary-switch", CT::RotarySwitch},  {"dna-bases", CT::DnaBases},
        {"dna-strand", CT::DnaStrand},
        {"number-field", CT::NumberField},    {"wave-draw", CT::WaveDraw},
        {"text-field", CT::TextField},
        {"gain-reduction", CT::GainReduction},
        {"tap", CT::TapTempo},                {"vu-meter", CT::VuMeter},
        {"field-scope", CT::FieldScope},
        {"spectrum-scope", CT::SpectrumScope},
        {"video-preview", CT::VideoPreview},
        {"video-transport", CT::VideoTransport},  {"formula", CT::Formula},
        {"slice-map", CT::SliceMap},        {"picture-field", CT::PictureField},          {"lfo-scope", CT::LfoScope},
        {"threshold-meter", CT::ThresholdMeter},
        {"helix-strands", CT::HelixStrands},
        {"clip-grid", CT::ClipGrid},
        {"screen-button", CT::ScreenButton},
    };
    return t;
}

inline bool controlTypeFromName(const std::string& s, LayoutSpec::ControlType& out) {
    for (const auto& e : controlTypeNames())
        if (s == e.name) { out = e.type; return true; }
    return false;
}

inline const char* controlTypeToName(LayoutSpec::ControlType t) {
    for (const auto& e : controlTypeNames())
        if (e.type == t) return e.name;
    return "knob";
}

namespace layout_detail {
inline std::string jsonString(const std::string& s) {
    std::string o = "\"";
    for (const char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if ((unsigned char) c < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    o += "\\u00";
                    o += hex[((unsigned char) c >> 4) & 0xF];
                    o += hex[(unsigned char) c & 0xF];
                } else {
                    o += c;
                }
        }
    }
    return o + "\"";
}
}

inline std::string toJson(const LayoutSpec& spec) {
    using layout_detail::jsonString;
    std::string o = "{\"width\":" + std::to_string(spec.width)
                  + ",\"height\":" + std::to_string(spec.height);
    if (spec.resize == LayoutSpec::Resize::Stretch) o += ",\"resize\":\"stretch\"";
    if (spec.resize == LayoutSpec::Resize::Grow) o += ",\"resize\":\"grow\"";
    o += ",\"controls\":[";
    bool firstControl = true;
    for (const auto& c : spec.controls) {
        if (!firstControl) o += ",";
        firstControl = false;
        o += "{\"type\":";
        o += jsonString(controlTypeToName(c.type));
        if (!c.param.empty())  o += ",\"param\":" + jsonString(c.param);
        if (!c.param2.empty()) o += ",\"param2\":" + jsonString(c.param2);
        if (!c.label.empty())  o += ",\"label\":" + jsonString(c.label);
        o += ",\"x\":" + std::to_string(c.x) + ",\"y\":" + std::to_string(c.y)
           + ",\"w\":" + std::to_string(c.w) + ",\"h\":" + std::to_string(c.h);
        if (c.decimalPlaces != 2) o += ",\"decimals\":" + std::to_string(c.decimalPlaces);
        if (c.logarithmic) o += ",\"log\":true";
        if (!c.options.empty()) {
            o += ",\"options\":[";
            for (size_t i = 0; i < c.options.size(); ++i) {
                if (i) o += ",";
                o += jsonString(c.options[i]);
            }
            o += "]";
        }
        for (const auto& kv : c.extra) o += "," + jsonString(kv.first) + ":" + jsonString(kv.second);
        o += "}";
    }
    return o + "]}";
}

}
