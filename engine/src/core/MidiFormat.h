#pragma once
#include <string>

#include "hum/dsp/DspMath.h"

namespace hum {

inline std::string midiNoteName(int note) {
    static const char* kNames[12] = {"C",  "C#", "D",  "D#", "E",  "F",
                                     "F#", "G",  "G#", "A",  "A#", "B"};
    if (note < 0 || note > kMidiMax) return "?";
    return std::string(kNames[note % 12]) + std::to_string(note / 12 - 1);
}

inline std::string midiCcName(int cc) {
    switch (cc) {
        case 1:   return "Mod Wheel";
        case 2:   return "Breath";
        case 7:   return "Volume";
        case 10:  return "Pan";
        case 11:  return "Expression";
        case 64:  return "Sustain";
        case 71:  return "Resonance";
        case 74:  return "Cutoff";
        case 120: return "All Sound Off";
        case 121: return "Reset Controllers";
        case 123: return "All Notes Off";
        default:  return {};
    }
}

inline std::string formatMidiEvent(const unsigned char* d, int size) {
    if (size < 1) return "(empty)";
    const int status = d[0] & 0xf0;
    const int ch = (d[0] & 0x0f) + 1;
    const int b1 = size > 1 ? d[1] : 0;
    const int b2 = size > 2 ? d[2] : 0;
    auto chan = [&] { return "ch " + std::to_string(ch) + "  "; };

    switch (d[0]) {
        case 0xf8: return "Clock";
        case 0xfa: return "Start";
        case 0xfb: return "Continue";
        case 0xfc: return "Stop";
        case 0xfe: return "Active Sensing";
        case 0xff: return "System Reset";
        case 0xf1: return "MTC Quarter Frame " + std::to_string(b1);
        case 0xf2: return "Song Position " + std::to_string(b1 | (b2 << 7));
        case 0xf3: return "Song Select " + std::to_string(b1);
        case 0xf6: return "Tune Request";
        case 0xf0: return "SysEx";
        default: break;
    }
    switch (status) {
        case 0x80:
            return chan() + "Note Off  " + midiNoteName(b1) + "  vel " + std::to_string(b2);
        case 0x90:
            if (b2 == 0)
                return chan() + "Note Off  " + midiNoteName(b1) + "  (vel 0)";
            return chan() + "Note On   " + midiNoteName(b1) + "  vel " + std::to_string(b2);
        case 0xa0:
            return chan() + "Poly AT   " + midiNoteName(b1) + "  " + std::to_string(b2);
        case 0xb0: {
            const auto name = midiCcName(b1);
            return chan() + "CC " + std::to_string(b1)
                 + (name.empty() ? std::string() : " (" + name + ")")
                 + "  = " + std::to_string(b2);
        }
        case 0xc0: return chan() + "Program " + std::to_string(b1);
        case 0xd0: return chan() + "Aftertouch " + std::to_string(b1);
        case 0xe0: {
            const int bend = (b1 | (b2 << 7)) - 8192;
            return chan() + "Pitch Bend " + (bend > 0 ? "+" : "") + std::to_string(bend);
        }
        default: break;
    }
    return "status 0x" + [&] {
        static const char* hex = "0123456789abcdef";
        std::string s;
        s += hex[(d[0] >> 4) & 0xf];
        s += hex[d[0] & 0xf];
        return s;
    }();
}

inline int midiEventChannel(const unsigned char* d, int size) {
    if (size < 1 || (d[0] & 0xf0) == 0xf0) return 0;
    return (d[0] & 0x0f) + 1;
}

}
