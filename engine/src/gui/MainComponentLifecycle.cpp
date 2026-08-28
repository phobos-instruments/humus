#include "gui/MainComponent.h"

#include "core/PackLoader.h"
#include "gui/AppSettings.h"
#include "gui/DefaultPatchHandler.h"
#include "gui/HubClient.h"
#include "gui/SettingsComponent.h"
#include "gui/Telemetry.h"
#include "gui/TelemetryEvents.h"

namespace hum {

void MainComponent::openSetupWizard(bool firstBoot) {
    if (wizard_) { wizard_->toFront(true); return; }
    SetupWizard::Callbacks cbs;
    cbs.onAppearanceChanged = [this] { refreshAppearance(); };
    cbs.onFinished = [this] {
        if (auto* w = wizard_.release()) {
            w->setVisible(false);
            juce::MessageManager::callAsync([w] { delete w; });
        }
        offerDefaultPatchHandlerOnce();
    };
    wizard_ = std::make_unique<SetupWizard>(host_, std::move(cbs), firstBoot);
    wizard_->setAlwaysOnTop(true);
    wizard_->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
    wizard_->setCentrePosition(getScreenBounds().getCentre());
    wizard_->setVisible(true);
    wizard_->toFront(true);
}

void MainComponent::openGuide(bool firstBoot) {
    if (guide_) { guide_->toFront(true); return; }
    GuideView::Actions a;
    a.onOpenDemo = [this](juce::File f) {
        confirmDiscardThenRun([this, f] { openFileAt(f); });
    };
    a.onWizard = [this, firstBoot] { openSetupWizard(firstBoot); };
    a.onFinished = [this] {
        if (auto* g = guide_.release()) {
            g->setVisible(false);
            juce::MessageManager::callAsync([g] { delete g; });
        }
        if (!wizard_) offerDefaultPatchHandlerOnce();
    };
    guide_ = std::make_unique<GuideView>(std::move(a), firstBoot);
    guide_->setAlwaysOnTop(true);
    guide_->addToDesktop(juce::ComponentPeer::windowHasDropShadow);
    guide_->setCentrePosition(getScreenBounds().getCentre());
    guide_->setVisible(true);
    guide_->toFront(true);
}

void MainComponent::startupCheckin() {
    if (juce::JUCEApplication::getInstance() == nullptr) return;
    const auto lic = LicenseStore::current();
    if (AppSettings::instance().getInt("updates.auto", 0) == 0) {
        if (!lic.valid) maybeShowNag();
        return;
    }
    HubClient::checkin(lic.valid ? lic.licenseId : std::string(),
        [safe = juce::Component::SafePointer<MainComponent>(this)](
            bool ok, hubproto::CheckinResult r) {
            if (safe == nullptr || !ok) return;
            if (r.licenseStatus == hubproto::LicenseStatus::Revoked) {
                LicenseStore::remove();
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon, "License deactivated",
                    "This install's license was deactivated on the hub, so "
                    "Humus is back to unregistered - which locks nothing. "
                    "If this is a surprise, get in touch.");
            }
            if (!r.hasUpdate) return;
            if (AppSettings::instance().getString("app.skipVersion", "")
                == juce::String(r.updateVersion)) return;
            safe->showUpdateNotice(r.updateVersion, r.updateUrl, r.updateNotes,
                                   r.updateSha256);
        });
    if (!lic.valid) maybeShowNag();
}

void MainComponent::maybeShowNag() {
    auto& st = AppSettings::instance();
    if (st.getInt("setup.completed", 0) == 0) return;
    const int n = st.getInt("license.bootCount", 0) + 1;
    st.set("license.bootCount", n);
    if (n % 10 != 0) return;
    nagCard_ = std::make_unique<NagCard>();
    nagCard_->onEnterLicense = [this] {
        dismissNag();
        openSettings(SettingsComponent::kLicense);
    };
    nagCard_->onLater = [this] { dismissNag(); };
    addAndMakeVisible(*nagCard_);
    placeUpdateNotice();
    nagCard_->toFront(false);
}

