// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <juce_graphics/juce_graphics.h>

#include "gui/editor/Geometry.h"

namespace hum {

template <class T>
juce::Rectangle<T> toJuce(const BasicRect<T>& r) { return {r.x, r.y, r.w, r.h}; }

template <class T>
BasicRect<T> rectOf(const juce::Rectangle<T>& r) { return {r.getX(), r.getY(), r.getWidth(), r.getHeight()}; }

template <class T>
BasicPoint<T> pointOf(const juce::Point<T>& p) { return {p.x, p.y}; }

}
