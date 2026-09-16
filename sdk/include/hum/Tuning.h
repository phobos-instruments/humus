// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "hum/Number.h"

#include "hum/dsp/DspMath.h"

namespace hum {

class Tuning {
public:
    static constexpr int kMaxDegrees = 128;

    Tuning() {
        for (int k = 1; k <= 12; ++k) degrees_[(std::size_t) (k - 1)] = 100.0 * (double) k;
    }

    static Tuning equalDivisions(int divisions, double periodCents = 1200.0,
                                 double rootNote = 69.0, double rootHz = kA4Hz) {
        Tuning t;
        t.equal_ = true;
        t.n_ = divisions < 1 ? 1 : (divisions > kMaxDegrees ? kMaxDegrees : divisions);
        t.periodCents_ = periodCents;
        t.rootNote_ = rootNote;
        t.rootHz_ = rootHz > 0.0 ? rootHz : kA4Hz;
        for (int k = 1; k <= t.n_; ++k)
            t.degrees_[(std::size_t) (k - 1)] = t.periodCents_ * (double) k / (double) t.n_;
        return t;
    }

    static Tuning fromCents(const double* degreeCents, int n, double rootNote = 69.0,
                            double rootHz = kA4Hz) {
        if (degreeCents == nullptr || n < 1 || n > kMaxDegrees
            || degreeCents[(std::size_t) (n - 1)] <= 0.0)
            return equalDivisions(12, 1200.0, rootNote, rootHz);
        for (int i = 1; i < n; ++i)
            if (degreeCents[(std::size_t) i] <= degreeCents[(std::size_t) (i - 1)])
                return equalDivisions(12, 1200.0, rootNote, rootHz);
        Tuning t;
        t.equal_ = false;
        t.n_ = n;
        for (int i = 0; i < n; ++i) t.degrees_[(std::size_t) i] = degreeCents[(std::size_t) i];
        t.periodCents_ = degreeCents[(std::size_t) (n - 1)];
        t.rootNote_ = rootNote;
        t.rootHz_ = rootHz > 0.0 ? rootHz : kA4Hz;
        return t;
    }
    static Tuning fromCents(const std::vector<double>& degreeCents, double rootNote = 69.0,
                            double rootHz = kA4Hz) {
        return fromCents(degreeCents.data(), (int) degreeCents.size(), rootNote, rootHz);
    }

    static Tuning fromSclText(const std::string& text, double rootNote = 69.0,
                              double rootHz = kA4Hz) {
        const Tuning fallback = equalDivisions(12, 1200.0, rootNote, rootHz);
        double cents[kMaxDegrees];
        int want = -1, got = 0, realLine = 0;
        std::size_t pos = 0;
        while (pos <= text.size()) {
            std::size_t eol = text.find('\n', pos);
            if (eol == std::string::npos) eol = text.size();
            std::size_t a = pos, b = eol;
            pos = eol + 1;
            while (a < b && (text[a] == ' ' || text[a] == '\t' || text[a] == '\r')) ++a;
            while (b > a && (text[b - 1] == ' ' || text[b - 1] == '\t' || text[b - 1] == '\r')) --b;
            if (a < b && text[a] == '!') continue;
            ++realLine;
            if (realLine == 1) continue;
            const char* s = text.c_str() + a;
            const char* end = nullptr;
            if (realLine == 2) {
                char* numEnd = nullptr;
                const long nl = std::strtol(s, &numEnd, 10);
                if (numEnd == s || nl < 1 || nl > kMaxDegrees) return fallback;
                want = (int) nl;
                continue;
            }
            if (got >= want) break;
            double v;
            const double p = scanDouble(s, &end);
            if (end == s) return fallback;
            bool isCents = false;
            for (const char* c = s; c != end; ++c)
                if (*c == '.') { isCents = true; break; }
            if (isCents) {
                v = p;
            } else if (end < text.c_str() + b && *end == '/') {
                const char* qs = end + 1;
                const double q = scanDouble(qs, &end);
                if (end == qs || p <= 0.0 || q <= 0.0) return fallback;
                v = 1200.0 * std::log2(p / q);
            } else {
                if (p <= 0.0) return fallback;
                v = 1200.0 * std::log2(p);
            }
            if (v <= 1e-9 && got == 0) { --want; continue; }
            cents[got++] = v;
        }
        if (want < 1 || got != want) return fallback;
        return fromCents(cents, got, rootNote, rootHz);
    }

    void setKeyboardMap(bool on) {
        keyMapped_ = on;
        if (!on) return;
        for (int k = 0; k < 12; ++k) {
            const double target = (double) k * periodCents_ / 12.0;
            int d = (int) std::lround(degreeAt(target));
            keyMap_[(std::size_t) k] = (std::int8_t) (d < 0 ? 0 : (d >= n_ ? n_ - 1 : d));
        }
    }
    bool keyboardMapped() const { return keyMapped_; }

