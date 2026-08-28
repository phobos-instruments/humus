#pragma once
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/BridgeWorker.h"
#include "gui/LookAndFeel.h"
#include "gui/OffscreenEditorHost.h"

namespace hum {

class BridgeWorkerEditor {
public:
    explicit BridgeWorkerEditor(BridgeWorker& worker) : worker_(worker) {
        worker_.onCreateEditor  = [this] { create(); };
        worker_.onDestroyEditor = [this] { destroy(); };
        worker_.onSetFloating   = [this](bool f) { setFloating(f); };
    }
    ~BridgeWorkerEditor() { destroy(); }

private:
    void create() {
        if (offscreen_ != nullptr || floatWin_ != nullptr) { reportSize(); return; }
        auto* inst = worker_.pluginInstance();
        if (inst == nullptr || !inst->hasEditor()) { worker_.sendEditorCreated(0, 0, 0); return; }
        std::unique_ptr<juce::AudioProcessorEditor> ed(inst->createEditorIfNeeded());
        if (ed == nullptr) { worker_.sendEditorCreated(0, 0, 0); return; }
        offscreen_ = std::make_unique<OffscreenEditorHost>(std::move(ed));
        offscreen_->onNaturalSizeChanged = [this] { reportSize(); };
        report();
    }

    void destroy() {
        floatWin_.reset();
        offscreen_.reset();
    }

    void setFloating(bool floating) {
        if (floating) {
            if (floatWin_ != nullptr) return;
            std::unique_ptr<juce::AudioProcessorEditor> ed;
            if (offscreen_ != nullptr) ed = offscreen_->releaseEditor();
            offscreen_.reset();
            if (ed == nullptr) {
                auto* inst = worker_.pluginInstance();
                if (inst != nullptr) ed.reset(inst->createEditorIfNeeded());
            }
            if (ed == nullptr) { worker_.sendFloatClosed(); return; }
            floatWin_ = std::make_unique<FloatWindow>(std::move(ed), [this] {
                returnToEmbed();
                worker_.sendFloatClosed();
            });
        } else {
            returnToEmbed();
        }
    }

    void returnToEmbed() {
        if (floatWin_ == nullptr) return;
        auto ed = floatWin_->release();
        floatWin_.reset();
        if (ed != nullptr) {
            offscreen_ = std::make_unique<OffscreenEditorHost>(std::move(ed));
            offscreen_->onNaturalSizeChanged = [this] { reportSize(); };
            report();
        }
    }

    void report() {
        if (offscreen_ == nullptr) return;
        const auto xid = (unsigned long) reinterpret_cast<uintptr_t>(offscreen_->getWindowHandle());
        worker_.sendEditorCreated(xid, offscreen_->naturalWidth(), offscreen_->naturalHeight());
    }
    void reportSize() {
        if (offscreen_ != nullptr)
            worker_.sendSizeChanged(offscreen_->naturalWidth(), offscreen_->naturalHeight());
    }

    struct FloatWindow : juce::DocumentWindow {
        FloatWindow(std::unique_ptr<juce::AudioProcessorEditor> ed, std::function<void()> onClose)
            : juce::DocumentWindow("Plugin", Palette::background, juce::DocumentWindow::allButtons),
              editor_(std::move(ed)), onClose_(std::move(onClose)) {
            setUsingNativeTitleBar(true);
            setContentNonOwned(editor_.get(), true);
            setResizable(false, false);
            centreWithSize(juce::jmax(80, editor_->getWidth()), juce::jmax(60, editor_->getHeight()));
            setVisible(true);
        }
        ~FloatWindow() override { clearContentComponent(); }
        std::unique_ptr<juce::AudioProcessorEditor> release() {
            clearContentComponent();
            return std::move(editor_);
        }
        void closeButtonPressed() override { if (onClose_) onClose_(); }
        std::unique_ptr<juce::AudioProcessorEditor> editor_;
        std::function<void()> onClose_;
    };

    BridgeWorker& worker_;
    std::unique_ptr<OffscreenEditorHost> offscreen_;
    std::unique_ptr<FloatWindow> floatWin_;
};

}
