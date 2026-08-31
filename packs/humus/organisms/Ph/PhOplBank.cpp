#include "Ph/PhOplBank.h"

namespace hum::phbank {

namespace {

using Op = PhOpl::Op;
using Patch = PhOpl::Patch;

constexpr Op op(uint8_t mult, uint8_t tl, uint8_t ar, uint8_t dr, uint8_t sl,
                uint8_t rr, uint8_t ws = 0, uint8_t vibSus = 0x20) {
    return Op{(uint8_t) (vibSus | (mult & 15)), (uint8_t) (tl & 63),
              (uint8_t) ((ar << 4) | (dr & 15)), (uint8_t) ((sl << 4) | (rr & 15)),
              (uint8_t) (ws & 7)};
}

Patch make(uint8_t fb, bool am, Op mod, Op car) {
    Patch p;
    p.fbcon1 = (uint8_t) ((fb << 1) | (am ? 1 : 0));
    p.ops = {mod, car, Op{}, Op{}};
    return p;
}

}

const std::vector<PhOpl::Instrument>& oplFactory() {
    static const std::vector<PhOpl::Instrument> bank = {
        {"Card Bass",   make(4, false, op(1, 18, 15, 4, 4, 8), op(1, 4, 15, 6, 2, 8))},
        {"Buzz Bass",   make(6, false, op(1, 14, 15, 3, 2, 8, 1), op(1, 6, 15, 5, 2, 8))},
        {"Square Lead", make(3, false, op(2, 20, 14, 3, 1, 7, 3), op(1, 8, 14, 4, 1, 7))},
        {"Thin Lead",   make(5, false, op(3, 24, 15, 4, 2, 7, 1), op(1, 10, 15, 4, 1, 7, 2))},
        {"Glass Keys",  make(2, false, op(4, 26, 15, 7, 3, 6), op(1, 8, 15, 8, 3, 6))},
        {"Tin Piano",   make(3, false, op(3, 22, 15, 8, 4, 7, 1), op(1, 6, 15, 9, 4, 7))},
        {"Steel Organ", make(0, true,  op(1, 10, 15, 1, 0, 8), op(2, 12, 15, 1, 0, 8))},
        {"Reed Organ",  make(1, true,  op(2, 14, 12, 1, 0, 7, 4), op(4, 16, 12, 1, 0, 7))},
        {"Brass Rip",   make(6, false, op(1, 16, 10, 4, 1, 6), op(1, 8, 11, 5, 2, 6))},
        {"Cold Bell",   make(2, false, op(7, 28, 15, 5, 3, 4, 1), op(2, 10, 15, 6, 3, 4))},
        {"Dust Chime",  make(1, false, op(10, 32, 15, 6, 4, 4, 2), op(3, 14, 15, 7, 4, 4))},
        {"Arc String",  make(4, true,  op(1, 18, 8, 2, 1, 6, 6), op(2, 16, 9, 2, 1, 6, 1))},
    };
    return bank;
}

}
