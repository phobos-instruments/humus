// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/juce/JuceBrickViews.h"

#include "gui/editor/juce/JuceRichView.h"

namespace hum {

std::unique_ptr<RichView> JuceBrickViews::rich(const LayoutSpec& spec, const LayoutSpec::Control& control,
                                               RichOwner& owner) {
    if (auto view = makeJucePlayerBrick(parent_, host_, spec, control, owner)) return view;
    if (auto view = makeJuceReadoutBrick(parent_, host_, spec, control, owner)) return view;
    return makeJucePickerBrick(parent_, host_, spec, control, owner);
}

}
