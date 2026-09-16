// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "io/SignalFile.h"

#include "hum/Number.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace hum::signalfile {
namespace {

constexpr int kEdfHeaderBytes = 256;
constexpr int kMaxSignals = 512;
constexpr juce::int64 kMaxRead = 64ll * 1024 * 1024;

bool toNumber(const juce::String& s, double& v) {
    const auto text = s.trim().toStdString();
    if (text.empty()) return false;
    const char* stop = nullptr;
    const double parsed = scanDouble(text.c_str(), &stop);
    if (stop == nullptr || stop == text.c_str() || *stop != '\0') return false;
    v = parsed;
    return true;
}

bool numeric(const juce::String& s) {
    double v = 0.0;
    return toNumber(s, v);
}

juce::StringArray fields(const juce::String& line) {
    juce::String norm = line;
    norm = norm.replaceCharacters("\t;", "  ");
    auto out = juce::StringArray::fromTokens(norm.contains(",") ? norm : norm,
                                             norm.contains(",") ? "," : " ", "\"");
    out.trim();
    out.removeEmptyStrings();
    return out;
}

double medianStep(const std::vector<double>& t) {
    if (t.size() < 3) return 0.0;
    std::vector<double> d;
    d.reserve(t.size() - 1);
    for (std::size_t i = 1; i < t.size(); ++i) d.push_back(t[i] - t[i - 1]);
    std::sort(d.begin(), d.end());
    const double m = d[d.size() / 2];
    return m > 1.0e-12 && m < 1.0e4 ? m : 0.0;
}

std::string trimmedField(const char* p, int n) {
    std::string s(p, (std::size_t) n);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
    std::size_t a = 0;
    while (a < s.size() && s[a] == ' ') ++a;
    return s.substr(a);
}

double fieldNumber(const char* p, int n) {
    return scanDouble(trimmedField(p, n).c_str());
}

long fieldInteger(const char* p, int n) {
    return std::strtol(trimmedField(p, n).c_str(), nullptr, 10);
}

bool integral(const std::vector<double>& v) {
    for (double x : v)
        if (std::abs(x - std::floor(x)) > 1.0e-9) return false;
    return true;
}

bool rising(const std::vector<double>& v) {
    for (std::size_t i = 1; i < v.size(); ++i)
        if (v[i] <= v[i - 1]) return false;
    return true;
}

bool counting(const std::vector<double>& v) {
    if (v.size() < 2 || !integral(v)) return false;
    std::size_t steps = 0;
    for (std::size_t i = 1; i < v.size(); ++i) {
        const double d = v[i] - v[i - 1];
        if (d < 0.0 || std::abs(d - 1.0) < 1.0e-9) ++steps;
    }
    return steps * 10 >= (v.size() - 1) * 9;
}

bool flat(const std::vector<double>& v) {
    double lo = v.empty() ? 0.0 : v.front(), hi = lo;
    for (double x : v) { lo = std::min(lo, x); hi = std::max(hi, x); }
    return hi - lo < 1.0e-12;
}

std::string headerLabel(const juce::StringArray& cells) {
    juce::StringArray keep;
    for (const auto& c : cells) keep.add(c.trim());
    return keep.joinIntoString("\t").toStdString();
}

std::string labelFor(const std::string& header, const std::vector<int>& columns, int which) {
    if (header.empty()) return {};
    const auto names = juce::StringArray::fromTokens(juce::String(header), "\t", "");
    const int index = which < (int) columns.size() ? columns[(std::size_t) which] : -1;
    if (index >= 0 && index < names.size()) return names[index].trim().toStdString();
    return {};
}

bool annotationChannel(const std::string& label) {
    auto lower = label;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char) std::tolower(c); });
    return lower.find("annotation") != std::string::npos
           || lower.find("status") != std::string::npos;
}

}

const char* kindName(Kind k) {
    switch (k) {
        case Kind::TextColumns: return "text columns";
        case Kind::Biosignal: return "biosignal recording";
        case Kind::CircuitSim: return "circuit simulation";
        case Kind::None: break;
    }
    return "";
}

