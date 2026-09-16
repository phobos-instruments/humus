// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstddef>

namespace hum {

class Chord {
public:
    static constexpr int kMaxTones = 8;

    enum class Id {
        Minor = 0, Major = 1, Phrygian = 2, Pentatonic = 3, Dorian = 4,
        HarmonicMinor = 5, WholeTone = 6, Hirajoshi = 7, HarmonicJI = 8,
        Count = 9,
    };

    Chord() = default;

    static Chord byId(Id id) {
        switch (id) {
            case Id::Major:         return Chord({0.0, 400.0, 700.0, 1100.0}, 4);
            case Id::Phrygian:      return Chord({0.0, 100.0, 700.0, 1000.0}, 4);
            case Id::Pentatonic:    return Chord({0.0, 300.0, 500.0, 1000.0}, 4);
            case Id::Dorian:        return Chord({0.0, 300.0, 700.0, 900.0}, 4);
            case Id::HarmonicMinor: return Chord({0.0, 300.0, 700.0, 1100.0}, 4);
            case Id::WholeTone:     return Chord({0.0, 400.0, 600.0, 1000.0}, 4);
            case Id::Hirajoshi:     return Chord({0.0, 300.0, 700.0, 800.0}, 4);
            case Id::HarmonicJI:    return Chord({0.0, 386.3, 702.0, 968.8}, 4);
            case Id::Minor:
            default:                return Chord({0.0, 300.0, 700.0, 1000.0}, 4);
        }
    }
    static Chord byId(int id) {
        return byId(id >= 0 && id < (int) Id::Count ? (Id) id : Id::Minor);
    }

    static const char* nameOf(Id id) {
        static const char* kNames[(std::size_t) Id::Count] = {
            "Minor", "Major", "Phrygian", "Pentatonic", "Dorian",
            "Harmonic Minor", "Whole Tone", "Hirajoshi", "Harmonic (JI)",
        };
        return kNames[(std::size_t) id];
    }

    int size() const { return n_; }
    double cents(int i) const {
        return n_ < 1 ? 0.0 : cents_[(std::size_t) (((i % n_) + n_) % n_)];
    }

private:
    Chord(std::array<double, kMaxTones> cents, int n) : cents_(cents), n_(n) {}
    std::array<double, kMaxTones> cents_{};
    int n_ = 0;
};

}
