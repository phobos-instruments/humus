// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <string>
#include <vector>

#include "gui/editor/views/BrickView.h"

namespace hum {

class ChoiceButtonsView : public virtual BrickView {
public:
    virtual void showIndex(int index) = 0;
    virtual int shownIndex() const = 0;
    virtual int optionCount() const = 0;

    std::function<void(int index)> onChoose;
};

struct ComboItem {
    int id = 0;
    std::string text;
};

class ComboView : public virtual BrickView {
public:
    virtual void setItems(const std::vector<ComboItem>& items) = 0;
    virtual void showSelectedId(int id) = 0;
    virtual int shownSelectedId() const = 0;
    virtual std::string shownText() const = 0;
    virtual void showTip(const std::string& tip) = 0;

    std::function<void(int id)> onPick;
    std::function<void()> onOpen;
    std::function<void(bool forward)> onStep;
};

}
