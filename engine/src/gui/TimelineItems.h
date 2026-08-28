#pragma once
#include <tuple>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum::timeline {

struct ItemRef {
    enum class Kind { Clip, Box };
    Kind kind = Kind::Clip;
    int row = -1;
    int key = 0;
    bool operator<(const ItemRef& o) const {
        return std::tie(kind, row, key) < std::tie(o.kind, o.row, o.key);
    }
    bool operator==(const ItemRef& o) const {
        return kind == o.kind && row == o.row && key == o.key;
    }
};

class ItemKind {
public:
    virtual ~ItemKind() = default;
    virtual ItemRef::Kind kind() const = 0;
    virtual std::vector<ItemRef> all(int row) const = 0;
    virtual juce::Rectangle<int> bounds(const ItemRef&) const = 0;
    virtual bool alive(const ItemRef&) const = 0;
    virtual void remove(const std::vector<ItemRef>&) = 0;
    virtual void duplicateAfter(const std::vector<ItemRef>&) = 0;
    virtual int merge(const std::vector<ItemRef>&) = 0;
};

}
