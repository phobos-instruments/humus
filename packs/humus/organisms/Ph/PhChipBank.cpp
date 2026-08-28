#include "Ph/PhChipBank.h"

namespace hum::phbank {

namespace {

using Op = PhChip::Op;
using Patch = PhChip::Patch;

constexpr Op op(uint8_t mult, uint8_t tl, uint8_t ar, uint8_t dr, uint8_t sl,
                uint8_t rr, uint8_t d2r = 0) {
    return Op{mult, 3, tl, 0, ar, dr, d2r, rr, sl, 0};
}

constexpr uint8_t kChain = 0, kPair = 4, kAll = 7;

Patch make(uint8_t alg, uint8_t fb, Op a, Op b, Op c, Op d) {
    Patch p;
    p.alg = alg;
    p.fb = fb;
    p.ops = {a, b, c, d};
    return p;
}

}

const std::vector<PhChip::Instrument>& chipFactory() {
    static const std::vector<PhChip::Instrument> bank = {
        {"Soil Bass",  make(kPair, 4, op(1, 30, 31, 14, 4, 9), op(1, 6, 31, 10, 2, 8),
                                      op(2, 46, 31, 16, 5, 9), op(1, 14, 31, 12, 2, 8))},
        {"Round Bass", make(kPair, 2, op(1, 40, 31, 12, 3, 8), op(1, 6, 31, 8, 1, 7),
                                      op(1, 52, 31, 14, 4, 8), op(1, 16, 31, 9, 1, 7))},
        {"Growl Bass", make(kChain, 7, op(1, 34, 31, 16, 5, 9), op(2, 40, 31, 14, 4, 9),
                                       op(1, 44, 31, 12, 4, 9), op(1, 6, 31, 10, 2, 8))},
        {"Glass Bell", make(kPair, 2, op(7, 34, 31, 8, 2, 4), op(1, 8, 31, 6, 1, 3),
                                      op(11, 44, 31, 9, 2, 4), op(2, 16, 31, 7, 1, 3))},
        {"Chime",      make(kPair, 1, op(9, 40, 31, 7, 2, 3), op(1, 10, 31, 5, 1, 2),
                                      op(13, 46, 31, 8, 2, 3), op(4, 20, 31, 6, 1, 2))},
        {"Marimba",    make(kPair, 0, op(4, 38, 31, 18, 6, 12), op(1, 8, 31, 16, 4, 11),
                                      op(7, 52, 31, 20, 7, 12), op(2, 24, 31, 17, 5, 11))},
        {"Wood",       make(kChain, 6, op(6, 42, 31, 24, 8, 14), op(3, 46, 31, 22, 7, 13),
                                       op(2, 44, 31, 22, 7, 13), op(1, 10, 31, 20, 6, 13))},
        {"Pluck",      make(kPair, 5, op(3, 36, 31, 18, 5, 11), op(1, 8, 31, 15, 3, 10),
                                      op(5, 48, 31, 19, 6, 11), op(1, 18, 31, 16, 4, 10))},
        {"Brass",      make(kPair, 3, op(1, 32, 24, 10, 3, 8), op(1, 8, 26, 7, 1, 7),
                                      op(2, 44, 23, 11, 3, 8), op(1, 14, 25, 8, 1, 7))},
        {"Reed",       make(kPair, 4, op(2, 38, 28, 9, 2, 8), op(1, 8, 30, 6, 1, 7),
                                      op(3, 50, 27, 10, 3, 8), op(1, 18, 29, 7, 1, 7))},
        {"Organ",      make(kAll, 0, op(1, 12, 31, 4, 0, 8), op(2, 22, 31, 4, 0, 8),
                                     op(4, 30, 31, 4, 0, 8), op(8, 38, 31, 4, 0, 8))},
        {"Hollow",     make(kAll, 2, op(1, 14, 31, 6, 1, 8), op(3, 30, 31, 6, 1, 8),
                                     op(5, 40, 31, 7, 1, 8), op(7, 48, 31, 7, 1, 8))},
        {"Deep Pad",   make(kPair, 1, op(1, 40, 14, 6, 2, 5), op(1, 10, 16, 4, 1, 4),
                                      op(2, 50, 13, 6, 2, 5), op(1, 18, 15, 4, 1, 4))},
        {"Glass Pad",  make(kAll, 1, op(1, 16, 12, 5, 1, 5), op(2, 28, 12, 5, 1, 5),
                                     op(6, 42, 11, 6, 2, 5), op(9, 50, 11, 6, 2, 5))},
        {"Lead",       make(kPair, 6, op(1, 26, 31, 10, 3, 8), op(1, 6, 31, 8, 1, 7),
                                      op(2, 34, 31, 11, 3, 8), op(1, 12, 31, 9, 1, 7))},
        {"Tick",       make(kChain, 7, op(12, 30, 31, 28, 9, 15), op(9, 36, 31, 28, 9, 15),
                                       op(6, 40, 31, 27, 9, 15), op(3, 8, 31, 26, 8, 15))},
    };
    return bank;
}

}
