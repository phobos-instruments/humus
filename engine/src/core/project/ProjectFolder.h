// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <string>

namespace hum::project {

inline constexpr const char* kMarker = "humus-project.json";
inline constexpr const char* kFolderSuffix = " Project";
inline constexpr const char* kLoops = "Loops";
inline constexpr const char* kRecordings = "Recordings";
inline constexpr const char* kLegacyLoopsSuffix = " Loops";

bool isProject(const std::string& dir);
std::string rootOf(const std::string& document);
std::string landingFor(const std::string& chosen);
std::string saveStartDir(const std::string& currentDocument, const std::string& lastUsed);
bool mark(const std::string& root);
std::string loopsDirFor(const std::string& document);
std::string recordingsDirFor(const std::string& document);
std::string stampedFolderName(int year, int month, int day, int hour, int minute, int second);

bool isInside(const std::string& file, const std::string& dir);

struct Shelves {
    std::string root;
    std::string formerRoot;
    std::string sharedRecordings;
};

std::string gatherTarget(const std::string& file, const Shelves& shelves);
std::string gatheredCopy(const std::string& source, const std::string& wanted);
int pruneEmptyFolders(const std::string& dir);

}
