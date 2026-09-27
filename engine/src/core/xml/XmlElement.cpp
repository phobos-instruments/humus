// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "core/xml/Xml.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <locale>
#include <sstream>

#include "core/params/ValueText.h"

namespace hum::xml {

namespace {

bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

bool sameIgnoringCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char) a[i]) != std::tolower((unsigned char) b[i])) return false;
    return true;
}

const std::string& emptyText() {
    static const std::string empty;
    return empty;
}

}

std::unique_ptr<Element> Element::textNode(std::string text) {
    auto e = std::make_unique<Element>(std::string());
    e->text_ = std::move(text);
    return e;
}

bool Element::hasTag(const std::string& name) const { return sameIgnoringCase(tag_, name); }

const std::pair<std::string, std::string>* Element::find(const std::string& name) const {
    for (const auto& a : attributes_)
        if (a.first == name) return &a;
    return nullptr;
}

const std::string& Element::attribute(const std::string& name) const {
    const auto* a = find(name);
    return a != nullptr ? a->second : emptyText();
}

std::string Element::attribute(const std::string& name, const std::string& fallback) const {
    const auto* a = find(name);
    return a != nullptr ? a->second : fallback;
}

int Element::intAttribute(const std::string& name, int fallback) const {
    const auto* a = find(name);
    return a != nullptr ? intValue(a->second) : fallback;
}

double Element::doubleAttribute(const std::string& name, double fallback) const {
    const auto* a = find(name);
    return a != nullptr ? doubleValue(a->second) : fallback;
}

bool Element::boolAttribute(const std::string& name, bool fallback) const {
    const auto* a = find(name);
    if (a == nullptr) return fallback;
    size_t i = 0;
    while (i < a->second.size() && isSpace(a->second[i])) ++i;
    const char c = i < a->second.size() ? a->second[i] : 0;
    return c == '1' || c == 't' || c == 'y' || c == 'T' || c == 'Y';
}

void Element::setAttribute(const std::string& name, const std::string& value) {
    for (auto& a : attributes_)
        if (a.first == name) {
            a.second = value;
            return;
        }
    attributes_.emplace_back(name, value);
}

void Element::setAttribute(const std::string& name, int value) { setAttribute(name, std::to_string(value)); }

void Element::setAttribute(const std::string& name, double value) { setAttribute(name, numberText(value)); }

void Element::removeAttribute(const std::string& name) {
    attributes_.erase(std::remove_if(attributes_.begin(), attributes_.end(),
                                     [&name](const auto& a) { return a.first == name; }),
                      attributes_.end());
}

Element* Element::child(const std::string& name) const {
    for (const auto& c : children_)
        if (c->hasTag(name)) return c.get();
    return nullptr;
}

Element* Element::addChild(const std::string& tag) { return addChild(std::make_unique<Element>(tag)); }

Element* Element::addChild(std::unique_ptr<Element> child) {
    children_.push_back(std::move(child));
    return children_.back().get();
}

Element* Element::prependChild(std::unique_ptr<Element> child) { return insertChild(std::move(child), 0); }

Element* Element::insertChild(std::unique_ptr<Element> child, int index) {
    const auto at = index < 0 || index > numChildren() ? children_.end() : children_.begin() + index;
    return children_.insert(at, std::move(child))->get();
}

std::unique_ptr<Element> Element::takeChild(Element* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it)
        if (it->get() == child) {
            auto taken = std::move(*it);
            children_.erase(it);
            return taken;
        }
    return nullptr;
}

std::string Element::allSubText() const {
    if (isText()) return text_;
    std::string out;
    for (const auto& c : children_) out += c->allSubText();
    return out;
}

std::unique_ptr<Element> Element::copy() const {
    auto e = std::make_unique<Element>(tag_);
    e->text_ = text_;
    e->attributes_ = attributes_;
    for (const auto& c : children_) e->children_.push_back(c->copy());
    return e;
}

int intValue(const std::string& text) {
    size_t i = 0;
    while (i < text.size() && isSpace(text[i])) ++i;
    const bool negative = i < text.size() && text[i] == '-';
    if (negative || (i < text.size() && text[i] == '+')) ++i;
    const std::int64_t limit = negative ? -(std::int64_t) std::numeric_limits<int>::min()
                                        : (std::int64_t) std::numeric_limits<int>::max();
    std::int64_t v = 0;
    for (; i < text.size() && text[i] >= '0' && text[i] <= '9'; ++i)
        v = std::min(limit, v * 10 + (std::int64_t) (text[i] - '0'));
    return (int) (negative ? -v : v);
}

double doubleValue(const std::string& text) {
    size_t i = 0;
    while (i < text.size() && isSpace(text[i])) ++i;
    const bool negative = i < text.size() && text[i] == '-';
    size_t word = i + (i < text.size() && (text[i] == '-' || text[i] == '+') ? 1 : 0);
    auto wordIs = [&text, word](const char* w) {
        for (size_t k = 0; k < 3; ++k)
            if (word + k >= text.size() || std::tolower((unsigned char) text[word + k]) != w[k]) return false;
        return true;
    };
    if (wordIs("nan")) return std::numeric_limits<double>::quiet_NaN();
    if (wordIs("inf")) return negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    const char* start = text.c_str();
    const char* end = start;
    scanDouble(start, &end);
    if (end == start) return 0.0;
    std::istringstream in(std::string(start, end));
    in.imbue(std::locale::classic());
    double v = 0.0;
    in >> v;
    if (!in.fail()) return v;
    if (std::isinf(v) || std::abs(v) == std::numeric_limits<double>::max())
        return v < 0.0 ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    return 0.0;
}

}
