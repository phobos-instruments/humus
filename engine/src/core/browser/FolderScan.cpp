// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/browser/FolderScan.h"

#include <chrono>
#include <cstdint>
#include <system_error>
#include <utility>

#include "core/browser/FileTimes.h"
#include "hum/FileBytes.h"

namespace hum::browser {

namespace {

bool hidden(const std::filesystem::path& p) {
    const auto name = utf8Text(p.filename());
    return name.empty() || name[0] == '.' || (name.size() > 7 && name.compare(name.size() - 7, 7, ".saving") == 0);
}

}

std::int64_t secondsSinceEpoch(std::filesystem::file_time_type t) {
    using namespace std::chrono;
    const auto sys = time_point_cast<system_clock::duration>(t - std::filesystem::file_time_type::clock::now()
                                                             + system_clock::now());
    return (std::int64_t) duration_cast<seconds>(sys.time_since_epoch()).count();
}

bool onMissingDrive(const std::string& path) {
    std::error_code ec;
    const auto p = utf8Path(path);
    const auto root = p.root_path();
    if (!root.empty() && !std::filesystem::exists(root, ec)) return true;
    const auto text = utf8Text(p);
    const std::string volumes = "/Volumes/";
    if (text.rfind(volumes, 0) != 0) return false;
    const auto end = text.find('/', volumes.size());
    const auto drive = text.substr(0, end == std::string::npos ? text.size() : end);
    return !std::filesystem::exists(utf8Path(drive), ec);
}

bool isUnder(const std::string& path, const std::string& root) {
    if (root.empty() || path.size() <= root.size() || path.compare(0, root.size(), root) != 0) return false;
    return root.back() == '/' || root.back() == '\\' || path[root.size()] == '/' || path[root.size()] == '\\';
}

FolderScan::FolderScan(std::string root) : root_(std::move(root)) {
    std::error_code ec;
    const auto p = utf8Path(root_);
    if (std::filesystem::is_directory(p, ec)) pending_.push_back(p);
    else done_ = true;
}

void FolderScan::enter(const std::filesystem::path& dir) { pending_.push_back(dir); }

bool FolderScan::step(int budget, const std::function<void(const Seen&)>& onSeen) {
    std::error_code ec;
    while (budget > 0 && !pending_.empty()) {
        const auto dir = pending_.back();
        pending_.pop_back();
        std::filesystem::directory_iterator it(dir, std::filesystem::directory_options::skip_permission_denied, ec);
        if (ec) { ec.clear(); continue; }
        for (const auto& item : it) {
            const auto& p = item.path();
            if (hidden(p) || item.is_symlink(ec)) continue;
            const auto path = utf8Text(p);
            Seen seen;
            seen.path = path;
            seen.modified = secondsSinceEpoch(item.last_write_time(ec));
            seen.created = createdSeconds(path);
            if (item.is_directory(ec)) {
                seen.kind = kindOfFolder(path);
                if (seen.kind != Kind::Other) onSeen(seen);
                if (seen.kind != Kind::Shader) enter(p);
                continue;
            }
            if (!item.is_regular_file(ec)) continue;
            seen.kind = kindOfFile(path);
            if (seen.kind == Kind::Other) continue;
            seen.size = (std::int64_t) item.file_size(ec);
            onSeen(seen);
            --budget;
        }
    }
    done_ = pending_.empty();
    return done_;
}

void ScanPass::take(const Seen& seen) {
    seen_.insert(seen.path);
    if (index_.find(seen.path) == nullptr) fresh_.push_back(seen);
    index_.observe(seen.path, seen.size, seen.modified, now_, seen.kind, seen.created);
}

int ScanPass::finish() {
    std::vector<std::string> gone;
    for (const auto& [path, e] : index_.entries())
        if (isUnder(path, root_) && seen_.count(path) == 0) gone.push_back(path);
    int moved = 0;
    for (const auto& f : fresh_)
        if (!index_.relocate(f.path, f.size, gone).empty()) ++moved;
    for (const auto& path : gone) {
        const auto* e = index_.find(path);
        if (e != nullptr && !e->annotated()) index_.forget(path);
    }
    return moved;
}

}
