// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hum::xml {

class Element;

class ChildRange {
public:
    using Owned = std::vector<std::unique_ptr<Element>>;
    class iterator {
    public:
        explicit iterator(Owned::const_iterator it) : it_(it) {}
        Element* operator*() const { return it_->get(); }
        iterator& operator++() {
            ++it_;
            return *this;
        }
        bool operator!=(const iterator& o) const { return it_ != o.it_; }

    private:
        Owned::const_iterator it_;
    };
    explicit ChildRange(const Owned& owned) : owned_(owned) {}
    iterator begin() const { return iterator(owned_.begin()); }
    iterator end() const { return iterator(owned_.end()); }

private:
    const Owned& owned_;
};

class Element {
public:
    explicit Element(std::string tag) : tag_(std::move(tag)) {}
    static std::unique_ptr<Element> textNode(std::string text);

    const std::string& tag() const { return tag_; }
    bool isText() const { return tag_.empty(); }
    const std::string& text() const { return text_; }
    bool hasTag(const std::string& name) const;

    bool hasAttribute(const std::string& name) const { return find(name) != nullptr; }
    const std::string& attribute(const std::string& name) const;
    std::string attribute(const std::string& name, const std::string& fallback) const;
    int intAttribute(const std::string& name, int fallback = 0) const;
    double doubleAttribute(const std::string& name, double fallback = 0.0) const;
    bool boolAttribute(const std::string& name, bool fallback = false) const;
    void setAttribute(const std::string& name, const std::string& value);
    void setAttribute(const std::string& name, const char* value) { setAttribute(name, std::string(value)); }
    void setAttribute(const std::string& name, int value);
    void setAttribute(const std::string& name, double value);
    void removeAttribute(const std::string& name);
    int numAttributes() const { return (int) attributes_.size(); }
    const std::string& attributeName(int i) const { return attributes_[(size_t) i].first; }
    const std::string& attributeValue(int i) const { return attributes_[(size_t) i].second; }

    ChildRange children() const { return ChildRange(children_); }
    int numChildren() const { return (int) children_.size(); }
    Element* childAt(int i) const { return i >= 0 && i < numChildren() ? children_[(size_t) i].get() : nullptr; }
    Element* child(const std::string& name) const;
    Element* addChild(const std::string& tag);
    Element* addChild(std::unique_ptr<Element> child);
    Element* prependChild(std::unique_ptr<Element> child);
    Element* insertChild(std::unique_ptr<Element> child, int index);
    void addText(const std::string& text) { addChild(textNode(text)); }
    std::unique_ptr<Element> takeChild(Element* child);
    void removeChild(Element* child) { takeChild(child); }
    void deleteChildren() { children_.clear(); }

    std::string allSubText() const;
    std::unique_ptr<Element> copy() const;

private:
    const std::pair<std::string, std::string>* find(const std::string& name) const;

    std::string tag_, text_;
    std::vector<std::pair<std::string, std::string>> attributes_;
    std::vector<std::unique_ptr<Element>> children_;
};

struct Format {
    bool header = true;
    const char* newLine = "\r\n";
    int lineWrap = 60;
    Format singleLine() const {
        auto f = *this;
        f.newLine = nullptr;
        return f;
    }
};

std::unique_ptr<Element> parse(const std::string& text);
std::unique_ptr<Element> parseFile(const std::string& path);
std::string write(const Element& root, const Format& format = {});
bool writeFile(const std::string& path, const Element& root, const Format& format = {});

std::string numberText(double value);
int intValue(const std::string& text);
double doubleValue(const std::string& text);

}
