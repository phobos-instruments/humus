// JUCE's software renderer cannot clip a native NSView, so the plugin view is re-parented into a masks-to-bounds container.
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <ImageIO/ImageIO.h>

#include <cstdlib>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

@interface HumusEmbeddedClipView : NSView
@end

@implementation HumusEmbeddedClipView
- (BOOL)isFlipped { return YES; }
- (NSView*)hitTest:(NSPoint)point {
    NSView* v = [super hitTest:point];
    return v == self ? nil : v;
}
@end

// Under the plugin view only: a whole-container background blanked every JUCE-painted box around the editor.
@interface HumusEmbedBackplate : NSView
@end

@implementation HumusEmbedBackplate
- (NSView*)hitTest:(NSPoint)point { (void) point; return nil; }
@end

namespace hum {

static NSView* findPluginNSView(juce::Component& c) {
    if (auto* nvc = dynamic_cast<juce::NSViewComponent*>(&c))
        return (__bridge NSView*) nvc->getView();
    for (int i = 0; i < c.getNumChildComponents(); ++i)
        if (auto* child = c.getChildComponent(i))
            if (NSView* v = findPluginNSView(*child)) return v;
    return nullptr;
}

// Zoom is AppKit's frame-vs-bounds transform: bounds span the unscaled source, frame the scaled on-screen rect.
bool embeddedClipUpdate(juce::Component& editor, juce::ComponentPeer& peer,
                        juce::Rectangle<int> clipRegionInPeer,
                        juce::Point<int> editorOriginInPeer,
                        float scale, juce::Colour background, void*& handle) {
    if (std::getenv("HUM_EMBED_NOWRAP") != nullptr) return false;
    NSView* plugin = findPluginNSView(editor);
    if (!plugin) return false;
    NSView* peerView = (__bridge NSView*) peer.getNativeHandle();
    if (!peerView) return true;

    auto* clip = (HumusEmbeddedClipView*) handle;
    if (clip && plugin.superview != clip) {
        [clip removeFromSuperview];
        [clip release];
        clip = nil;
        handle = nullptr;
    }
    // Never re-parent across windows: addSubview sends _setWindow: to the plugin view, which some plugins crash in.
    if (!clip && (peerView.window == nil || plugin.window != peerView.window))
        return true;
    if (!clip) {
        clip = [[HumusEmbeddedClipView alloc] initWithFrame:NSZeroRect];
        clip.wantsLayer = YES;
        // macOS 14+ re-syncs masksToBounds from clipsToBounds on every layout pass, clobbering a bare masksToBounds=YES.
        if (@available(macOS 14.0, *)) clip.clipsToBounds = YES;
        clip.layer.masksToBounds = YES;
        [peerView addSubview:clip];
        [clip addSubview:plugin];
        handle = clip;
    }
    if (clip.hidden) [clip setHidden:NO];
    {
        HumusEmbedBackplate* plate = nil;
        for (NSView* s in clip.subviews)
            if ([s isKindOfClass:[HumusEmbedBackplate class]]) {
                plate = (HumusEmbedBackplate*) s;
                break;
            }
        if (!plate) {
            plate = [[HumusEmbedBackplate alloc] initWithFrame:NSZeroRect];
            plate.wantsLayer = YES;
            CGColorRef bg = CGColorCreateSRGB(background.getFloatRed(),
                                              background.getFloatGreen(),
                                              background.getFloatBlue(), 1.0);
            plate.layer.backgroundColor = bg;
            CGColorRelease(bg);
            [clip addSubview:plate positioned:NSWindowBelow relativeTo:plugin];
            [plate release];
        }
        // editorOriginInPeer, not plugin.frame: the movement watcher can run before JUCE has moved the plugin view.
        const NSRect want = NSMakeRect(editorOriginInPeer.x, editorOriginInPeer.y,
                                       plugin.frame.size.width,
                                       plugin.frame.size.height);
        if (!NSEqualRects(plate.frame, want)) plate.frame = want;
    }
    const CGFloat s = scale > 0.01f ? (CGFloat) scale : 1.0;
    const NSRect r = NSMakeRect(clipRegionInPeer.getX(), clipRegionInPeer.getY(),
                                clipRegionInPeer.getWidth(), clipRegionInPeer.getHeight());
    // Re-poking identical frame/bounds at 30 Hz makes the layer-backed surface shimmer.
    if (!NSEqualRects(clip.frame, r)) [clip setFrame:r];
    const NSSize bs = NSMakeSize(r.size.width / s, r.size.height / s);
    if (!NSEqualSizes(clip.bounds.size, bs)) [clip setBoundsSize:bs];
    const NSPoint bo = NSMakePoint(
        editorOriginInPeer.x + (r.origin.x - editorOriginInPeer.x) / s,
        editorOriginInPeer.y + (r.origin.y - editorOriginInPeer.y) / s);
    if (!NSEqualPoints(clip.bounds.origin, bo)) [clip setBoundsOrigin:bo];
    return true;
}

bool embeddedClipHide(void*& handle) {
    auto* clip = (HumusEmbeddedClipView*) handle;
    if (!clip) return false;
    if (!clip.hidden) [clip setHidden:YES];
    return true;
}

// JUCE teardown expects the plugin view as a direct peer child; across windows it stays in the container.
void embeddedClipRemove(juce::Component& editor, juce::ComponentPeer* peer, void*& handle) {
    auto* clip = (HumusEmbeddedClipView*) handle;
    if (!clip) return;
    NSView* plugin = findPluginNSView(editor);
    NSView* peerView = peer ? (__bridge NSView*) peer->getNativeHandle() : nil;
    if (plugin && plugin.superview == clip && peerView
        && plugin.window == peerView.window)
        [peerView addSubview:plugin];
    [clip removeFromSuperview];
    [clip release];
    handle = nullptr;
}

// Out-of-process AU views (NSRemoteView) blank out when re-parented or bounds-zoomed.
static bool isRemoteHostedView(NSView* v) {
    NSString* cls = NSStringFromClass([v class]);
    if ([cls containsString:@"NSRemoteView"] || [cls containsString:@"AUv2ContainerView"]
        || [cls containsString:@"RemoteContainerView"])
        return true;
    for (NSView* s in v.subviews)
        if (isRemoteHostedView(s)) return true;
    return false;
}

bool embeddedViewIsRemote(juce::Component& editor) {
    NSView* v = findPluginNSView(editor);
    return v != nullptr && isRemoteHostedView(v);
}

static void dumpViewTree(NSView* v, int depth, juce::String& into) {
    into << juce::String::repeatedString("  ", depth)
         << [NSStringFromClass([v class]) UTF8String]
         << (v.layer ? " (layer)" : "")
         << " " << (int) v.frame.size.width << "x" << (int) v.frame.size.height << "\n";
    for (NSView* s in v.subviews) dumpViewTree(s, depth + 1, into);
}

juce::String embeddedDebugViewTree(juce::Component& editor) {
    NSView* v = findPluginNSView(editor);
    if (!v) return "<no NSView>";
    juce::String s;
    dumpViewTree(v, 0, s);
    return s;
}

juce::Rectangle<int> embeddedEffectiveFrame(juce::Component& editor) {
    NSView* v = findPluginNSView(editor);
    if (!v) return {};
    NSView* painted = [v.superview isKindOfClass:[HumusEmbeddedClipView class]]
                          ? v.superview : v;
    if (painted.hiddenOrHasHiddenAncestor) return {};
    const NSRect f = painted.frame;
    return {(int) f.origin.x, (int) f.origin.y,
            (int) f.size.width, (int) f.size.height};
}

// Probe only after the run loop has spun: AppKit re-syncs masksToBounds on layout passes.
bool embeddedClipIsMasking(juce::Component& editor) {
    NSView* v = findPluginNSView(editor);
    if (!v || ![v.superview isKindOfClass:[HumusEmbeddedClipView class]]) return false;
    return v.superview.layer != nil && v.superview.layer.masksToBounds;
}

// Child windows float above every view, unclippable; the size match attributes one to this editor when several boxes are open.
bool embeddedEditorUsesOwnWindow(juce::ComponentPeer& peer, int natW, int natH) {
    NSView* peerView = (__bridge NSView*) peer.getNativeHandle();
    if (!peerView || !peerView.window) return false;
    for (NSWindow* w in peerView.window.childWindows) {
        NSString* cls = NSStringFromClass([w class]);
        if ([cls hasPrefix:@"NS"] || [cls containsString:@"JUCE"]) continue;
        if (std::abs((int) w.frame.size.width - natW) <= 2
            && std::abs((int) w.frame.size.height - natH) <= 2)
            return true;
    }
    return false;
}

juce::String embeddedDebugWindowChildren(juce::ComponentPeer& peer) {
    NSView* peerView = (__bridge NSView*) peer.getNativeHandle();
    if (!peerView || !peerView.window) return "<no window>";
    juce::String s;
    for (NSView* v in peerView.superview.subviews) {
        const NSRect f = v.frame;
        s << "  sibling " << [NSStringFromClass([v class]) UTF8String]
          << " " << (int) f.origin.x << "," << (int) f.origin.y
          << " " << (int) f.size.width << "x" << (int) f.size.height
          << (v.hidden ? " hidden" : "") << "\n";
    }
    for (NSWindow* w in peerView.window.childWindows)
        s << "  childWindow " << [NSStringFromClass([w class]) UTF8String]
          << " " << (int) w.frame.size.width << "x" << (int) w.frame.size.height << "\n";
    return s;
}

juce::String embeddedDebugSuperviews(juce::Component& editor) {
    NSView* v = findPluginNSView(editor);
    if (!v) return "<no NSView>";
    juce::String s;
    for (NSView* p = v; p != nil; p = p.superview)
        s << [NSStringFromClass([p class]) UTF8String]
          << " frame=" << (int) p.frame.origin.x << "," << (int) p.frame.origin.y
          << " " << (int) p.frame.size.width << "x" << (int) p.frame.size.height
          << " bounds=" << (int) p.bounds.origin.x << "," << (int) p.bounds.origin.y
          << " " << (int) p.bounds.size.width << "x" << (int) p.bounds.size.height
          << "\n";
    return s;
}

juce::String debugDumpPeerSubviews(juce::ComponentPeer* peer) {
    if (peer == nullptr) return "<no peer>";
    NSView* pv = (__bridge NSView*) peer->getNativeHandle();
    if (!pv) return "<no view>";
    juce::String s;
    for (NSView* v in pv.subviews) {
        s << "  peerChild " << [NSStringFromClass([v class]) UTF8String]
          << " frame=" << (int) v.frame.origin.x << "," << (int) v.frame.origin.y
          << " " << (int) v.frame.size.width << "x" << (int) v.frame.size.height
          << " bounds=" << (int) v.bounds.origin.x << "," << (int) v.bounds.origin.y
          << (v.hidden ? " HIDDEN" : "") << "\n";
        for (NSView* c in v.subviews)
            s << "    sub " << [NSStringFromClass([c class]) UTF8String]
              << " frame=" << (int) c.frame.origin.x << "," << (int) c.frame.origin.y
              << " " << (int) c.frame.size.width << "x" << (int) c.frame.size.height
              << (c.hidden ? " HIDDEN" : "") << "\n";
    }
    return s;
}

// Own-window capture needs no screen-recording permission, so a headless session can see the composite.
bool debugCaptureOwnWindow(juce::ComponentPeer* peer, const juce::File& out) {
    if (peer == nullptr) return false;
    NSView* v = (__bridge NSView*) peer->getNativeHandle();
    if (!v || !v.window) return false;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CGImageRef img = CGWindowListCreateImage(
        CGRectNull, kCGWindowListOptionIncludingWindow,
        (CGWindowID) [v.window windowNumber], kCGWindowImageBoundsIgnoreFraming);
#pragma clang diagnostic pop
    if (img == nullptr) return false;
    NSURL* url = [NSURL fileURLWithPath:
        [NSString stringWithUTF8String:out.getFullPathName().toRawUTF8()]];
    CGImageDestinationRef dst = CGImageDestinationCreateWithURL(
        (__bridge CFURLRef) url, (__bridge CFStringRef) @"public.png", 1, nullptr);
    bool ok = false;
    if (dst != nullptr) {
        CGImageDestinationAddImage(dst, img, nullptr);
        ok = CGImageDestinationFinalize(dst);
        CFRelease(dst);
    }
    CGImageRelease(img);
    return ok;
}

// GPU-layer content does not render through cacheDisplayInRect, so a uniform result may just be gpu-only.
bool embeddedDebugDumpNSView(juce::Component& editor, const juce::File& out) {
    NSView* v = findPluginNSView(editor);
    if (!v) return false;
    NSBitmapImageRep* rep = [v bitmapImageRepForCachingDisplayInRect:[v bounds]];
    if (!rep) return false;
    [v cacheDisplayInRect:[v bounds] toBitmapImageRep:rep];
    NSData* png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    [png writeToFile:[NSString stringWithUTF8String:out.getFullPathName().toRawUTF8()]
          atomically:YES];
    const NSInteger w = rep.pixelsWide, h = rep.pixelsHigh;
    if (w < 4 || h < 4) return false;
    NSColor* first = [rep colorAtX:2 y:2];
    for (NSInteger y = 2; y < h; y += h / 16 + 1)
        for (NSInteger x = 2; x < w; x += w / 16 + 1)
            if (![[rep colorAtX:x y:y] isEqual:first]) return true;
    return false;
}

}  // namespace hum
