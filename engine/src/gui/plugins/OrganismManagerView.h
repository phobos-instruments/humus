// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <functional>
#include <memory>
#include <vector>

#include <juce_cryptography/juce_cryptography.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/packs/PackLoader.h"
#include "core/packs/BuiltinPacks.h"
#include "core/packs/PackRegistry.h"
#include "core/packs/PackUpdates.h"
#include "gui/style/LookAndFeel.h"
#include "gui/app/Telemetry.h"
#include "gui/app/TelemetryEvents.h"
#include "hum/PackEntry.h"
#include "gui/common/Localisation.h"

namespace hum {

class OrganismManagerView : public juce::Component {
public:
    std::function<void()> onPacksChanged;

    OrganismManagerView() {
        title_.setText(tr("organism-manager.installed-organism-packs", "Installed organism packs"), juce::dontSendNotification);
        title_.setColour(juce::Label::textColourId, Palette::text);
        title_.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
        addAndMakeVisible(title_);

        installBtn_.setButtonText(tr("organism-manager.install-pack", "Install pack..."));
        installBtn_.onClick = [this] { chooseAndInstall(); };
        addAndMakeVisible(installBtn_);

        checkBtn_.setButtonText(tr("organism-manager.check-for-updates", "Check for updates"));
        checkBtn_.onClick = [this] { checkForUpdates(); };
        addAndMakeVisible(checkBtn_);

        status_.setColour(juce::Label::textColourId, Palette::textDim);
        status_.setFont(juce::FontOptions(11.0f));
        addAndMakeVisible(status_);

        rebuildRows();
    }

    void rebuildRows() {
        rows_.clear();
        for (const auto& pack : PackRegistry::instance().packs()) {
            auto row = std::make_unique<Row>();
            row->id = juce::String(pack.manifest.id);
            int classes = 0;
            for (const auto& c : pack.organisms) classes += (int) c.classes.size();

            row->enable.setToggleState(pack.enabled, juce::dontSendNotification);
            row->enable.setColour(juce::ToggleButton::tickColourId, Palette::accent);
            row->enable.onClick = [this, r = row.get()] {
                const bool on = r->enable.getToggleState();
                PackRegistry::instance().setPackEnabled(r->id.toStdString(), on);
                telemetryCount(telemetry::packToggle(r->id.toStdString(), on));
                changed();
            };
            addAndMakeVisible(row->enable);

            row->name.setText(juce::String(pack.manifest.name) + "   v"
                                  + juce::String(pack.manifest.version) + juce::String("   -   ")
                                  + juce::String(classes) + " organisms"
                                  + (pack.builtin ? "" : "   (installed)"),
                              juce::dontSendNotification);
            row->name.setColour(juce::Label::textColourId, Palette::text);
            row->name.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
            addAndMakeVisible(row->name);

            row->desc.setText(juce::String(pack.manifest.description), juce::dontSendNotification);
            row->desc.setColour(juce::Label::textColourId, Palette::textDim);
            row->desc.setFont(juce::FontOptions(11.5f));
            addAndMakeVisible(row->desc);

            if (!pack.builtin) {
                row->uninstall = std::make_unique<juce::TextButton>("Uninstall");
                row->uninstall->onClick = [this, r = row.get()] { uninstall(r->id.toStdString()); };
                addAndMakeVisible(*row->uninstall);
            }
            for (const auto& u : updates_)
                if (u.id == pack.manifest.id) {
                    row->update = std::make_unique<juce::TextButton>(tr("organism-manager.update-to-v", "Update to v")
                                                                    + juce::String(u.version));
                    row->update->setColour(juce::TextButton::buttonColourId, Palette::accent);
                    row->update->onClick = [this, u] { applyUpdate(u); };
                    addAndMakeVisible(*row->update);
                    if (!u.notes.empty())
                        row->desc.setText(juce::String(u.notes), juce::dontSendNotification);
                    break;
                }
            rows_.push_back(std::move(row));
        }
        setSize(560, 96 + (int) rows_.size() * kRowH);
        resized();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(Palette::background);
        int y = 40;
        for (size_t i = 0; i < rows_.size(); ++i, y += kRowH) {
            g.setColour(Palette::panel);
            g.fillRoundedRectangle(10.0f, (float) y, (float) getWidth() - 20.0f, kRowH - 8.0f, 6.0f);
        }
    }

