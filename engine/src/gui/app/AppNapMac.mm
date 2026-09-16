// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
// macOS App Nap opt-out. A backgrounded app gets its timers coalesced to a
// crawl - which is not just a sluggish GUI: the modulation poll, live MIDI
// routing and the OSC pump all ride the message thread, so a napping Humus
// audibly stutters the moment another window takes focus. A live instrument
// is user-initiated work for as long as it runs.
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

namespace hum {

void beginAppKeepAwake() {
    static id token = nil;
    if (token != nil) return;
    token = [[NSProcessInfo processInfo]
        beginActivityWithOptions:(NSActivityUserInitiatedAllowingIdleSystemSleep
                                  | NSActivityLatencyCritical)
                          reason:@"live audio engine"];
    [token retain];
}

// Prohibited is the only policy under which a probed plugin's window does not
// come up. Not for the bridge child, which has to show a hosted editor.
void becomeWindowlessProcess() {
    [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
}

}  // namespace hum
