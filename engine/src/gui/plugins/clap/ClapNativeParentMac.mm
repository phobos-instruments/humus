// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#import <AppKit/AppKit.h>

#include <juce_gui_extra/juce_gui_extra.h>

#include "gui/plugins/clap/ClapNativeParent.h"

@interface HumusClapParentView : NSView
@end

@implementation HumusClapParentView
- (BOOL)isFlipped { return YES; }
@end

namespace hum::claphost {

namespace {
class CocoaParent : public NativeParent {
public:
    CocoaParent() {
        NSView* view = [[HumusClapParentView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
        host_.setView((__bridge void*) view);
        [view release];
    }
    ~CocoaParent() override { host_.setView(nullptr); }

    juce::Component& component() override { return host_; }
    clap_window_t window() override {
        clap_window_t w{};
        w.api = CLAP_WINDOW_API_COCOA;
        w.cocoa = host_.getView();
        return w;
    }
    void settle() override {
        juce::Component::SafePointer<juce::Component> safe(&host_);
        juce::MessageManager::callAsync([safe] {
            auto* nvc = dynamic_cast<juce::NSViewComponent*>(safe.getComponent());
            if (nvc == nullptr) return;
            NSView* view = (__bridge NSView*) nvc->getView();
            const NSRect frame = view.frame;
            [view setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height + 1)];
            [view setFrame:frame];
        });
    }

private:
    juce::NSViewComponent host_;
};
}

std::unique_ptr<NativeParent> NativeParent::make() { return std::make_unique<CocoaParent>(); }

}