    void resized() override {
        title_.setBounds(14, 8, getWidth() - 28, 24);
        int y = 40;
        for (auto& r : rows_) {
            r->enable.setBounds(18, y + 8, 28, 24);
            const int nameW = getWidth() - 70 - (r->uninstall ? 96 : 0);
            r->name.setBounds(52, y + 4, nameW, 22);
            r->desc.setBounds(52, y + 26, getWidth() - 70, 34);
            if (r->uninstall) r->uninstall->setBounds(getWidth() - 104, y + 6, 88, 22);
            if (r->update) r->update->setBounds(getWidth() - 148, y + 34, 132, 22);
            y += kRowH;
        }
        installBtn_.setBounds(getWidth() - 140, getHeight() - 38, 126, 26);
        checkBtn_.setBounds(getWidth() - 288, getHeight() - 38, 140, 26);
        status_.setBounds(14, getHeight() - 36, getWidth() - 300, 22);
    }

private:
    void changed() {
        if (onPacksChanged) onPacksChanged();
    }

    void chooseAndInstall() {
        chooser_ = std::make_unique<juce::FileChooser>(tr("organism-manager.install-organism-pack", "Install organism pack"),
                                                       juce::File(), "*.humpack");
        chooser_->launchAsync(juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this](const juce::FileChooser& fc) {
                                  auto f = fc.getResult();
                                  if (f == juce::File{}) return;
                                  std::string err;
                                  if (PackLoader::instance().installHumpack(f, err)) {
                                      status_.setText(tr("organism-manager.installed", "Installed ") + f.getFileNameWithoutExtension(),
                                                      juce::dontSendNotification);
                                      rebuildRows();
                                      changed();
                                  } else {
                                      status_.setText(tr("organism-manager.install-failed", "Install failed: ") + juce::String(err),
                                                      juce::dontSendNotification);
                                  }
                              });
    }

