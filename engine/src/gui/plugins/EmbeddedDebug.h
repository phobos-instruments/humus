// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

#if JUCE_MAC
bool embeddedDebugDumpNSView(juce::Component& editor, const juce::File& out);
juce::String embeddedDebugViewTree(juce::Component& editor);
juce::String embeddedDebugSuperviews(juce::Component& editor);
juce::String embeddedDebugWindowChildren(juce::ComponentPeer& peer);
juce::Rectangle<int> embeddedEffectiveFrame(juce::Component& editor);
bool embeddedClipIsMasking(juce::Component& editor);
bool embeddedEditorUsesOwnWindow(juce::ComponentPeer& peer, int natW, int natH);
bool embeddedViewIsRemote(juce::Component& editor);
bool embeddedViewDrawsContent(juce::Component& editor);
bool embeddedViewResistsZoom(juce::Component& editor);
bool embeddedHitLocal(juce::Component& editor, juce::ComponentPeer& peer,
                      juce::Point<float> pointInPeer, juce::Point<float>& local);
void* embeddedProbeViewMake(int w, int h);
void embeddedProbeViewRelease(void* view);
#else
inline bool embeddedDebugDumpNSView(juce::Component&, const juce::File&) { return false; }
inline juce::String embeddedDebugViewTree(juce::Component&) { return {}; }
inline juce::String embeddedDebugSuperviews(juce::Component&) { return {}; }
inline juce::String embeddedDebugWindowChildren(juce::ComponentPeer&) { return {}; }
inline juce::Rectangle<int> embeddedEffectiveFrame(juce::Component&) { return {}; }
inline bool embeddedClipIsMasking(juce::Component&) { return true; }
inline bool embeddedEditorUsesOwnWindow(juce::ComponentPeer&, int, int) { return false; }
inline bool embeddedViewIsRemote(juce::Component&) { return false; }
inline bool embeddedViewDrawsContent(juce::Component&) { return true; }
inline bool embeddedViewResistsZoom(juce::Component&) { return false; }
inline bool embeddedHitLocal(juce::Component&, juce::ComponentPeer&, juce::Point<float>,
                             juce::Point<float>&) { return false; }
inline void* embeddedProbeViewMake(int, int) { return nullptr; }
inline void embeddedProbeViewRelease(void*) {}
#endif

}
