// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/tracks/FileDragImage.h"

#if JUCE_MAC

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

namespace hum {
namespace {

NSImage* toNSImage(const juce::Image& img, float scale) {
    if (!img.isValid()) return nil;
    juce::MemoryOutputStream mo;
    if (!juce::PNGImageFormat().writeImageToStream(img, mo)) return nil;
    NSData* data = [NSData dataWithBytes:mo.getData() length:mo.getDataSize()];
    NSImage* out = [[[NSImage alloc] initWithData:data] autorelease];
    [out setSize:NSMakeSize(img.getWidth() / scale, img.getHeight() / scale)];
    return out;
}

void* ivar(id self, const char* name) {
    void* out = nullptr;
    object_getInstanceVariable(self, name, &out);
    return out;
}

NSDragOperation opMask(id, SEL, NSDraggingSession*, NSDraggingContext) {
    return NSDragOperationCopy;
}

void ended(id self, SEL, NSDraggingSession*, NSPoint p, NSDragOperation) {
    // A view gets no mouse-up of its own when a dragging session ends, and JUCE
    // leaves the button latched down without one (juce_Windowing_mac.mm does the same).
    if (auto* view = (NSView*) ivar(self, "view"))
        if (auto* raw = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp,
                                                CGPointMake(p.x, p.y), kCGMouseButtonLeft)) {
            if (id e = [NSEvent eventWithCGEvent:raw]) [view mouseUp:e];
            CFRelease(raw);
        }
    if (auto* cb = (std::function<void()>*) ivar(self, "done")) {
        (*cb)();
        delete cb;
        object_setInstanceVariable(self, "done", nullptr);
    }
}

// Named per translation unit: the app and both plugin formats can be loaded
// into one host, and a shared class name makes the runtime pick one and warn.
Class sourceClass() {
    static Class cls = [] {
        juce::String name("HumusFileDragSource_");
        name << juce::String::toHexString((juce::pointer_sized_int) &toNSImage);
        Class c = objc_allocateClassPair([NSObject class], name.toRawUTF8(), 0);
        class_addProtocol(c, @protocol(NSDraggingSource));
        class_addIvar(c, "view", sizeof(void*), 3, "^v");
        class_addIvar(c, "done", sizeof(void*), 3, "^v");
        class_addMethod(c, @selector(draggingSession:sourceOperationMaskForDraggingContext:),
                        (IMP) opMask, "L@:@L");
        class_addMethod(c, @selector(draggingSession:endedAtPoint:operation:),
                        (IMP) ended, "v@:@{CGPoint=dd}L");
        objc_registerClassPair(c);
        return c;
    }();
    return cls;
}

}  // namespace

bool dragFilesWithImage(const juce::StringArray& files, juce::Component* source,
                        const juce::Image& chip, std::function<void()> onDone) {
    if (files.isEmpty() || source == nullptr || !chip.isValid()) return false;
    auto* view = (NSView*) source->getWindowHandle();
    if (view == nil) return false;
    NSEvent* event = [[view window] currentEvent];
    if (event == nil) return false;
    NSImage* image = toNSImage(chip, 2.0f);
    if (image == nil) return false;

    NSMutableArray* items = [[[NSMutableArray alloc] init] autorelease];
    const auto at = [event locationInWindow];
    for (int i = 0; i < files.size(); ++i) {
        NSString* path = [NSString stringWithUTF8String:files[i].toRawUTF8()];
        auto* item = [[[NSDraggingItem alloc]
            initWithPasteboardWriter:[NSURL fileURLWithPath:path]] autorelease];
        // The grabbed clip leads the stack; the rest keep the system's file icon.
        NSImage* art = i == 0 ? image : [[NSWorkspace sharedWorkspace] iconForFile:path];
        const auto size = [art size];
        [item setDraggingFrame:[view convertRect:NSMakeRect(at.x - size.width * 0.5,
                                                            at.y - size.height * 0.5,
                                                            size.width, size.height)
                                        fromView:nil]
                      contents:art];
        [items addObject:item];
    }

    id helper = [[sourceClass() new] autorelease];
    object_setInstanceVariable(helper, "view", (void*) view);
    if (onDone != nullptr)
        object_setInstanceVariable(helper, "done", new std::function<void()>(std::move(onDone)));
    if (auto* session = [view beginDraggingSessionWithItems:items event:event source:helper]) {
        session.animatesToStartingPositionsOnCancelOrFail = YES;
        session.draggingFormation = NSDraggingFormationNone;
        return true;
    }
    return false;
}

}  // namespace hum

#endif