    static juce::String fetch(const juce::String& url) {
        auto stream = juce::URL(url).createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(8000));
        return stream ? stream->readEntireStreamAsString() : juce::String();
    }

    void checkForUpdates() {
        checkBtn_.setEnabled(false);
        status_.setText(tr("organism-manager.checking-for-updates", "Checking for updates..."), juce::dontSendNotification);
        std::vector<InstalledPack> installed;
        for (const auto& p : PackRegistry::instance().packs())
            installed.push_back({p.manifest.id, p.manifest.version});
        const auto tag = PackLoader::platformTag();

        juce::Component::SafePointer<OrganismManagerView> safe(this);
        juce::Thread::launch([safe, installed, tag] {
            const auto json = fetch(kPackManifestUrl);
            auto ups = availableUpdates(json.toStdString(), installed, HUM_PACK_ABI, tag);
            const bool reached = json.isNotEmpty();
            juce::MessageManager::callAsync([safe, ups, reached] {
                if (auto* self = safe.getComponent()) self->onUpdatesFetched(ups, reached);
            });
        });
    }

    void onUpdatesFetched(std::vector<PackUpdate> ups, bool reached) {
        checkBtn_.setEnabled(true);
        updates_ = std::move(ups);
        if (!reached)
            status_.setText(tr("organism-manager.could-not-reach", "Could not reach ") + juce::URL(kPackManifestUrl).getDomain(),
                            juce::dontSendNotification);
        else if (updates_.empty())
            status_.setText(tr("organism-manager.everything-is-up-to-date", "Everything is up to date"), juce::dontSendNotification);
        else
            status_.setText(juce::String((int) updates_.size()) + tr("organism-manager.update", " update")
                                + (updates_.size() == 1 ? "" : "s") + " available",
                            juce::dontSendNotification);
        rebuildRows();
    }

    void applyUpdate(PackUpdate u) {
        if (!juce::String(u.url).startsWithIgnoreCase("https://")) {
            status_.setText(tr("organism-manager.refused-plain-url", "Refused: the update for ")
                                + juce::String(u.id) + tr("organism-manager.not-https", " is not offered over https"),
                            juce::dontSendNotification);
            return;
        }
        if (u.sha256.size() != 64) {
            status_.setText(tr("organism-manager.refused-no-checksum", "Refused: the update for ")
                                + juce::String(u.id) + tr("organism-manager.no-checksum", " carries no checksum"),
                            juce::dontSendNotification);
            return;
        }
        status_.setText(tr("organism-manager.downloading", "Downloading ") + juce::String(u.id) + " v" + juce::String(u.version)
                            + "...", juce::dontSendNotification);
        juce::Component::SafePointer<OrganismManagerView> safe(this);
        juce::Thread::launch([safe, u] {
            auto tmp = juce::File::createTempFile(".humpack");
            bool ok = false;
            if (auto in = juce::URL(juce::String(u.url))
                             .createInputStream(juce::URL::InputStreamOptions(
                                                    juce::URL::ParameterHandling::inAddress)
                                                    .withConnectionTimeoutMs(15000))) {
                juce::FileOutputStream out(tmp);
                ok = out.openedOk() && out.writeFromInputStream(*in, -1) > 0;
            }
            juce::MessageManager::callAsync([safe, u, tmp, ok] {
                auto* self = safe.getComponent();
                if (self == nullptr) { tmp.deleteFile(); return; }
                self->finishUpdate(u, tmp, ok);
            });
        });
    }

    void finishUpdate(const PackUpdate& u, juce::File tmp, bool downloaded) {
        if (!downloaded) {
            status_.setText(tr("organism-manager.download-failed-for", "Download failed for ") + juce::String(u.id),
                            juce::dontSendNotification);
            tmp.deleteFile();
            return;
        }
        std::string err;
        bool ok = false;
        {
            juce::FileInputStream in(tmp);
            const auto have = in.openedOk() ? juce::SHA256(in).toHexString().toLowerCase()
                                            : juce::String();
            if (have != juce::String(u.sha256))
                err = "the download's checksum does not match the manifest";
            else
                ok = PackLoader::instance().installHumpack(tmp, err);
        }
        tmp.deleteFile();
        if (ok) {
            status_.setText(tr("organism-manager.updated", "Updated ") + juce::String(u.id) + tr("organism-manager.to-v", " to v") + juce::String(u.version),
                            juce::dontSendNotification);
            updates_.erase(std::remove_if(updates_.begin(), updates_.end(),
                                          [&](const PackUpdate& x) { return x.id == u.id; }),
                           updates_.end());
            rebuildRows();
            changed();
        } else {
            status_.setText(tr("organism-manager.update-failed", "Update failed: ") + juce::String(err), juce::dontSendNotification);
        }
    }

    void uninstall(const std::string& id) {
        std::string err;
        if (PackLoader::instance().uninstallPack(id, err)) {
            status_.setText(tr("organism-manager.uninstalled", "Uninstalled ") + juce::String(id), juce::dontSendNotification);
            rebuildRows();
            changed();
        } else {
            status_.setText(tr("organism-manager.uninstall-failed", "Uninstall failed: ") + juce::String(err), juce::dontSendNotification);
        }
    }

    static constexpr int kRowH = 72;
    static constexpr const char* kPackManifestUrl =
        "https://humus.phobos-instruments.com/downloads/packs/versions.json";
    struct Row {
        juce::String id;
        juce::ToggleButton enable;
        juce::Label name, desc;
        std::unique_ptr<juce::TextButton> uninstall;
        std::unique_ptr<juce::TextButton> update;
    };

    juce::Label title_, status_;
    std::vector<std::unique_ptr<Row>> rows_;
    juce::TextButton installBtn_, checkBtn_;
    std::vector<PackUpdate> updates_;
    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganismManagerView)
};

}
