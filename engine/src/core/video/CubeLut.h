// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "hum/Number.h"

namespace hum::lut {

inline constexpr int kMaxSize = 64;

struct Cube {
    int size = 0;
    float domainMin[3] = {0.0f, 0.0f, 0.0f};
    float domainMax[3] = {1.0f, 1.0f, 1.0f};
    std::vector<float> rgb;

    bool valid() const {
        return size >= 2 && size <= kMaxSize
               && rgb.size() == (std::size_t) size * size * size * 3;
    }
    int width() const { return size * size; }
    int height() const { return size; }
};

inline std::string lutWord(const std::string& line, std::size_t& at) {
    while (at < line.size() && (line[at] == ' ' || line[at] == '\t')) ++at;
    const auto from = at;
    while (at < line.size() && line[at] != ' ' && line[at] != '\t') ++at;
    return line.substr(from, at - from);
}

inline bool lutTriple(const std::string& line, std::size_t at, float* out) {
    for (int i = 0; i < 3; ++i) {
        auto word = lutWord(line, at);
        if (word.empty()) return false;
        for (auto& ch : word)
            if (ch == ',') ch = '.';
        const char* end = nullptr;
        const double v = scanDouble(word.c_str(), &end);
        if (end != word.c_str() + word.size()) return false;
        out[i] = (float) v;
    }
    return true;
}

inline Cube parseCube(const std::string& text, std::string& error) {
    Cube cube;
    int oneDimensional = 0;
    std::vector<float> table;
    std::size_t at = 0;
    while (at <= text.size()) {
        const auto eol = text.find('\n', at);
        auto line = text.substr(at, eol == std::string::npos ? std::string::npos : eol - at);
        at = eol == std::string::npos ? text.size() + 1 : eol + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        std::size_t cursor = 0;
        const auto head = lutWord(line, cursor);
        if (head.empty() || head[0] == '#') continue;
        if (head == "TITLE") continue;
        if (head == "LUT_3D_SIZE" || head == "LUT_1D_SIZE") {
            const auto n = lutWord(line, cursor);
            const int size = (int) scanDouble(n.c_str());
            if (size < 2 || size > kMaxSize) {
                error = "a LUT of " + n + " points is outside what we can load";
                return {};
            }
            cube.size = size;
            oneDimensional = head == "LUT_1D_SIZE" ? size : 0;
            continue;
        }
        if (head == "DOMAIN_MIN" || head == "DOMAIN_MAX") {
            lutTriple(line, cursor, head == "DOMAIN_MIN" ? cube.domainMin : cube.domainMax);
            continue;
        }
        float triple[3] = {};
        if (!lutTriple(line, 0, triple)) continue;
        table.insert(table.end(), triple, triple + 3);
    }
    if (cube.size < 2) {
        error = "this file does not say how big its table is";
        return {};
    }
    const int n = cube.size;
    if (oneDimensional > 0) {
        if ((int) table.size() < n * 3) {
            error = "this file has fewer points than it promised";
            return {};
        }
        cube.rgb.resize((std::size_t) n * n * n * 3);
        for (int b = 0; b < n; ++b)
            for (int g = 0; g < n; ++g)
                for (int r = 0; r < n; ++r) {
                    const auto to = (((std::size_t) b * n + g) * n + r) * 3;
                    cube.rgb[to + 0] = table[(std::size_t) r * 3 + 0];
                    cube.rgb[to + 1] = table[(std::size_t) g * 3 + 1];
                    cube.rgb[to + 2] = table[(std::size_t) b * 3 + 2];
                }
        return cube;
    }
    if ((int) (table.size() / 3) < n * n * n) {
        error = "this file has fewer points than it promised";
        return {};
    }
    table.resize((std::size_t) n * n * n * 3);
    cube.rgb = std::move(table);
    return cube;
}

inline std::vector<unsigned char> cubeStrip(const Cube& cube) {
    std::vector<unsigned char> out;
    if (!cube.valid()) return out;
    const int n = cube.size;
    out.resize((std::size_t) cube.width() * cube.height() * 4);
    for (int b = 0; b < n; ++b)
        for (int g = 0; g < n; ++g)
            for (int r = 0; r < n; ++r) {
                const auto from = (((std::size_t) b * n + g) * n + r) * 3;
                const auto to = ((std::size_t) g * cube.width() + b * n + r) * 4;
                for (int c = 0; c < 3; ++c) {
                    const float v = std::clamp(cube.rgb[from + c], 0.0f, 1.0f);
                    out[to + (std::size_t) c] = (unsigned char) std::lround(v * 255.0f);
                }
                out[to + 3] = 255;
            }
    return out;
}

}
