// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace hum::browser {

enum class Kind { Other, Project, Patch, Sound, Impulse, Bank, Midi, Scale, Shader, Video, Preset, Folder };

struct KindText {
    Kind kind;
    const char* word;
    const char* badge;
};

const KindText* kindTexts(int& count);
const char* kindWord(Kind kind);
const char* kindBadge(Kind kind);
bool parseKindWord(std::string_view word, Kind& out);

Kind kindOfFile(const std::string& path);
Kind kindOfFolder(const std::string& dir);
bool isProjectFolder(const std::string& dir);
std::string fileName(const std::string& path);
bool matchesPatterns(const std::string& path, const std::string& patterns);
std::vector<Kind> kindsInPatterns(const std::string& patterns);
std::string parentOf(const std::string& path);

}
