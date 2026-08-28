#include "Ph/PhSixOpBank.h"

#include <cstring>

namespace hum::phbank {

namespace {

struct Env { uint8_t r[4], l[4]; };
constexpr Env kEnvs[] = {
    {{99, 40, 25, 55}, {99, 95, 90, 0}},
    {{99, 62, 46, 62}, {99, 70, 0, 0}},
    {{99, 82, 66, 76}, {99, 45, 0, 0}},
    {{50, 36, 30, 44}, {99, 90, 85, 0}},
    {{28, 30, 28, 38}, {99, 92, 88, 0}},
    {{99, 46, 32, 46}, {99, 60, 20, 0}},
    {{99, 93, 86, 93}, {99, 25, 0, 0}},
};

struct OpSpec { uint8_t coarse, level, env; };

struct VoiceSpec {
    const char* name;
    uint8_t alg;
    uint8_t fb;
    OpSpec ops[6];
};

constexpr uint8_t kStack = 4;
constexpr uint8_t kChain = 0;
constexpr uint8_t kAdd = 31;
constexpr uint8_t kDeep = 15;
constexpr OpSpec kOff{0, 0, 0};

constexpr VoiceSpec kVoices[] = {
    {"PURE SINE", kStack, 0, {kOff, kOff, kOff, kOff, kOff, {1, 99, 0}}},
    {"SOFT TINE", kStack, 2, {kOff, kOff, {1, 46, 1}, {1, 74, 5}, {14, 58, 2}, {1, 99, 5}}},
    {"WARM REED", kStack, 3, {kOff, kOff, {2, 52, 0}, {1, 62, 0}, {1, 72, 0}, {1, 99, 0}}},
    {"GLASS HUM", kStack, 5, {kOff, kOff, {5, 42, 5}, {2, 56, 5}, {7, 62, 5}, {1, 99, 5}}},
    {"LOAM BASS", kStack, 6, {kOff, kOff, kOff, kOff, {1, 80, 1}, {1, 99, 1}}},
    {"DEEP ROOT", kStack, 2, {kOff, kOff, {1, 40, 0}, {1, 55, 0}, {2, 62, 1}, {1, 99, 0}}},
    {"GROWL", kStack, 7, {kOff, kOff, {3, 58, 1}, {1, 70, 1}, {1, 86, 1}, {1, 99, 1}}},
    {"MARIMBA", kStack, 0, {kOff, kOff, kOff, kOff, {4, 66, 2}, {1, 99, 2}}},
    {"TUBE BELL", kStack, 1, {kOff, kOff, {7, 48, 5}, {3, 60, 5}, {11, 55, 5}, {1, 99, 5}}},
    {"CHIME", kStack, 3, {kOff, kOff, {9, 44, 5}, {4, 52, 5}, {14, 50, 5}, {1, 99, 5}}},
    {"WOOD", kStack, 4, {kOff, kOff, kOff, kOff, {6, 72, 6}, {1, 99, 6}}},
    {"DRIP", kStack, 5, {kOff, kOff, kOff, kOff, {12, 64, 6}, {2, 99, 6}}},
    {"SOIL PAD", kStack, 1, {{2, 34, 3}, {1, 70, 3}, {3, 30, 3}, {2, 62, 3}, {1, 40, 3}, {1, 99, 3}}},
    {"MIST PAD", kStack, 0, {{4, 26, 4}, {2, 66, 4}, {1, 30, 4}, {1, 70, 4}, {3, 24, 4}, {1, 99, 4}}},
    {"GLASS PAD", kStack, 2, {{7, 30, 4}, {2, 64, 4}, {5, 26, 4}, {1, 68, 4}, {3, 30, 4}, {1, 99, 4}}},
    {"BOWED", kStack, 3, {kOff, kOff, {2, 44, 3}, {1, 66, 3}, {1, 52, 3}, {1, 99, 3}}},
    {"SOFT HORN", kStack, 4, {kOff, kOff, {1, 56, 3}, {2, 60, 3}, {1, 64, 3}, {1, 99, 3}}},
    {"BRASS", kStack, 5, {kOff, kOff, {1, 66, 0}, {2, 66, 0}, {1, 78, 0}, {1, 99, 0}}},
    {"HOLLOW", kStack, 6, {kOff, kOff, {3, 50, 0}, {3, 60, 0}, {2, 40, 0}, {1, 99, 0}}},
    {"SQUARE LEAD", kAdd, 0, {{9, 26, 0}, {7, 32, 0}, {5, 42, 0}, {3, 56, 0}, {1, 74, 0}, {1, 99, 0}}},
    {"FIFTH LEAD", kStack, 2, {kOff, kOff, {3, 60, 0}, {3, 70, 0}, {1, 60, 0}, {1, 99, 0}}},
    {"WHISTLE", kStack, 0, {kOff, kOff, kOff, kOff, {1, 34, 3}, {1, 99, 3}}},
    {"CLAV", kStack, 6, {kOff, kOff, {1, 62, 1}, {2, 66, 1}, {3, 70, 1}, {1, 99, 1}}},
    {"HARPSI", kStack, 4, {kOff, kOff, {2, 58, 1}, {3, 62, 1}, {5, 60, 1}, {1, 99, 1}}},
    {"E PIANO", kStack, 2, {kOff, kOff, {1, 50, 1}, {1, 76, 1}, {12, 54, 2}, {1, 99, 5}}},
    {"TOY PIANO", kStack, 3, {kOff, kOff, {6, 50, 2}, {2, 70, 2}, {10, 52, 2}, {1, 99, 2}}},
    {"ORGAN", kAdd, 0, {{8, 40, 0}, {6, 46, 0}, {4, 54, 0}, {3, 62, 0}, {2, 72, 0}, {1, 99, 0}}},
    {"REED ORGAN", kAdd, 2, {{5, 34, 0}, {4, 40, 0}, {3, 50, 0}, {2, 60, 0}, {1, 70, 0}, {1, 99, 0}}},
    {"SPRING", kDeep, 5, {{9, 40, 5}, {5, 46, 5}, {3, 52, 5}, {2, 58, 5}, {1, 64, 5}, {1, 99, 5}}},
    {"IRON", kDeep, 7, {{11, 44, 2}, {7, 48, 2}, {5, 54, 2}, {3, 58, 2}, {2, 62, 2}, {1, 99, 2}}},
    {"COMPOST", kChain, 6, {{13, 38, 1}, {6, 44, 1}, {3, 52, 1}, {1, 99, 1}, {2, 60, 1}, {1, 99, 1}}},
    {"SPORE", kChain, 4, {{15, 30, 6}, {8, 40, 6}, {4, 50, 6}, {1, 99, 6}, {5, 44, 6}, {1, 99, 6}}},
};

constexpr int kVoiceCount = (int) (sizeof(kVoices) / sizeof(kVoices[0]));

void packOp(uint8_t* p, const OpSpec& op, uint8_t detune) {
    const Env& e = kEnvs[op.env];
    for (int i = 0; i < 4; ++i) p[i] = e.r[i];
    for (int i = 0; i < 4; ++i) p[4 + i] = e.l[i];
    p[8] = 0;
    p[9] = 0; p[10] = 0;
    p[11] = 0;
    p[12] = (uint8_t) ((detune << 3) | 0);
    p[13] = (uint8_t) (2 << 2);
    p[14] = op.level;
    p[15] = (uint8_t) ((op.coarse << 1) | 0);
    p[16] = 0;
}

void packVoice(uint8_t* v, const VoiceSpec& spec) {
    std::memset(v, 0, 128);
    for (int op = 0; op < 6; ++op)
        packOp(v + op * 17, spec.ops[op], (uint8_t) (op % 2 == 0 ? 7 : 8));
    v[102] = 99; v[103] = 99; v[104] = 99; v[105] = 99;
    v[106] = 50; v[107] = 50; v[108] = 50; v[109] = 50;
    v[110] = spec.alg;
    v[111] = (uint8_t) (spec.fb | (1 << 3));
    v[112] = 35;
    v[116] = 1;
    v[117] = 24;
    const auto n = (int) std::strlen(spec.name);
    std::memcpy(v + 118, spec.name, (size_t) (n < 10 ? n : 10));
    for (int i = n; i < 10; ++i) v[118 + i] = ' ';
}

}

int sixOpFactoryCount() { return kVoiceCount; }

void fillSixOpFactory(uint8_t bank[][128], int slots) {
    for (int s = 0; s < slots; ++s)
        packVoice(bank[s], kVoices[s < kVoiceCount ? s : 0]);
}

}
