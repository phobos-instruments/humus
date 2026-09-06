#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/LicenseCheck.h"
#include "gui/AppSettings.h"
#include "gui/HubClient.h"
#include "gui/LicenseStore.h"
#include "gui/LookAndFeel.h"
#include "gui/Telemetry.h"
#include "gui/Localisation.h"

namespace hum {

class LicensePanel : public juce::Component {
public:
    LicensePanel() {
        status_.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
        addAndMakeVisible(status_);
        detail_.setFont(juce::FontOptions(12.0f));
        detail_.setColour(juce::Label::textColourId, Palette::textDim);
        addAndMakeVisible(detail_);

        code_.setTextToShowWhenEmpty("HUM-XXXXX-XXXXX-XXXXX",
                                     Palette::textDim.withAlpha(0.6f));
        code_.setFont(juce::FontOptions(14.0f));
        code_.onTextChange = [this] { refreshCodeFeedback(); };
        code_.onReturnKey = [this] { if (activate_.isEnabled()) doActivate(); };
        addAndMakeVisible(code_);

        feedback_.setFont(juce::FontOptions(12.0f));
        addAndMakeVisible(feedback_);

        activate_.setButtonText(tr("license.activate", "Activate"));
        activate_.onClick = [this] { doActivate(); };
        addAndMakeVisible(activate_);

        remove_.setButtonText(tr("license.remove-license", "Remove License"));
        remove_.onClick = [this] {
            LicenseStore::remove();
            refresh();
        };
        addChildComponent(remove_);

        privacyTitle_.setText(tr("license.privacy", "Privacy"), juce::dontSendNotification);
        privacyTitle_.setFont(juce::FontOptions(15.0f).withStyle("Bold"));
        addAndMakeVisible(privacyTitle_);

        consent_.setButtonText(tr("license.share-anonymous-usage-data", "Share anonymous usage data"));
        consent_.setToggleState(
            AppSettings::instance().getInt("telemetry.enabled", 0) != 0,
            juce::dontSendNotification);
        consent_.onClick = [this] {
            AppSettings::instance().set("telemetry.enabled",
                                        consent_.getToggleState() ? 1 : 0);
            telemetrySyncConsent();
        };
        addAndMakeVisible(consent_);

        updates_.setButtonText(tr("license.check-for-updates-at-startup", "Check for updates at startup"));
        updates_.setToggleState(
            AppSettings::instance().getInt("updates.auto", 0) != 0,
            juce::dontSendNotification);
        updates_.onClick = [this] {
            AppSettings::instance().set("updates.auto",
                                        updates_.getToggleState() ? 1 : 0);
        };
        addAndMakeVisible(updates_);

        recap_.setText(tr("license.sharing-sends-anonymous-usage-counts",
           "Sharing sends anonymous usage counts (like which "
           "organisms you plant or how often you save) and whether "
           "the last session crashed. The startup update check asks "
           "our server once per launch. Both are tied to a random "
           "ID, not to you - no names, no emails, no patch "
           "contents, ever. With both off, Humus never touches the "
           "network except when you ask it to."),
                       juce::dontSendNotification);
        recap_.setFont(juce::FontOptions(12.0f));
        recap_.setColour(juce::Label::textColourId, Palette::textDim);
        recap_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(recap_);

        refresh();
    }

    void refresh() {
        const auto lic = LicenseStore::current();
        if (lic.valid) {
            const auto who = lic.name.empty() ? lic.plan : lic.name;
            status_.setText(tr("license.registered-to", "Registered to ") + juce::String(who),
                            juce::dontSendNotification);
            status_.setColour(juce::Label::textColourId, Palette::accent);
            detail_.setText(juce::String(lic.plan) + tr("license.activated", "  -  activated ")
                                + juce::String(lic.issuedAt).upToFirstOccurrenceOf(
                                    "T", false, false),
                            juce::dontSendNotification);
        } else {
            status_.setText(tr("license.unregistered", "Unregistered"), juce::dontSendNotification);
            status_.setColour(juce::Label::textColourId, Palette::text);
            detail_.setText(tr("license.humus-is-fully-functional-either",
               "Humus is fully functional either way. Got a code? "
               "Make it official below."),
                            juce::dontSendNotification);
        }
        code_.setVisible(!lic.valid);
        feedback_.setVisible(!lic.valid);
        activate_.setVisible(!lic.valid);
        remove_.setVisible(lic.valid);
        refreshCodeFeedback();
    }

    void resized() override {
        auto r = getLocalBounds().reduced(12);
        status_.setBounds(r.removeFromTop(24));
        detail_.setBounds(r.removeFromTop(20));
        r.removeFromTop(10);
        auto row = r.removeFromTop(28);
        code_.setBounds(row.removeFromLeft(230));
        row.removeFromLeft(8);
        activate_.setBounds(row.removeFromLeft(90));
        remove_.setBounds(code_.getBounds().withWidth(130));
        feedback_.setBounds(r.removeFromTop(22));
        r.removeFromTop(18);
        privacyTitle_.setBounds(r.removeFromTop(24));
        consent_.setBounds(r.removeFromTop(26));
        updates_.setBounds(r.removeFromTop(26));
        recap_.setBounds(r.removeFromTop(96));
    }

private:
    void refreshCodeFeedback() {
        const auto raw = code_.getText().toStdString();
        if (raw.empty()) {
            feedback_.setText({}, juce::dontSendNotification);
            activate_.setEnabled(false);
            return;
        }
        const bool ok = licensecheck::checksumOk(raw);
        activate_.setEnabled(ok);
        feedback_.setText(ok ? tr("license.code-looks-good", "code looks good") : tr("license.keep-typing-that-doesn-t", "keep typing - that doesn't scan yet"),
                          juce::dontSendNotification);
        feedback_.setColour(juce::Label::textColourId,
                            ok ? Palette::accent : Palette::textDim);
    }

    void doActivate() {
        activate_.setEnabled(false);
        feedback_.setText("activating...", juce::dontSendNotification);
        HubClient::activate(
            code_.getText().toStdString(), {},
            [safe = juce::Component::SafePointer<LicensePanel>(this)](
                hubproto::ActivateResult r) {
                if (safe == nullptr) return;
                if (!r.ok) {
                    safe->feedback_.setText(friendlyError(r.error),
                                            juce::dontSendNotification);
                    safe->feedback_.setColour(juce::Label::textColourId,
                                              Palette::textDim);
                    safe->activate_.setEnabled(true);
                    return;
                }
                const auto lic = LicenseStore::save(r.payloadB64, r.sigB64);
                if (!lic.valid) {
                    safe->feedback_.setText(
                        "that license did not verify - try again, or ping us",
                        juce::dontSendNotification);
                    safe->activate_.setEnabled(true);
                    return;
                }
                safe->refresh();
            });
    }

    static juce::String friendlyError(const std::string& e) {
        if (e == "unknown_code") return "the hub does not know that code";
        if (e == "code_revoked") return "that code has been retired";
        if (e == "activation_limit") return "that code has used all its activations";
        return "could not reach the hub - check your connection and try again";
    }

    juce::Label status_, detail_, feedback_, privacyTitle_, recap_;
    juce::TextEditor code_;
    juce::TextButton activate_, remove_;
    juce::ToggleButton consent_, updates_;
};

}
