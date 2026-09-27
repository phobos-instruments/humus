// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <algorithm>
#include <cstdint>
#include <string>
#include <system_error>

#include "core/browser/FileIndex.h"
#include "core/json/Json.h"
#include "hum/FileBytes.h"

namespace hum::browser {

namespace {

constexpr int kFormat = 1;

json::Value strings(const std::vector<std::string>& list) {
    json::Value::Items items;
    for (const auto& s : list) items.push_back(json::Value::fromString(s));
    return json::Value::fromItems(std::move(items));
}

std::vector<std::string> stringsOf(const json::Value& v) {
    std::vector<std::string> out;
    for (const auto& item : v.items())
        if (item.isString()) out.push_back(item.text());
    return out;
}

std::string hexOf(const std::vector<std::uint8_t>& bytes) {
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const auto b : bytes) {
        out += kDigits[b >> 4];
        out += kDigits[b & 0x0f];
    }
    return out;
}

int nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

std::vector<std::uint8_t> bytesOf(const std::string& hex) {
    std::vector<std::uint8_t> out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        const int hi = nibble(hex[i]), lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back((std::uint8_t) (hi * 16 + lo));
    }
    return out;
}

std::int64_t wholeOf(const json::Value& v) { return v.isNumber() ? (std::int64_t) v.number() : 0; }

json::Value entryJson(const Entry& e) {
    json::Value o = json::Value::fromMembers({});
    o.set("path", json::Value::fromString(e.path));
    o.set("kind", json::Value::fromString(kindWord(e.kind)));
    o.set("size", json::Value::fromInt(e.size));
    o.set("modified", json::Value::fromInt(e.modified));
    o.set("added", json::Value::fromInt(e.added));
    if (e.created > 0) o.set("created", json::Value::fromInt(e.created));
    if (e.lastUsed > 0) o.set("used", json::Value::fromInt(e.lastUsed));
    if (e.rating > 0) o.set("rating", json::Value::fromInt(e.rating));
    if (e.favourite) o.set("favourite", json::Value::fromBool(true));
    if (!e.tags.empty()) o.set("tags", strings(e.tags));
    const auto& f = e.facts;
    if (f.seconds > 0.0) o.set("seconds", json::Value::fromDouble(f.seconds));
    if (f.sampleRate > 0.0) o.set("rate", json::Value::fromDouble(f.sampleRate));
    if (f.channels > 0) o.set("channels", json::Value::fromInt(f.channels));
    if (f.bpm > 0.0) o.set("bpm", json::Value::fromDouble(f.bpm));
    if (!f.key.empty()) o.set("key", json::Value::fromString(f.key));
    if (!f.peaks.empty()) o.set("peaks", json::Value::fromString(hexOf(f.peaks)));
    if (f.boxes > 0) o.set("boxes", json::Value::fromInt(f.boxes));
    if (!f.families.empty()) o.set("families", json::Value::fromString(f.families));
    if (f.probed) o.set("probed", json::Value::fromBool(true));
    return o;
}

Entry entryOf(const json::Value& o) {
    Entry e;
    e.path = o["path"].text();
    if (!parseKindWord(o["kind"].text(), e.kind)) e.kind = kindOfFile(e.path);
    e.size = wholeOf(o["size"]);
    e.modified = wholeOf(o["modified"]);
    e.added = wholeOf(o["added"]);
    e.created = wholeOf(o["created"]);
    e.lastUsed = wholeOf(o["used"]);
    e.rating = o["rating"].isNumber() ? std::clamp(o["rating"].integer(), 0, kMaxRating) : 0;
    e.favourite = o["favourite"].truthy();
    e.tags = stringsOf(o["tags"]);
    e.facts.seconds = o["seconds"].isNumber() ? o["seconds"].number() : 0.0;
    e.facts.sampleRate = o["rate"].isNumber() ? o["rate"].number() : 0.0;
    e.facts.channels = o["channels"].isNumber() ? o["channels"].integer() : 0;
    e.facts.bpm = o["bpm"].isNumber() ? o["bpm"].number() : 0.0;
    e.facts.key = o["key"].text();
    e.facts.peaks = bytesOf(o["peaks"].text());
    e.facts.boxes = o["boxes"].isNumber() ? o["boxes"].integer() : 0;
    e.facts.families = o["families"].text();
    e.facts.probed = o["probed"].truthy();
    return e;
}

}

bool saveIndex(const std::string& file, FileIndex& index) {
    json::Value root = json::Value::fromMembers({});
    root.set("format", json::Value::fromInt(kFormat));
    json::Value::Items entries;
    for (const auto& [path, e] : index.entries()) entries.push_back(entryJson(e));
    root.set("entries", json::Value::fromItems(std::move(entries)));
    json::Value::Items collections;
    for (const auto& c : index.collections()) {
        json::Value o = json::Value::fromMembers({});
        o.set("name", json::Value::fromString(c.name));
        o.set("paths", strings(c.paths));
        collections.push_back(std::move(o));
    }
    root.set("collections", json::Value::fromItems(std::move(collections)));
    root.set("watched", strings(index.watched()));

    const std::string temp = file + ".saving";
    {
        auto out = openForWriting(temp);
        if (!out) return false;
        out << json::write(root);
        if (!out.good()) return false;
    }
    std::error_code ec;
    std::filesystem::rename(utf8Path(temp), utf8Path(file), ec);
    if (ec) {
        std::filesystem::remove(utf8Path(temp), ec);
        return false;
    }
    index.markClean();
    return true;
}

bool loadIndex(const std::string& file, FileIndex& into) {
    std::string text;
    if (!json::readTextFile(file, text)) return false;
    const auto root = json::parse(text);
    if (!root.isObject() || root["format"].integer() > kFormat) return false;
    FileIndex fresh;
    for (const auto& o : root["entries"].items()) {
        auto e = entryOf(o);
        if (e.path.empty()) continue;
        fresh.entries_[e.path] = std::move(e);
    }
    for (const auto& o : root["collections"].items())
        if (const auto name = o["name"].text(); !name.empty() && fresh.collection(name) == nullptr)
            fresh.collections_.push_back({name, stringsOf(o["paths"])});
    fresh.watched_ = stringsOf(root["watched"]);
    into = std::move(fresh);
    return true;
}

}
