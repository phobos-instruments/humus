#include "gui/DefaultPatchHandler.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/AppSettings.h"

#if JUCE_MAC
#include <CoreServices/CoreServices.h>
#include "gui/Localisation.h"
#endif

namespace hum {

#if JUCE_MAC

static constexpr const char* kPatchUti = "com.totel.humus.patch";
static constexpr const char* kAskedKey = "defaultHandler.hum.asked";

static juce::String currentHandler() {
    juce::String out;
    if (auto uti = juce::String(kPatchUti).toCFString()) {
        if (auto* h = LSCopyDefaultRoleHandlerForContentType(uti, kLSRolesEditor)) {
            out = juce::String::fromCFString(h);
            CFRelease(h);
        }
        CFRelease(uti);
    }
    return out;
}

static juce::String ourBundleId() {
    juce::String out;
    if (auto* main = CFBundleGetMainBundle())
        if (auto id = CFBundleGetIdentifier(main))
            out = juce::String::fromCFString(id);
    return out;
}

static bool claimType(const juce::String& bundleId) {
    bool ok = false;
    if (auto uti = juce::String(kPatchUti).toCFString()) {
        if (auto handler = bundleId.toCFString()) {
            ok = LSSetDefaultRoleHandlerForContentType(uti, kLSRolesEditor, handler) == noErr;
            CFRelease(handler);
        }
        CFRelease(uti);
    }
    return ok;
}

#endif

void offerDefaultPatchHandlerOnce() {
#if JUCE_MAC
    auto& settings = AppSettings::instance();
    if (settings.getInt(kAskedKey, 0) != 0) return;

    const auto us = ourBundleId();
    if (us.isEmpty()) return;

    if (currentHandler().equalsIgnoreCase(us)) {
        settings.set(kAskedKey, 1);
        return;
    }

    auto* aw = new juce::AlertWindow(
        tr("default-patch-handler.open-hum-patches", "Open .hum patches with Humus?"),
        tr("default-patch-handler.double-click",
           "Humus can open .hum patches when you double-click them.\n\n"
           "You can change this later in Finder: Get Info on a patch, then Open With."),
        juce::MessageBoxIconType::QuestionIcon);
    aw->addButton(tr("default-patch-handler.use-humus", "Use Humus"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton(tr("default-patch-handler.not-now", "Not Now"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create([us](int r) {
        AppSettings::instance().set(kAskedKey, 1);
        if (r == 1) claimType(us);
    }), true);
#endif
}

}
