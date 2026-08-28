#include "hum/Registry.h"

#include <cctype>

#include "AuxIO/AuxIO.h"
#include "Bus/Bus.h"
#include "Crossfader/Crossfader.h"
#include "FileRecorder/FileRecorder.h"
#include "Send/Send.h"
#include "Gain/Gain.h"
#include "Invert/Invert.h"
#include "MidiBus/MidiBus.h"
#include "MidiIO/MidiIO.h"
#include "Mixer/Mixer.h"
#include "SoundIn/SoundIn.h"
#include "SoundOut/SoundOut.h"

namespace hum {

namespace {
OrganismPtr makeFromPattern(const std::string& cls) {
    if (cls.size() < 3) return nullptr;
    if (cls.rfind("Midi", 0) == 0 && cls.size() > 7
        && cls.compare(cls.size() - 3, 3, "Bus") == 0) {
        size_t num = 0;
        size_t i = 4;
        for (; i < cls.size() - 3 && std::isdigit((unsigned char) cls[i]); ++i)
            num = num * 10 + (size_t) (cls[i] - '0');
        if (i == cls.size() - 3 && num >= 1 && num <= 64)
            return std::make_unique<MidiBus>((int) num);
        return nullptr;
    }
    char prefix = cls.front();
    if (prefix != 'S' && prefix != 'M' && prefix != 'P') return nullptr;

    size_t i = 1, num = 0;
    bool hasDigit = false;
    for (; i < cls.size() && std::isdigit((unsigned char) cls[i]); ++i) {
        num = num * 10 + (size_t) (cls[i] - '0');
        hasDigit = true;
    }
    if (!hasDigit || num < 1 || num > 64) return nullptr;
    std::string suffix = cls.substr(i);

    int width = (prefix == 'M') ? 1 : 2;
    if (suffix == "Mixer") return std::make_unique<Mixer>((int) num, width);
    if (suffix == "Bus" && prefix != 'P') return std::make_unique<Bus>((int) num, width);
    return nullptr;
}
}

void hum_register_layouts_core(Registry&);

void hum_register_pack_core(Registry& r) {
    hum_register_layouts_core(r);
    r.registerClass("SoundOut", [] { return std::make_unique<SoundOut>(); });
    r.registerClass("SoundIn",  [] { return std::make_unique<SoundIn>(); });
    r.registerClass("SGain",    [] { return std::make_unique<Gain>(2); });
    r.registerClass("MGain",    [] { return std::make_unique<Gain>(1); });
    r.registerClass("Invert",   [] { return std::make_unique<Invert>(); });

    r.registerClass("Mixer", [] { return std::make_unique<Mixer>(4, 2); });
    r.registerClass("Bus",   [] { return std::make_unique<Bus>(2, 2); });
    r.registerClass("Gain",  [] { return std::make_unique<Gain>(2); });
    r.registerClass("Send",  [] { return std::make_unique<Send>(); });

    r.registerClass("Crossfader", [] { return std::make_unique<Crossfader>(); });

    r.registerClass("FileRecorder", [] { return std::make_unique<FileRecorder>(2); });
    for (int n = 1; n <= 32; ++n)
        r.registerClass(std::to_string(n) + "FileRecorder",
                        [n] { return std::make_unique<FileRecorder>(n); });

    for (int n : {2, 3, 4, 5, 6, 7, 8}) {
        r.registerClass("S" + std::to_string(n) + "Mixer", [n] { return std::make_unique<Mixer>(n, 2); });
        r.registerClass("M" + std::to_string(n) + "Mixer", [n] { return std::make_unique<Mixer>(n, 1); });
        r.registerClass("P" + std::to_string(n) + "Mixer",
                        [n] { return std::make_unique<Mixer>(n, 1, true); });
    }
    for (int n : {2, 3, 4, 5, 6, 7, 8}) {
        r.registerClass("S" + std::to_string(n) + "Bus", [n] { return std::make_unique<Bus>(n, 2); });
        r.registerClass("M" + std::to_string(n) + "Bus", [n] { return std::make_unique<Bus>(n, 1); });
    }
    r.registerClass("MidiBus", [] { return std::make_unique<MidiBus>(4); });
    for (int n : {2, 3, 4, 6, 8})
        r.registerClass("Midi" + std::to_string(n) + "Bus",
                        [n] { return std::make_unique<MidiBus>(n); });
    r.registerPatternFallback("core", &makeFromPattern);

    r.registerClass("AudioIn",  [] { return std::make_unique<AuxIn>(0); });
    r.registerClass("AudioOut", [] { return std::make_unique<AuxOut>(0); });
    for (int n = 1; n <= 8; ++n) {
        r.registerClass("AuxIn" + std::to_string(n),
                        [n] { return std::make_unique<AuxIn>(n + 1); });
        r.registerClass("AuxOut" + std::to_string(n),
                        [n] { return std::make_unique<AuxOut>(n + 1); });
    }

    r.registerClass("MidiIn",  [] { return std::make_unique<MidiInNode>(0); });
    r.registerClass("MidiOut", [] { return std::make_unique<MidiOutNode>(0); });
    for (int n = 1; n <= 8; ++n) {
        r.registerClass("MidiIn" + std::to_string(n),
                        [n] { return std::make_unique<MidiInNode>(n - 1); });
        r.registerClass("MidiOut" + std::to_string(n),
                        [n] { return std::make_unique<MidiOutNode>(n - 1); });
    }
}

}