bool loadTextColumns(const juce::String& text, Signal& out) {
    juce::StringArray lines;
    lines.addLines(text);

    std::vector<juce::StringArray> body;
    std::string label;
    for (const auto& raw : lines) {
        const auto line = raw.trim();
        if (line.isEmpty() || line.startsWithChar('#') || line.startsWithChar('%')
            || line.startsWithChar(';') || line.startsWithChar('/'))
            continue;
        auto cells = fields(line);
        if (cells.isEmpty()) continue;
        int numbers = 0;
        for (const auto& c : cells) numbers += numeric(c) ? 1 : 0;
        if (numbers == 0) {
            if (body.empty()) label = headerLabel(cells);
            continue;
        }
        body.push_back(std::move(cells));
        if ((int) body.size() >= kMaxSamples) break;
    }
    if ((int) body.size() < kLeastSamples) return false;

    const int width = body.front().size();
    std::vector<int> columns;
    for (int i = 0; i < width; ++i)
        if (numeric(body.front()[i])) columns.push_back(i);
    if (columns.empty()) return false;

    std::vector<std::vector<double>> rows;
    rows.reserve(body.size());
    for (const auto& cells : body) {
        if (cells.size() != width) continue;
        std::vector<double> row;
        row.reserve(columns.size());
        bool whole = true;
        for (int i : columns) {
            double v = 0.0;
            if (!toNumber(cells[i], v)) { whole = false; break; }
            row.push_back(v);
        }
        if (whole) rows.push_back(std::move(row));
    }
    if ((int) rows.size() < kLeastSamples) return false;

    const int columnCount = (int) columns.size();
    std::vector<std::vector<double>> byColumn((std::size_t) columnCount);
    for (int c = 0; c < columnCount; ++c) {
        byColumn[(std::size_t) c].reserve(rows.size());
        for (const auto& r : rows) byColumn[(std::size_t) c].push_back(r[(std::size_t) c]);
    }

    std::vector<bool> signalLike((std::size_t) columnCount, false);
    int signals = 0;
    for (int c = 0; c < columnCount; ++c) {
        const auto& v = byColumn[(std::size_t) c];
        signalLike[(std::size_t) c] = !rising(v) && !counting(v) && !flat(v);
        signals += signalLike[(std::size_t) c] ? 1 : 0;
    }

    int valueColumn = -1;
    for (int c = 0; c < columnCount && valueColumn < 0; ++c)
        if (signalLike[(std::size_t) c]) valueColumn = c;
    if (valueColumn < 0)
        valueColumn = columnCount >= 2 && rising(byColumn.front()) ? 1 : 0;

    double rate = 0.0;
    if (columnCount >= 2 && rising(byColumn.front()) && !counting(byColumn.front()))
        if (const double step = medianStep(byColumn.front()); step > 0.0) rate = 1.0 / step;

    out.samples.clear();
    out.samples.reserve(rows.size());
    for (const auto& r : rows) out.samples.push_back((float) r[(std::size_t) valueColumn]);
    out.channels = signals > 0 ? signals : columnCount;
    out.sampleRate = rate;
    out.label = labelFor(label, columns, valueColumn);
    out.kind = Kind::TextColumns;
    return true;
}

