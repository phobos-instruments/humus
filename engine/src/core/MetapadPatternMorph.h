#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "hum/Pattern.h"
#include "hum/PatternMatrix.h"

namespace hum {

namespace patternmorph {

struct Contributor {
    const Pattern* pattern = nullptr;
    double weight = 0.0;
};

inline size_t dominantIndex(const std::vector<Contributor>& src) {
    size_t best = 0;
    for (size_t i = 1; i < src.size(); ++i)
        if (src[i].weight > src[best].weight) best = i;
    return best;
}

inline const PatternChannel* channelOfType(const Pattern& p, const std::string& type, int ordinal) {
    int idx = 0;
    for (const auto& ch : p.channels) {
        if (ch.type != type) continue;
        if (idx == ordinal) return &ch;
        ++idx;
    }
    return nullptr;
}

inline void voteTriggers(std::vector<int>& out,
                         const std::vector<std::pair<const std::vector<int>*, double>>& lists) {
    double wsum = 0.0;
    std::map<int, double> score;
    for (const auto& [ticks, w] : lists) {
        wsum += w;
        for (int t : *ticks) score[t] += w;
    }
    out.clear();
    for (const auto& [tick, s] : score)
        if (s >= 0.5 * wsum - 1e-9) out.push_back(tick);
}

template <typename Step, typename Decode, typename Encode, typename Blend>
inline std::string voteMatrix(const std::vector<std::pair<const std::string*, double>>& mats,
                              Decode decode, Encode encode, Blend blend) {
    double wsum = 0.0;
    size_t maxLen = 0;
    std::vector<std::pair<std::vector<Step>, double>> decoded;
    for (const auto& [m, w] : mats) {
        decoded.push_back({decode(*m), w});
        maxLen = std::max(maxLen, decoded.back().first.size());
        wsum += w;
    }
    std::vector<Step> out(maxLen);
    for (size_t s = 0; s < maxLen; ++s)
        out[s] = blend(decoded, s, wsum);
    return encode(out);
}

inline PatternChannel morphChannel(const std::vector<Contributor>& src, size_t dom,
                                   const std::string& type, int ordinal) {
    PatternChannel out = *channelOfType(*src[dom].pattern, type, ordinal);

    if (type == "trigger-timepoints") {
        std::vector<std::pair<const std::vector<int>*, double>> lists;
        for (const auto& c : src)
            if (const auto* ch = channelOfType(*c.pattern, type, ordinal))
                lists.push_back({&ch->triggers, c.weight});
        voteTriggers(out.triggers, lists);
        return out;
    }

    std::vector<std::pair<const std::string*, double>> mats;
    for (const auto& c : src)
        if (const auto* ch = channelOfType(*c.pattern, type, ordinal))
            mats.push_back({&ch->matrix, c.weight});
    using Mats = std::vector<std::pair<std::vector<BasslineStep>, double>>;
    if (type == "bassline-pattern-matrix") {
        out.matrix = voteMatrix<BasslineStep>(
            mats, decodeBassline, encodeBassline,
            [](const Mats& d, size_t s, double wsum) {
                BasslineStep st;
                double gateW = 0.0, bestW = -1.0;
                for (const auto& [steps, w] : d)
                    if (s < steps.size() && steps[s].gate) {
                        gateW += w;
                        if (w > bestW) { bestW = w; st = steps[s]; }
                    }
                st.gate = gateW >= 0.5 * wsum - 1e-9;
                return st;
            });
        return out;
    }
    using ArpMats = std::vector<std::pair<std::vector<ArpStep>, double>>;
    if (type == "trigger-tie-matrix") {
        out.matrix = voteMatrix<ArpStep>(
            mats, decodeArp, encodeArp,
            [](const ArpMats& d, size_t s, double wsum) {
                ArpStep st;
                double trigW = 0.0, tieW = 0.0;
                for (const auto& [steps, w] : d)
                    if (s < steps.size()) {
                        if (steps[s].trigger) trigW += w;
                        if (steps[s].tie) tieW += w;
                    }
                st.trigger = trigW >= 0.5 * wsum - 1e-9;
                st.tie = tieW >= 0.5 * wsum - 1e-9;
                return st;
            });
        return out;
    }
    if (type == "octave-row") {
        size_t maxLen = 0;
        for (const auto& [m, w] : mats) maxLen = std::max(maxLen, m->size());
        std::string row(maxLen, '.');
        for (size_t s = 0; s < maxLen; ++s) {
            double wsum = 0.0, up = 0.0;
            for (const auto& [m, w] : mats) {
                wsum += w;
                if (s < m->size() && (*m)[s] == '^') up += w;
            }
            if (up >= 0.5 * wsum - 1e-9) row[s] = '^';
        }
        out.matrix = row;
        return out;
    }
    if (type == "note-events") {
        struct Cell { double w = 0.0, vel = 0.0, len = 0.0; };
        std::map<std::pair<int, int>, Cell> cells;
        double wsum = 0.0;
        for (const auto& c : src) {
            const auto* ch = channelOfType(*c.pattern, type, ordinal);
            if (ch == nullptr) continue;
            wsum += c.weight;
            for (const auto& n : decodeNoteEvents(ch->matrix)) {
                auto& cell = cells[{n.tick, n.pitch}];
                cell.w += c.weight;
                cell.vel += c.weight * n.velocity;
                cell.len += c.weight * n.lengthTicks;
            }
        }
        std::vector<NoteEvent> notes;
        for (const auto& [key, cell] : cells)
            if (cell.w >= 0.5 * wsum - 1e-9)
                notes.push_back({key.first, key.second,
                                 std::max(1, (int) std::lround(cell.len / cell.w)),
                                 std::clamp((int) std::lround(cell.vel / cell.w), 1, 127)});
        out.matrix = encodeNoteEvents(notes)
                   + encodeCCEvents(decodeCCEvents(out.matrix));
        return out;
    }
    return out;
}

inline Pattern morph(const std::vector<Contributor>& src) {
    if (src.empty()) return {};
    const size_t dom = dominantIndex(src);
    Pattern out = *src[dom].pattern;
    std::map<std::string, int> ordinals;
    for (auto& ch : out.channels) {
        const int ordinal = ordinals[ch.type]++;
        ch = morphChannel(src, dom, ch.type, ordinal);
    }
    return out;
}

inline bool samePlayableContent(const Pattern& a, const Pattern& b) {
    if (a.channels.size() != b.channels.size()) return false;
    if (a.duration != b.duration || a.matrixResolution != b.matrixResolution) return false;
    for (size_t i = 0; i < a.channels.size(); ++i) {
        const auto& ca = a.channels[i];
        const auto& cb = b.channels[i];
        if (ca.type != cb.type || ca.triggers != cb.triggers || ca.matrix != cb.matrix)
            return false;
    }
    return true;
}

}
}
