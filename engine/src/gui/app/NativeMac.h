// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace hum {

#if JUCE_MAC
void beginAppKeepAwake();
void becomeWindowlessProcess();
void disableAutomaticWindowTabbing();
juce::String nativeWindowTitle(juce::ComponentPeer* peer);
bool debugCaptureOwnWindow(juce::ComponentPeer* peer, const juce::File& out);
juce::String debugDumpPeerSubviews(juce::ComponentPeer* peer);

inline void syncNativeWindowTitle(juce::ComponentPeer& peer, const juce::String& title) {
    if (nativeWindowTitle(&peer) != title) peer.setTitle(title);
}
#else
inline void beginAppKeepAwake() {}
inline void becomeWindowlessProcess() {}
inline void disableAutomaticWindowTabbing() {}
inline bool debugCaptureOwnWindow(juce::ComponentPeer*, const juce::File&) { return false; }
inline juce::String debugDumpPeerSubviews(juce::ComponentPeer*) { return {}; }
inline void syncNativeWindowTitle(juce::ComponentPeer&, const juce::String&) {}
#endif

}