bool loadBiosignal(const juce::MemoryBlock& bytes, Signal& out) {
    const auto* base = static_cast<const char*>(bytes.getData());
    const auto size = (juce::int64) bytes.getSize();
    if (base == nullptr || size < kEdfHeaderBytes) return false;

    const bool wide = (unsigned char) base[0] == 0xFFu;
    if (!wide && trimmedField(base, 8) != "0") return false;
    const int wordBytes = wide ? 3 : 2;

    const long ns = fieldInteger(base + 252, 4);
    const long records = fieldInteger(base + 236, 8);
    const double duration = fieldNumber(base + 244, 8);
    if (ns <= 0 || ns > kMaxSignals || records <= 0 || duration <= 0.0) return false;
    const juce::int64 headerBytes = kEdfHeaderBytes + (juce::int64) ns * kEdfHeaderBytes;
    if (size < headerBytes) return false;

    const char* labels = base + kEdfHeaderBytes;
    const char* physMin = labels + 16 * ns + 80 * ns + 8 * ns;
    const char* physMax = physMin + 8 * ns;
    const char* digMin = physMax + 8 * ns;
    const char* digMax = digMin + 8 * ns;
    const char* perRecord = digMax + 8 * ns + 80 * ns;

    std::vector<long> counts((std::size_t) ns, 0);
    long recordWords = 0;
    for (long i = 0; i < ns; ++i) {
        counts[(std::size_t) i] = fieldInteger(perRecord + 8 * i, 8);
        if (counts[(std::size_t) i] < 0 || counts[(std::size_t) i] > kMaxSamples) return false;
        recordWords += counts[(std::size_t) i];
    }
    if (recordWords <= 0) return false;

    long pick = -1;
    for (long i = 0; i < ns && pick < 0; ++i)
        if (counts[(std::size_t) i] > 0
            && !annotationChannel(trimmedField(labels + 16 * i, 16)))
            pick = i;
    if (pick < 0) return false;

    long skipWords = 0;
    for (long i = 0; i < pick; ++i) skipWords += counts[(std::size_t) i];
    const long take = counts[(std::size_t) pick];

    const double pMin = fieldNumber(physMin + 8 * pick, 8);
    const double pMax = fieldNumber(physMax + 8 * pick, 8);
    const double dMin = fieldNumber(digMin + 8 * pick, 8);
    const double dMax = fieldNumber(digMax + 8 * pick, 8);
    const double span = dMax - dMin;
    const double scale = std::abs(span) > 1.0e-9 ? (pMax - pMin) / span : 1.0;

    const auto* data = reinterpret_cast<const unsigned char*>(base) + headerBytes;
    const juce::int64 available = size - headerBytes;
    const juce::int64 recordBytes = (juce::int64) recordWords * wordBytes;
    const long usable = (long) std::min<juce::int64>(records, available / recordBytes);
    if (usable <= 0) return false;

    out.samples.clear();
    out.samples.reserve((std::size_t) std::min<juce::int64>((juce::int64) usable * take,
                                                            kMaxSamples));
    for (long r = 0; r < usable; ++r) {
        const unsigned char* p = data + (juce::int64) r * recordBytes
                               + (juce::int64) skipWords * wordBytes;
        for (long i = 0; i < take; ++i) {
            const unsigned char* w = p + (juce::int64) i * wordBytes;
            int raw = wide ? (int) (w[0] | (w[1] << 8) | (w[2] << 16))
                           : (int) (std::int16_t) (w[0] | (w[1] << 8));
            if (wide && (raw & 0x800000) != 0) raw -= 0x1000000;
            out.samples.push_back((float) ((raw - dMin) * scale + pMin));
            if ((int) out.samples.size() >= kMaxSamples) break;
        }
        if ((int) out.samples.size() >= kMaxSamples) break;
    }
    if ((int) out.samples.size() < kLeastSamples) return false;

    out.sampleRate = (double) take / duration;
    out.label = trimmedField(labels + 16 * pick, 16);
    out.channels = (int) ns;
    out.kind = Kind::Biosignal;
    return true;
}