    int notesPerPeriod() const { return keyMapped_ ? 12 : n_; }

    double centsAtNote(int noteOffset) const {
        if (!keyMapped_) return centsAt((double) noteOffset);
        const int period = noteOffset >= 0 ? noteOffset / 12 : (noteOffset - 11) / 12;
        const int k = noteOffset - period * 12;
        return (double) period * periodCents_ + degreeCentsAt(keyMap_[(std::size_t) k]);
    }

    double hz(double midiNote) const {
        const double x = midiNote - rootNote_;
        if (keyMapped_) {
            const int lo = (int) std::floor(x);
            const double frac = x - (double) lo;
            const double a = centsAtNote(lo);
            const double b = frac > 0.0 ? centsAtNote(lo + 1) : a;
            return rootHz_ * std::pow(2.0, (a + (b - a) * frac) / 1200.0);
        }
        if (equal_) {
            if (periodCents_ == 1200.0) return rootHz_ * std::pow(2.0, x / (double) n_);
            return rootHz_ * std::pow(2.0, x * periodCents_ / (1200.0 * (double) n_));
        }
        return rootHz_ * std::pow(2.0, centsAt(x) / 1200.0);
    }

    double midiNote(double hz) const {
        if (hz <= 0.0 || rootHz_ <= 0.0) return rootNote_;
        const double cents = 1200.0 * std::log2(hz / rootHz_);
        if (keyMapped_) {
            const double period = std::floor(cents / periodCents_);
            const double c = cents - period * periodCents_;
            for (int k = 0; k < 12; ++k) {
                const double a = degreeCentsAt(keyMap_[(std::size_t) k]);
                const double b = k == 11 ? periodCents_ : degreeCentsAt(keyMap_[(std::size_t) (k + 1)]);
                if (c < b || k == 11)
                    return rootNote_ + period * 12.0 + (double) k
                           + (b > a ? (c - a) / (b - a) : 0.0);
            }
        }
        if (equal_) return rootNote_ + cents * (double) n_ / periodCents_;
        return rootNote_ + degreeAt(cents);
    }

    int degreesPerPeriod() const { return n_; }
    double periodCents() const { return periodCents_; }
    double rootNote() const { return rootNote_; }
    double rootHz() const { return rootHz_; }

    double centsAtDegree(int k) const { return degreeCentsAt(k); }

    double degreeForCents(double cents) const { return degreeAt(cents); }

    enum PluginDelivery { kDeliverMts = 0, kDeliverBend = 1, kDeliverAuto = 2 };
    int pluginDelivery() const { return (int) delivery_; }
    void setPluginDelivery(int d) {
        delivery_ = (std::uint8_t) (d < 0 ? 0 : d > 2 ? 2 : d);
    }

    bool isStandard() const {
        return equal_ && !keyMapped_ && n_ == 12 && periodCents_ == 1200.0
               && rootNote_ == 69.0 && rootHz_ == kA4Hz;
    }

    bool operator==(const Tuning& o) const {
        if (equal_ != o.equal_ || keyMapped_ != o.keyMapped_
            || delivery_ != o.delivery_ || n_ != o.n_
            || periodCents_ != o.periodCents_
            || rootNote_ != o.rootNote_ || rootHz_ != o.rootHz_)
            return false;
        for (int i = 0; i < n_; ++i)
            if (degrees_[(std::size_t) i] != o.degrees_[(std::size_t) i]) return false;
        return true;
    }
    bool operator!=(const Tuning& o) const { return !(*this == o); }

private:
    double centsAt(double x) const {
        const double nd = (double) n_;
        const double period = std::floor(x / nd);
        const double i = x - period * nd;
        const int lo = (int) std::floor(i);
        const double frac = i - (double) lo;
        const double a = degreeCentsAt(lo);
        const double b = degreeCentsAt(lo + 1);
        return period * periodCents_ + a + (b - a) * frac;
    }

    double degreeAt(double cents) const {
        const double period = std::floor(cents / periodCents_);
        const double c = cents - period * periodCents_;
        for (int k = 0; k < n_; ++k) {
            const double a = degreeCentsAt(k), b = degreeCentsAt(k + 1);
            if (c < b || k == n_ - 1)
                return period * (double) n_ + (double) k + (b > a ? (c - a) / (b - a) : 0.0);
        }
        return period * (double) n_;
    }

    double degreeCentsAt(int k) const {
        return k <= 0 ? 0.0 : degrees_[(std::size_t) (k - 1 < n_ ? k - 1 : n_ - 1)];
    }

    bool equal_ = true;
    bool keyMapped_ = false;
    std::uint8_t delivery_ = 0;
    int n_ = 12;
    double periodCents_ = 1200.0;
    double rootNote_ = 69.0;
    double rootHz_ = kA4Hz;
    std::array<double, kMaxDegrees> degrees_{};
    std::array<std::int8_t, 12> keyMap_{};
};

}
