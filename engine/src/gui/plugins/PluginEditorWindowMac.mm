// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#import <AppKit/AppKit.h>
#include <juce_gui_basics/juce_gui_basics.h>

static void clearTrackingAreasDeep(NSView* v) {
    for (NSTrackingArea* ta in [[v trackingAreas] copy])
        [v removeTrackingArea:ta];
    for (NSView* sub in [v subviews])
        clearTrackingAreasDeep(sub);
}

static NSWindow* nsWindowFor(juce::ComponentPeer* peer) {
    if (!peer) return nil;
    NSView* view = (__bridge NSView*) peer->getNativeHandle();
    return view ? view.window : nil;
}

namespace hum {

void pluginWindowClearTrackingAreas(juce::ComponentPeer* peer) {
    NSView* root = (__bridge NSView*) (peer ? peer->getNativeHandle() : nullptr);
    if (root) clearTrackingAreasDeep(root);
}

void windowKeepFullscreenLocal(juce::ComponentPeer* peer) {
    NSWindow* w = nsWindowFor(peer);
    if (w == nil) return;
    w.collectionBehavior = (w.collectionBehavior
                            & ~(NSUInteger) NSWindowCollectionBehaviorFullScreenPrimary)
                           | NSWindowCollectionBehaviorFullScreenNone;
}

void pluginWindowAttachToMain(juce::ComponentPeer* pluginPeer,
                              juce::ComponentPeer* mainPeer) {
    NSWindow* plugin = nsWindowFor(pluginPeer);
    NSWindow* main   = nsWindowFor(mainPeer);
    if (plugin && main && plugin != main)
        [main addChildWindow:plugin ordered:NSWindowAbove];
}

void pluginWindowDetachFromMain(juce::ComponentPeer* pluginPeer,
                                juce::ComponentPeer* mainPeer) {
    NSWindow* plugin = nsWindowFor(pluginPeer);
    NSWindow* main   = nsWindowFor(mainPeer);
    if (plugin && main)
        [main removeChildWindow:plugin];
}

// macOS may adopt a newly opened titled window as a TAB of the frontmost
// window ("prefer tabs" - the DEFAULT in full screen): a floated plugin
// editor merges into the main window and the title bar takes the plugin's
// name. Opt the whole app out, like every DAW does. Call before any window.
void disableAutomaticWindowTabbing() {
    NSWindow.allowsAutomaticWindowTabbing = NO;
}

// The real NSWindow title. JUCE's Component::getName() only knows what JUCE
// set - a native rename (window tabbing, a plugin editor retitling its host
// window) is invisible to it, so the title guard must read the truth.
juce::String nativeWindowTitle(juce::ComponentPeer* peer) {
    NSWindow* w = nsWindowFor(peer);
    return w ? juce::String::fromUTF8([[w title] UTF8String]) : juce::String();
}

// Remove all window chrome so the plugin UI appears as a flush panel.
void pluginWindowMakeBorderless(juce::ComponentPeer* peer) {
    NSWindow* w = nsWindowFor(peer);
    if (!w) return;
    [w setStyleMask:NSWindowStyleMaskBorderless];
    [w setHasShadow:YES];           // subtle shadow keeps it visually distinct
    [w setMovableByWindowBackground:NO];
}

} // namespace hum