bool loadCircuitSim(const juce::MemoryBlock& bytes, Signal& out) {
    const auto* base = static_cast<const char*>(bytes.getData());
    const auto size = (juce::int64) bytes.getSize();
    if (base == nullptr || size < 64) return false;

    const juce::int64 scan = std::min<juce::int64>(size, 1 << 16);
    const juce::String head(juce::CharPointer_UTF8(base), (std::size_t) scan);
    if (!head.containsIgnoreCase("No. Variables:") || !head.containsIgnoreCase("No. Points:"))
        return false;

    long vars = 0, points = 0;
    bool complexValues = false;
    juce::int64 dataStart = -1;
    juce::StringArray names;
    juce::StringArray lines;
    lines.addLines(head);
    juce::int64 cursor = 0;
    bool readingNames = false;
    for (const auto& line : lines) {
        const juce::int64 lineBytes = (juce::int64) line.getNumBytesAsUTF8() + 1;
        const auto t = line.trim();
        if (t.startsWithIgnoreCase("No. Variables:")) vars = t.getTrailingIntValue();
        else if (t.startsWithIgnoreCase("No. Points:")) points = t.getTrailingIntValue();
        else if (t.startsWithIgnoreCase("Flags:")) complexValues = t.containsIgnoreCase("complex");
        else if (t.startsWithIgnoreCase("Variables:")) readingNames = true;
        else if (t.startsWithIgnoreCase("Binary:")) {
            dataStart = cursor + lineBytes;
            break;
        } else if (readingNames) {
            const auto cells = juce::StringArray::fromTokens(t, " \t", "");
            if (cells.size() >= 2) names.add(cells[1]);
        }
        cursor += lineBytes;
    }
    if (vars <= 0 || vars > kMaxSignals || points <= 0 || dataStart < 0) return false;

    const int perValue = complexValues ? 2 : 1;
    const juce::int64 stride = (juce::int64) vars * perValue * 8;
    const juce::int64 available = size - dataStart;
    const long usable = (long) std::min<juce::int64>(points, available / stride);
    if (usable < kLeastSamples) return false;

    const long valueVar = vars > 1 ? 1 : 0;
    const auto* data = reinterpret_cast<const unsigned char*>(base) + dataStart;
    auto readDouble = [](const unsigned char* p) {
        double v = 0.0;
        std::memcpy(&v, p, sizeof(double));
        return v;
    };

    out.samples.clear();
    out.samples.reserve((std::size_t) std::min<long>(usable, kMaxSamples));
    std::vector<double> times;
    if (vars > 1) times.reserve((std::size_t) std::min<long>(usable, kMaxSamples));
    for (long i = 0; i < usable && (int) out.samples.size() < kMaxSamples; ++i) {
        const unsigned char* row = data + (juce::int64) i * stride;
        out.samples.push_back(
            (float) readDouble(row + (juce::int64) valueVar * perValue * 8));
        if (vars > 1) times.push_back(readDouble(row));
    }
    if ((int) out.samples.size() < kLeastSamples) return false;

    bool rising = times.size() > 2;
    for (std::size_t i = 1; i < times.size() && rising; ++i) rising = times[i] > times[i - 1];
    if (const double step = rising ? medianStep(times) : 0.0; step > 0.0)
        out.sampleRate = 1.0 / step;
    else
        out.sampleRate = 0.0;
    out.label = names.size() > valueVar ? names[(int) valueVar].toStdString() : std::string();
    out.channels = (int) vars;
    out.kind = Kind::CircuitSim;
    return true;
}

bool load(const juce::File& file, Signal& out) {
    if (!file.existsAsFile() || file.getSize() <= 0) return false;
    juce::FileInputStream in(file);
    if (!in.openedOk()) return false;
    juce::MemoryBlock bytes;
    in.readIntoMemoryBlock(bytes, (std::size_t) std::min<juce::int64>(file.getSize(), kMaxRead));
    if (bytes.getSize() == 0) return false;

    if (loadBiosignal(bytes, out)) return true;
    if (loadCircuitSim(bytes, out)) return true;

    const auto* p = static_cast<const char*>(bytes.getData());
    const auto n = bytes.getSize();
    std::size_t printable = 0;
    const std::size_t look = std::min<std::size_t>(n, 4096);
    for (std::size_t i = 0; i < look; ++i) {
        const unsigned char c = (unsigned char) p[i];
        if (c == '\n' || c == '\r' || c == '\t' || (c >= ' ' && c <= '~')) ++printable;
    }
    if (look == 0 || printable * 10 < look * 9) return false;
    return loadTextColumns(juce::String::fromUTF8(p, (int) std::min<std::size_t>(n, 1u << 24)),
                           out);
}

}
