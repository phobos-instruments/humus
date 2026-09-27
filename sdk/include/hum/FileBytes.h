// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace hum {

inline std::filesystem::path utf8Path(const std::string& path) { return std::filesystem::u8path(path); }

inline std::string utf8Text(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return std::string(text.begin(), text.end());
}

inline std::ifstream openForReading(const std::string& path) {
    return std::ifstream(utf8Path(path), std::ios::binary);
}

inline std::ofstream openForWriting(const std::string& path) {
    return std::ofstream(utf8Path(path), std::ios::binary | std::ios::trunc);
}

inline std::string stripFileScheme(std::string uri) { return uri.rfind("file://", 0) == 0 ? uri.substr(7) : uri; }

inline std::string lowerExtension(const std::string& path) {
    auto ext = utf8Text(utf8Path(stripFileScheme(path)).extension());
    for (auto& c : ext) c = (char) std::tolower((unsigned char) c);
    return ext;
}

inline bool readFileBytes(const std::string& path, std::vector<std::uint8_t>& out) {
    out.clear();
    std::error_code ec;
    const auto p = utf8Path(path);
    if (path.empty() || !std::filesystem::is_regular_file(p, ec)) return false;
    auto in = openForReading(path);
    if (!in) return false;
    const auto size = std::filesystem::file_size(p, ec);
    if (ec) return false;
    out.resize((size_t) size);
    if (size > 0 && !in.read((char*) out.data(), (std::streamsize) size)) {
        out.clear();
        return false;
    }
    return true;
}

}
