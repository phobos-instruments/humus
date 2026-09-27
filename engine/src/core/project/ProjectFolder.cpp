// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/project/ProjectFolder.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

#include "hum/FileBytes.h"

namespace hum::project {

namespace {

namespace fs = std::filesystem;

std::string text(const fs::path& p) { return p.empty() ? std::string() : utf8Text(p); }

fs::path at(const std::string& path) {
    return path.empty() ? fs::path() : utf8Path(path).lexically_normal();
}

bool isFile(const fs::path& p) {
    std::error_code ec;
    return !p.empty() && fs::is_regular_file(p, ec);
}

bool isDir(const fs::path& p) {
    std::error_code ec;
    return !p.empty() && fs::is_directory(p, ec);
}

fs::path named(fs::path stem, const char* suffix) {
    stem += suffix;
    return stem;
}

fs::path folderFor(const fs::path& document) { return named(document.stem(), kFolderSuffix); }

bool inside(const fs::path& file, const fs::path& dir) {
    if (file.empty() || dir.empty()) return false;
    const auto rel = file.lexically_relative(dir);
    return !rel.empty() && rel != "." && *rel.begin() != "..";
}

fs::path pastFirstFolder(const fs::path& relative) {
    fs::path out;
    auto it = relative.begin();
    if (it != relative.end() && std::next(it) != relative.end()) ++it;
    for (; it != relative.end(); ++it) out /= *it;
    return out;
}

bool sameBytes(const fs::path& a, const fs::path& b) {
    std::error_code ec;
    if (fs::file_size(a, ec) != fs::file_size(b, ec) || ec) return false;
    auto one = openForReading(text(a)), two = openForReading(text(b));
    return std::equal(std::istreambuf_iterator<char>(one), std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(two));
}

fs::path freeSibling(const fs::path& wanted) {
    if (!fs::exists(wanted)) return wanted;
    for (int n = 2;; ++n) {
        auto next = wanted.parent_path()
                    / utf8Path(utf8Text(wanted.stem()) + " (" + std::to_string(n) + ")"
                               + utf8Text(wanted.extension()));
        if (!fs::exists(next)) return next;
    }
}

}

bool isProject(const std::string& dir) { return isFile(at(dir) / kMarker); }

std::string rootOf(const std::string& document) {
    if (document.empty()) return {};
    const auto dir = at(document).parent_path();
    return isFile(dir / kMarker) ? text(dir) : std::string();
}

std::string landingFor(const std::string& chosen) {
    const auto pick = at(chosen);
    if (pick.empty() || isFile(pick)) return chosen;
    const auto dir = pick.parent_path();
    if (isFile(dir / kMarker) || dir.filename() == folderFor(pick)) return text(pick);
    return text(dir / folderFor(pick) / pick.filename());
}

std::string saveStartDir(const std::string& currentDocument, const std::string& lastUsed) {
    if (const auto root = rootOf(currentDocument); !root.empty()) return root;
    return isProject(lastUsed) ? text(at(lastUsed).parent_path()) : lastUsed;
}

bool mark(const std::string& root) {
    if (isProject(root)) return true;
    const auto dir = at(root);
    std::error_code ec;
    if (!isDir(dir) && !fs::create_directories(dir, ec)) return false;
    auto out = openForWriting(text(dir / kMarker));
    out << "{ \"humus-project\": 1 }\n";
    return out.good();
}

std::string loopsDirFor(const std::string& document) {
    const auto doc = at(document);
    if (const auto root = rootOf(document); !root.empty())
        return text(at(root) / kLoops / doc.stem());
    return text(doc.parent_path() / named(doc.stem(), kLegacyLoopsSuffix));
}

std::string recordingsDirFor(const std::string& document) {
    const auto root = rootOf(document);
    return root.empty() ? std::string() : text(at(root) / kRecordings);
}

std::string stampedFolderName(int year, int month, int day, int hour, int minute, int second) {
    char out[32];
    std::snprintf(out, sizeof out, "%04d-%02d-%02d_%02d-%02d-%02d", year, month, day, hour, minute,
                  second);
    return out;
}

bool isInside(const std::string& file, const std::string& dir) { return inside(at(file), at(dir)); }

std::string gatherTarget(const std::string& file, const Shelves& shelves) {
    const auto source = at(file), root = at(shelves.root), former = at(shelves.formerRoot);
    if (root.empty() || inside(source, root)) return {};
    const auto into = root / kRecordings;
    if (inside(source, former)) {
        if (inside(source, former / kLoops)) return {};
        const auto formerRecordings = former / kRecordings;
        return text(into / (inside(source, formerRecordings)
                                ? source.lexically_relative(formerRecordings)
                                : source.filename()));
    }
    const auto shared = at(shelves.sharedRecordings);
    if (!inside(source, shared)) return {};
    return text(into / pastFirstFolder(source.lexically_relative(shared)));
}

std::string gatheredCopy(const std::string& source, const std::string& wanted) {
    const auto from = at(source);
    auto to = at(wanted);
    if (!isFile(from) || to.empty()) return {};
    if (isFile(to) && sameBytes(from, to)) return text(to);
    to = freeSibling(to);
    std::error_code ec;
    fs::create_directories(to.parent_path(), ec);
    return fs::copy_file(from, to, ec) ? text(to) : std::string();
}


int pruneEmptyFolders(const std::string& dir) {
    const auto root = at(dir);
    std::error_code ec;
    if (root.empty() || !fs::is_directory(root, ec)) return 0;
    std::vector<fs::path> deepestFirst;
    for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec),
         end;
         it != end && !ec; it.increment(ec))
        if (it->is_directory(ec)) deepestFirst.push_back(it->path());
    std::sort(deepestFirst.begin(), deepestFirst.end(), [](const fs::path& a, const fs::path& b) {
        return std::distance(a.begin(), a.end()) > std::distance(b.begin(), b.end());
    });
    int gone = 0;
    for (const auto& folder : deepestFirst) {
        if (fs::directory_iterator(folder, ec) != fs::directory_iterator()) continue;
        if (fs::remove(folder, ec)) ++gone;
    }
    return gone;
}

}