void MainComponent::dismissNag() {
    if (auto* n = nagCard_.release()) {
        n->setVisible(false);
        juce::MessageManager::callAsync([n] { delete n; });
    }
}

void MainComponent::flushTelemetry() {
    auto& sp = telemetrySpool();
    if (!sp.enabled() || sp.empty()) return;
    lastTelemetryFlushMs_ = juce::Time::currentTimeMillis();
    const auto body = hubproto::buildTelemetryBody(
        HubClient::instanceId(), HubClient::appVersion(),
        PackLoader::platformTag(), sp.crashedLastRun(), sp.events());
    HubClient::sendTelemetry(body, [](bool ok) {
        if (!ok) return;
        telemetrySpool().clearAfterFlush();
        telemetrySave();
    });
}

void MainComponent::checkForUpdatesManually() {
    if (juce::JUCEApplication::getInstance() == nullptr) return;
    setStatus("checking for updates...");
    HubClient::checkin("",
        [safe = juce::Component::SafePointer<MainComponent>(this)](
            bool ok, hubproto::CheckinResult r) {
            if (safe == nullptr) return;
            if (!ok) { safe->setStatus("could not reach the update server"); return; }
            if (r.hasUpdate) {
                safe->setStatus({});
                safe->showUpdateNotice(r.updateVersion, r.updateUrl, r.updateNotes,
                                   r.updateSha256);
            } else {
                safe->setStatus("Humus " + juce::String(HubClient::appVersion())
                                + " is up to date");
            }
        });
}

void MainComponent::showUpdateNotice(const std::string& version,
                                     const std::string& url,
                                     const std::string& notes,
                                     const std::string& sha256) {
    updateNotice_ = std::make_unique<UpdateNotice>(
        juce::String(version), juce::String(notes),
        juce::URL(juce::String(url)).getFileName());
    telemetryCount(telemetry::kUpdateShown);
    updateNotice_->onDownload = [this, url, sha256, version] {
        telemetryCount(telemetry::kUpdateTaken);
        if (downloaded_ != juce::File()) { downloaded_.revealToUser(); return; }
        if (updater_ == nullptr) updater_ = std::make_unique<AppUpdater>();
        juce::Component::SafePointer<MainComponent> safe(this);
        updater_->onProgress = [safe](double f) {
            if (safe != nullptr && safe->updateNotice_) safe->updateNotice_->setProgress(f);
        };
        updater_->onDone = [safe](bool ok, juce::String message, juce::File file) {
            if (safe == nullptr || !safe->updateNotice_) return;
            safe->downloaded_ = ok ? file : juce::File();
            safe->updateNotice_->setOutcome(ok ? message + " to "
                                                     + file.getParentDirectory().getFileName()
                                               : message,
                                            ok ? AppUpdater::revealVerb() : "Try again");
        };
        updateNotice_->setProgress(0.0);
        updater_->start(juce::String(url), juce::String(sha256), juce::String(version));
    };
    updateNotice_->onCancel = [this] {
        if (updater_) updater_->cancel();
        dismissUpdateNotice();
    };
    updateNotice_->onSkip = [this, version] {
        AppSettings::instance().set("app.skipVersion", juce::String(version));
        dismissUpdateNotice();
    };
    updateNotice_->onLater = [this] { dismissUpdateNotice(); };
    addAndMakeVisible(*updateNotice_);
    placeUpdateNotice();
    updateNotice_->toFront(false);
}

void MainComponent::placeUpdateNotice() {
    if (updateNotice_ && nagCard_) dismissNag();
    if (auto* card = updateNotice_ ? (juce::Component*) updateNotice_.get()
                                   : (juce::Component*) nagCard_.get())
        card->setTopLeftPosition(getWidth() - card->getWidth() - 14,
                                 getHeight() - card->getHeight() - 40);
}

void MainComponent::dismissUpdateNotice() {
    if (auto* n = updateNotice_.release()) {
        n->setVisible(false);
        juce::MessageManager::callAsync([n] { delete n; });
    }
}

}
