#pragma once
#include <array>
#include <cmath>
#include <cstddef>

#include "hum/Tuning.h"

namespace hum {

class Scale {
public:
    static constexpr int kMaxTones = 16;

    enum class Id {
        Chromatic = 0, Major = 1, Minor = 2, PentatonicMinor = 3,
        Dorian = 4, Phrygian = 5, Lydian = 6, Mixolydian = 7, Locrian = 8,
        HarmonicMinor = 9, MelodicMinor = 10, PentatonicMajor = 11,
        Blues = 12, WholeTone = 13, Hirajoshi = 14, Count = 15,
    };

    Scale() = default;

    static Scale byId(Id id) {
        switch (id) {
            case Id::Major:           return Scale({0, 200, 400, 500, 700, 900, 1100}, 7);
            case Id::Minor:           return Scale({0, 200, 300, 500, 700, 800, 1000}, 7);
            case Id::PentatonicMinor: return Scale({0, 300, 500, 700, 1000}, 5);
            case Id::Dorian:          return Scale({0, 200, 300, 500, 700, 900, 1000}, 7);
            case Id::Phrygian:        return Scale({0, 100, 300, 500, 700, 800, 1000}, 7);
            case Id::Lydian:          return Scale({0, 200, 400, 600, 700, 900, 1100}, 7);
            case Id::Mixolydian:      return Scale({0, 200, 400, 500, 700, 900, 1000}, 7);
            case Id::Locrian:         return Scale({0, 100, 300, 500, 600, 800, 1000}, 7);
            case Id::HarmonicMinor:   return Scale({0, 200, 300, 500, 700, 800, 1100}, 7);
            case Id::MelodicMinor:    return Scale({0, 200, 300, 500, 700, 900, 1100}, 7);
            case Id::PentatonicMajor: return Scale({0, 200, 400, 700, 900}, 5);
            case Id::Blues:           return Scale({0, 300, 500, 600, 700, 1000}, 6);
            case Id::WholeTone:       return Scale({0, 200, 400, 600, 800, 1000}, 6);
            case Id::Hirajoshi:       return Scale({0, 200, 300, 700, 800}, 5);
            case Id::Chromatic:
            default:                  return Scale();
        }
    }
    static Scale byId(int id) {
        return byId(id > 0 && id < (int) Id::Count ? (Id) id : Id::Chromatic);
    }

    static const char* nameOf(Id id) {
        static const char* kNames[(std::size_t) Id::Count] = {
            "Chromatic", "Major", "Minor", "Pentatonic Minor", "Dorian", "Phrygian",
            "Lydian", "Mixolydian", "Locrian", "Harmonic Minor", "Melodic Minor",
            "Pentatonic Major", "Blues", "Whole Tone", "Hirajoshi",
        };
        return kNames[(std::size_t) id];
    }

    static int nearestDegree(const Tuning& t, double cents) {
        const int n = t.degreesPerPeriod();
        if (n < 1) return 0;
        const int d = (int) std::lround(t.degreeForCents(cents));
        return ((d % n) + n) % n;
    }

    static int nearestNote(const Tuning& t, double cents) {
        if (!t.keyboardMapped()) return nearestDegree(t, cents);
        const double p = t.periodCents() > 0.0 ? t.periodCents() : 1200.0;
        const double c = cents - std::floor(cents / p) * p;
        int best = 0;
        double bestDist = 1.0e18;
        for (int k = 0; k < 12; ++k) {
            const double d = std::abs(t.centsAtNote(k) - c);
            if (d < bestDist) { bestDist = d; best = k; }
        }
        return best;
    }

    bool chromatic() const { return n_ == 0; }
    int size() const { return n_; }
    double cents(int i) const { return cents_[(std::size_t) i]; }

    bool allows(const Tuning& t, int degree, int keyDegree = 0) const {
        if (n_ == 0) return true;
        const int n = t.notesPerPeriod();
        if (n < 1) return true;
        const int d = (((degree - keyDegree) % n) + n) % n;
        for (int i = 0; i < n_; ++i)
            if (nearestNote(t, cents_[(std::size_t) i]) == d) return true;
        return false;
    }

private:
    Scale(std::array<double, kMaxTones> cents, int n) : cents_(cents), n_(n) {}
    std::array<double, kMaxTones> cents_{};
    int n_ = 0;
};

}
