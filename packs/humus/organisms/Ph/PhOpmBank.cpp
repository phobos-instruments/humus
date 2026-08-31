#include "Ph/PhOpmBank.h"

namespace hum::phbank {

namespace {

using Op = PhOpm::Op;
using Patch = PhOpm::Patch;

constexpr Op op(uint8_t mult, uint8_t tl, uint8_t ar, uint8_t d1r, uint8_t d1l,
                uint8_t rr, uint8_t dt2 = 0, uint8_t d2r = 0) {
    return Op{mult, 3, dt2, tl, 1, ar, d1r, d2r, rr, d1l, 0};
}

constexpr uint8_t kChain = 0, kPair = 4, kAll = 7;

Patch make(uint8_t alg, uint8_t fb, Op a, Op b, Op c, Op d,
           uint8_t pms = 0, uint8_t lfrq = 0, uint8_t pmd = 0) {
    Patch p;
    p.alg = alg;
    p.fb = fb;
    p.ops = {a, b, c, d};
    p.pms = pms;
    p.lfrq = lfrq;
    p.pmd = pmd;
    return p;
}

}

const std::vector<PhOpm::Instrument>& opmFactory() {
    static const std::vector<PhOpm::Instrument> bank = {
        {"Arcade Bass", make(kPair, 5, op(1, 28, 31, 14, 4, 9), op(1, 4, 31, 10, 2, 8),
                                       op(2, 44, 31, 16, 5, 9), op(1, 12, 31, 12, 2, 8))},
        {"Slap Bass",   make(kChain, 6, op(1, 30, 31, 18, 6, 10), op(3, 38, 31, 15, 5, 9),
                                        op(1, 42, 31, 13, 4, 9), op(1, 5, 31, 11, 2, 8))},
        {"Metal Bell",  make(kPair, 3, op(3, 32, 31, 8, 2, 4, 1), op(1, 6, 31, 6, 1, 3),
                                       op(7, 40, 31, 9, 2, 4, 2), op(2, 14, 31, 7, 1, 3))},
        {"Ice Bell",    make(kPair, 2, op(9, 36, 31, 7, 2, 3, 3), op(2, 10, 31, 5, 1, 2),
                                       op(5, 42, 31, 8, 2, 3, 1), op(1, 16, 31, 6, 1, 2))},
        {"Steel Pluck", make(kPair, 4, op(4, 34, 31, 15, 5, 8, 1), op(1, 8, 31, 12, 3, 7),
                                       op(2, 46, 31, 17, 6, 8), op(1, 18, 31, 13, 3, 7))},
        {"Brass Hit",   make(kAll, 6, op(1, 20, 22, 8, 1, 7), op(1, 16, 20, 8, 1, 7),
                                      op(1, 22, 18, 9, 2, 7), op(1, 12, 21, 8, 1, 7))},
        {"Saw Lead",    make(kAll, 7, op(1, 14, 28, 5, 1, 8), op(2, 24, 28, 5, 1, 8),
                                      op(1, 18, 28, 5, 1, 8), op(1, 10, 28, 5, 1, 8),
                             4, 200, 24)},
        {"Glass Pad",   make(kPair, 1, op(2, 40, 12, 4, 1, 5, 1), op(1, 12, 10, 4, 0, 5),
                                       op(3, 48, 11, 5, 1, 5), op(1, 18, 10, 4, 0, 5),
                             3, 180, 16)},
        {"Organ Tone",  make(kAll, 0, op(1, 12, 31, 2, 0, 9), op(2, 18, 31, 2, 0, 9),
                                      op(4, 26, 31, 2, 0, 9), op(8, 34, 31, 2, 0, 9))},
        {"Wood Knock",  make(kPair, 0, op(5, 40, 31, 20, 8, 13), op(1, 10, 31, 18, 6, 12),
                                       op(9, 52, 31, 22, 9, 13, 2), op(2, 26, 31, 19, 7, 12))},
        {"Snow Chime",  make(kPair, 1, op(11, 42, 31, 9, 3, 4, 3), op(3, 18, 31, 7, 2, 3, 1),
                                       op(13, 48, 31, 10, 3, 4, 2), op(4, 24, 31, 8, 2, 3))},
        {"Wire Key",    make(kChain, 5, op(6, 38, 31, 12, 4, 7, 1), op(2, 34, 31, 11, 3, 7),
                                        op(1, 30, 31, 10, 3, 7), op(1, 8, 31, 9, 2, 6))},
    };
    return bank;
}

}
