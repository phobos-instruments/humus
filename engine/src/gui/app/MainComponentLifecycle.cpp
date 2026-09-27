// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/app/MainComponent.h"
#include "gui/help/GuideView.h"
#include "gui/app/AppUpdater.h"
#include "gui/app/DeviceNotice.h"
#include "gui/app/UpdateNotice.h"
#include "gui/app/UsageLog.h"
#include "gui/app/SetupWizard.h"

#include "core/packs/PackLoader.h"
#include "gui/app/AppSettings.h"
#include "gui/app/DefaultPatchHandler.h"
#include "gui/app/HubClient.h"
#include "gui/settings/SettingsComponent.h"
#include "gui/app/Telemetry.h"
#include "gui/app/TelemetryEvents.h"
#include "gui/common/Localisation.h"

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
        return;
    }
    HubClient::checkin(lic.valid ? lic.licenseId : std::string(),
        [safe = juce::Component::SafePointer<MainComponent>(this)](
            bool ok, hubproto::CheckinResult r) {
            if (safe == nullptr || !ok) return;
            if (r.licenseStatus == hubproto::LicenseStatus::Revoked) {
                LicenseStore::remove();
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon,
                    tr("main-lifecycle.license-deactivated", "License deactivated"),
                    tr("main-lifecycle.license-deactivated-body",
                       "This install's license was deactivated on the hub, so "
                       "Humus is back to unregistered - which locks nothing. "
                       "If this is a surprise, get in touch."));
            }
            if (!r.hasUpdate) return;
            if (AppSettings::instance().getString("app.skipVersion", "")
                == juce::String(r.updateVersion)) return;
            safe->showUpdateNotice(r.updateVersion, r.updateUrl, r.updateNotes,
                                   r.updateSha256);
        });
}

void MainComponent::noteOpenTime() {
    const auto now = juce::Time::currentTimeMillis();
    if (openSinceMs_ > 0 && now > openSinceMs_) usagelog::addOpenSeconds((double) (now - openSinceMs_) / 1000.0);
    openSinceMs_ = now;
}

void MainComponent::noteBounce(double seconds) {
    usagelog::write(usage::afterBounce(usagelog::read(), seconds));
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
    notifyOn("updates", tr("main-lifecycle.checking-for-updates", "checking for updates..."));
    HubClient::checkin("",
        [safe = juce::Component::SafePointer<MainComponent>(this)](
            bool ok, hubproto::CheckinResult r) {
            if (safe == nullptr) return;
            if (!ok) {
                safe->notifyErrorOn("updates", tr("main-lifecycle.could-not-reach-the-update", "could not reach the update server"));
                return;
            }
            if (r.hasUpdate) {
                safe->notifyOn("updates", {});
                safe->showUpdateNotice(r.updateVersion, r.updateUrl, r.updateNotes,
                                   r.updateSha256);
            } else {
                safe->notifyOn("updates", tr("main-lifecycle.humus", "Humus ") + juce::String(HubClient::appVersion())
                                              + tr("main-lifecycle.up-to-date", " is up to date"));
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
                                            ok ? AppUpdater::revealVerb() : tr("main-lifecycle.try-again", "Try again"));
        };
        updateNotice_->setProgress(0.0);
        updater_->start(juce::String(url), juce::String(sha256), juce::String(version),
                        juce::String(HubClient::instanceId()));
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
    auto* notice = (juce::Component*) updateNotice_.get();
    if (notice != nullptr)
        notice->setTopLeftPosition(getWidth() - notice->getWidth() - 14,
                                   getHeight() - notice->getHeight() - 40);
    cards_.placeAbove(getLocalBounds().withTrimmedRight(14),
                      notice != nullptr ? notice->getY() - CardStack::kGap : getHeight() - 40);
}

void MainComponent::showDeviceNotice(const DeviceChange& change) {
    auto card = std::make_unique<DeviceNotice>(change);
    auto* raw = card.get();
    const int category = change.midi ? SettingsComponent::kMidi : SettingsComponent::kAudio;
    card->onOpenSettings = [this, raw, category] {
        cards_.remove(raw);
        placeUpdateNotice();
        openSettings(category);
    };
    presentCard(std::move(card));
}

void MainComponent::presentCard(std::unique_ptr<juce::Component> card) {
    cards_.push(std::move(card));
    placeUpdateNotice();
    cards_.toFront(false);
}

void MainComponent::dismissUpdateNotice() {
    if (auto* n = updateNotice_.release()) {
        n->setVisible(false);
        juce::MessageManager::callAsync([n] { delete n; });
        placeUpdateNotice();
    }
}

}
