// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "core/browser/FileIndex.h"

namespace hum::browser {

struct Seen {
    std::string path;
    Kind kind = Kind::Other;
    std::int64_t size = 0;
    std::int64_t modified = 0;
    std::int64_t created = 0;
};

class FolderScan {
public:
    explicit FolderScan(std::string root);

    bool step(int budget, const std::function<void(const Seen&)>& onSeen);
    bool done() const { return done_; }
    const std::string& root() const { return root_; }

private:
    void enter(const std::filesystem::path& dir);

    std::string root_;
    std::vector<std::filesystem::path> pending_;
    bool done_ = false;
};

class ScanPass {
public:
    ScanPass(FileIndex& index, std::string root, std::int64_t now) : index_(index), root_(std::move(root)), now_(now) {}

    void take(const Seen& seen);
    int finish();

private:
    FileIndex& index_;
    std::string root_;
    std::int64_t now_;
    std::set<std::string> seen_;
    std::vector<Seen> fresh_;
};

bool isUnder(const std::string& path, const std::string& root);
bool onMissingDrive(const std::string& path);
std::int64_t secondsSinceEpoch(std::filesystem::file_time_type t);

}
