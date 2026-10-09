// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#import <Cocoa/Cocoa.h>

#include "gui/app/MenuBarTitles.h"

namespace hum {

void hostMenuBarWithoutDockIcon() {
    [NSApplication sharedApplication];
    if ([NSApp activationPolicy] != NSApplicationActivationPolicyRegular)
        [NSApp setActivationPolicy: NSApplicationActivationPolicyProhibited];
}

std::vector<MenuBarTitle> menuBarTitles() {
    std::vector<MenuBarTitle> out;
    NSMenu* bar = [NSApp mainMenu];
    for (NSInteger i = 1; i < [bar numberOfItems]; ++i) {
        NSMenuItem* item = [bar itemAtIndex: i];
        const char* itemTitle = [[item title] UTF8String];
        const char* menuTitle = [[[item submenu] title] UTF8String];
        out.push_back({itemTitle != nullptr ? itemTitle : "", menuTitle != nullptr ? menuTitle : ""});
    }
    return out;
}

}
