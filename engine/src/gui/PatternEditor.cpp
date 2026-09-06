#include "gui/PatternEditor.h"

#include <algorithm>
#include <cmath>

#include "core/ParamSchema.h"
#include "gui/LookAndFeel.h"
#include "gui/ParamReset.h"
#include "gui/Localisation.h"

namespace hum {

PatternEditor::PatternEditor(EngineHost& host, std::string organism, PatternEditorSpec spec)
    : host_(host), name_(std::move(organism)), spec_(std::move(spec)) {
    host_.patterns().ensure(name_, spec_.laneCount);
    build();
    setSize(preferredContentWidth(), preferredContentHeight(preferredContentWidth()));
    startTimerHz(60);
}

void PatternEditor::repaintTimeline() {
    repaint(gridX(), kKnobRowH, getWidth() - gridX(), lanesBottom() - kKnobRowH);
}

void PatternEditor::timerCallback() {
    if (!host_.isPlaying()) {
        if (showPlayhead_) { showPlayhead_ = false; repaintTimeline(); }
        return;
    }
    double tick = host_.positionBeats() * Pattern::kTicksPerBeat;
    if (const auto* p = pattern(); p && p->duration > 0) tick = std::fmod(tick, (double) p->duration);
    showPlayhead_ = true;
    if (std::abs(tick - playTick_) > 0.5) { playTick_ = tick; repaintTimeline(); }
}

const Pattern* PatternEditor::pattern() const {
    if (auto* cm = host_.model().byName(name_)) return &cm->pattern;
    return nullptr;
}

int PatternEditor::preferredContentHeight(int) const {
    return kKnobRowH + kRulerH + spec_.laneCount * kLaneH + kToolbarH;
}

void PatternEditor::bindKnob(ParamSlider& k, const std::string& param) {
    double lo = 0.0, hi = 1.0;
    for (auto& d : schemaFor("Drums")) if (d.name == param) { lo = d.min; hi = d.max; break; }
    k.setRange(lo, hi, 0.0);
    k.setNumDecimalPlacesToDisplay(2);
    k.setValue(host_.liveParamValue(name_, param), juce::dontSendNotification);
    enableDoubleClickReset(k, host_, name_, param);
    const std::string cn = name_, pn = param;
    auto* kp = &k;
    k.onValueChange = [this, kp, cn, pn] { host_.editParam(cn, pn, kp->getValue()); };
    k.onDragStart  = [this, cn, pn] { host_.beginParamDrag(cn, pn); };
    k.onDragEnd    = [this] { host_.endParamDrag(); };
    k.onPopup = [this, cn, pn](juce::Point<int> p) {
        showAutomateMenu(host_, cn, pn, p, [this] { if (onAutomationChanged) onAutomationChanged(); });
    };
}

void PatternEditor::build() {
    auto mkToggle = [&](const std::string& caption, const std::string& param) {
        auto t = std::make_unique<juce::ToggleButton>(caption);
        t->setColour(juce::ToggleButton::textColourId, Palette::text);
        t->setToggleState(host_.liveParamValue(name_, param) >= 0.5, juce::dontSendNotification);
        const std::string cn = name_, pn = param;
        t->onStateChange = [this, tp = t.get(), cn, pn] {
            host_.editParam(cn, pn, tp->getToggleState() ? 1.0 : 0.0);
        };
        return t;
    };

    masterKnob_ = std::make_unique<ParamSlider>(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow);
    bindKnob(*masterKnob_, spec_.masterVolumeParam);
    addAndMakeVisible(*masterKnob_);

    if (!spec_.muteParam.empty()) { muteToggle_ = mkToggle("Mute", spec_.muteParam); addAndMakeVisible(*muteToggle_); }
    if (!spec_.gateParam.empty()) { gateToggle_ = mkToggle("Gate 1 & 2", spec_.gateParam); addAndMakeVisible(*gateToggle_); }

    for (int i = 0; i < spec_.laneCount; ++i) {
        if (!spec_.volumeParamPrefix.empty()) {
            auto k = std::make_unique<ParamSlider>(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox);
            bindKnob(*k, spec_.volumeParamPrefix + std::to_string(i + 1));
            addAndMakeVisible(*k); volKnobs_.push_back(std::move(k));
        }
        if (!spec_.enableParamPrefix.empty()) {
            auto t = mkToggle("", spec_.enableParamPrefix + std::to_string(i + 1));
            addAndMakeVisible(*t); enableToggles_.push_back(std::move(t));
        }
        if (!spec_.fileParamPrefix.empty()) {
            auto f = std::make_unique<SoundFileSlot>(host_, name_, spec_.fileParamPrefix + std::to_string(i + 1));
            addAndMakeVisible(*f); fileSlots_.push_back(std::move(f));
        }
        auto chip = std::make_unique<juce::TextButton>(laneSnapText(i));
        chip->setColour(juce::TextButton::buttonColourId, Palette::panelLight);
        chip->setColour(juce::TextButton::textColourOffId, Palette::textDim);
        chip->setTooltip(tr("pattern-editor.snap-resolution-for-this-lane", "Snap resolution for this lane"));
        chip->onClick = [this, i] {
            juce::PopupMenu m;
            const char* res[] = {"1/4", "1/8", "1/16", "1/32"};
            const juce::String cur = laneSnapText(i);
            for (int k = 0; k < 4; ++k) m.addItem(k + 1, res[k], true, cur == res[k]);
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(snapChips_[(size_t) i].get()),
                            [this, i, res](int r) {
                if (r < 1) return;
                host_.patterns().setChannelSnap(name_, i, res[r - 1]);
                snapChips_[(size_t) i]->setButtonText(res[r - 1]);
                repaint();
            });
        };
        addAndMakeVisible(*chip); snapChips_.push_back(std::move(chip));

        auto nl = std::make_unique<juce::TextButton>("<");
        auto nr = std::make_unique<juce::TextButton>(">");
        nl->setTooltip(tr("pattern-editor.nudge-this-lane-left-fine", "Nudge this lane left (fine, 1 tick)"));
        nr->setTooltip(tr("pattern-editor.nudge-this-lane-right-fine", "Nudge this lane right (fine, 1 tick)"));
        nl->onClick = [this, i] { host_.patterns().nudgeChannel(name_, i, -1); repaint(); };
        nr->onClick = [this, i] { host_.patterns().nudgeChannel(name_, i, +1); repaint(); };
        addAndMakeVisible(*nl); addAndMakeVisible(*nr);
        laneNudgeL_.push_back(std::move(nl)); laneNudgeR_.push_back(std::move(nr));
    }

    snapBox_ = std::make_unique<juce::ComboBox>();
    const char* res[] = {"1/4", "1/8", "1/16", "1/32"};
    for (int i = 0; i < 4; ++i) snapBox_->addItem(res[i], i + 1);
    juce::String cur = pattern() ? juce::String(pattern()->matrixResolution) : "1/16";
    for (int i = 0; i < 4; ++i) if (cur == res[i]) snapBox_->setSelectedId(i + 1, juce::dontSendNotification);
    if (snapBox_->getSelectedId() == 0) snapBox_->setSelectedId(3, juce::dontSendNotification);
    snapBox_->onChange = [this] {
        host_.patterns().setResolution(name_, snapBox_->getText().toStdString());
        for (int i = 0; i < (int) snapChips_.size(); ++i)
            snapChips_[(size_t) i]->setButtonText(laneSnapText(i));
        repaint();
    };
    addAndMakeVisible(*snapBox_);

    coarseL_ = std::make_unique<juce::TextButton>("<<");
    coarseR_ = std::make_unique<juce::TextButton>(">>");
    fineL_   = std::make_unique<juce::TextButton>("<");
    fineR_   = std::make_unique<juce::TextButton>(">");
    coarseL_->setTooltip(tr("pattern-editor.nudge-grid-left-by-the", "Nudge grid left by the snap step"));
    coarseR_->setTooltip(tr("pattern-editor.nudge-grid-right-by-the", "Nudge grid right by the snap step"));
    fineL_->setTooltip(tr("pattern-editor.nudge-grid-left-fine-1", "Nudge grid left (fine, 1 tick)"));
    fineR_->setTooltip(tr("pattern-editor.nudge-grid-right-fine-1", "Nudge grid right (fine, 1 tick)"));
    coarseL_->onClick = [this] { host_.patterns().reframe(name_, -snapTicks()); repaint(); };
    coarseR_->onClick = [this] { host_.patterns().reframe(name_, +snapTicks()); repaint(); };
    fineL_->onClick   = [this] { host_.patterns().reframe(name_, -1); repaint(); };
    fineR_->onClick   = [this] { host_.patterns().reframe(name_, +1); repaint(); };
    for (auto* b : {coarseL_.get(), fineL_.get(), fineR_.get(), coarseR_.get()}) addAndMakeVisible(*b);
}

void PatternEditor::reloadValues() {
    if (masterKnob_) masterKnob_->setValue(host_.liveParamValue(name_, spec_.masterVolumeParam), juce::dontSendNotification);
    if (muteToggle_) muteToggle_->setToggleState(host_.liveParamValue(name_, spec_.muteParam) >= 0.5, juce::dontSendNotification);
    if (gateToggle_) gateToggle_->setToggleState(host_.liveParamValue(name_, spec_.gateParam) >= 0.5, juce::dontSendNotification);
    for (int i = 0; i < (int) volKnobs_.size(); ++i)
        volKnobs_[(size_t) i]->setValue(host_.liveParamValue(name_, spec_.volumeParamPrefix + std::to_string(i + 1)), juce::dontSendNotification);
    for (int i = 0; i < (int) enableToggles_.size(); ++i)
        enableToggles_[(size_t) i]->setToggleState(host_.liveParamValue(name_, spec_.enableParamPrefix + std::to_string(i + 1)) >= 0.5, juce::dontSendNotification);
    for (auto& f : fileSlots_) f->refresh();
    for (int i = 0; i < (int) snapChips_.size(); ++i)
        snapChips_[(size_t) i]->setButtonText(laneSnapText(i));
    repaint();
}

void PatternEditor::fitZoom() {
    const auto* p = pattern();
    const double beats = (p && p->duration > 0) ? (double) p->duration / Pattern::kTicksPerBeat : 4.0;
    const double avail = getWidth() - gridX() - 6.0;
    if (avail > 0.0) ppb_ = juce::jlimit(18.0, 160.0, avail / std::max(1.0, beats));
}

void PatternEditor::resized() {
    fitZoom();
    const int w = getWidth();
    if (muteToggle_) muteToggle_->setBounds(4, 2, 70, 16);
    masterKnob_->setBounds(4, 18, 44, kKnobRowH - 20);
    if (gateToggle_) gateToggle_->setBounds(w - 96, 2, 92, 16);

    const int n = (int) volKnobs_.size();
    if (n > 0) {
        const int x0 = 64, x1 = w - 10;
        const double step = (double) (x1 - x0) / n;
        for (int i = 0; i < n; ++i)
            volKnobs_[(size_t) i]->setBounds((int) (x0 + i * step), 18, (int) step - 4, kKnobRowH - 20);
    }

    for (int i = 0; i < spec_.laneCount; ++i) {
        const int top = laneTop(i);
        if (i < (int) enableToggles_.size())
            enableToggles_[(size_t) i]->setBounds(kNumW + 2, top + (kLaneH - 18) / 2, kEnW, 18);
        const int chipX = kLeftW - kChipW - 2;
        const int nudgeRX = chipX - 2 - kNudgeW;
        const int nudgeLX = nudgeRX - kNudgeW;
        const int fileX = kNumW + kEnW + 6;
        const int fileW = nudgeLX - 4 - fileX;
        if (i < (int) fileSlots_.size())
            fileSlots_[(size_t) i]->setBounds(fileX, top + 2, fileW, kLaneH - 4);
        if (i < (int) laneNudgeL_.size()) {
            laneNudgeL_[(size_t) i]->setBounds(nudgeLX, top + 3, kNudgeW, kLaneH - 6);
            laneNudgeR_[(size_t) i]->setBounds(nudgeRX, top + 3, kNudgeW, kLaneH - 6);
        }
        if (i < (int) snapChips_.size())
            snapChips_[(size_t) i]->setBounds(chipX, top + 3, kChipW, kLaneH - 6);
    }

    const int ty = lanesBottom() + 3;
    coarseL_->setBounds(6, ty, 26, 22);
    fineL_->setBounds(34, ty, 20, 22);
    fineR_->setBounds(56, ty, 20, 22);
    coarseR_->setBounds(78, ty, 26, 22);
    snapBox_->setBounds(176, ty, 90, 22);
}

void PatternEditor::paint(juce::Graphics& g) {
    g.fillAll(Palette::background);

    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(tr("pattern-editor.volume", "Volume"), 4, kKnobRowH - 12, 56, 12, juce::Justification::centred);
    for (int i = 0; i < (int) volKnobs_.size(); ++i) {
        auto b = volKnobs_[(size_t) i]->getBounds();
        g.drawText(juce::String(i + 1), b.getX(), 4, b.getWidth(), 12, juce::Justification::centred);
    }

    paintRuler(g);
    for (int i = 0; i < spec_.laneCount; ++i) paintLane(g, i);

    if (showPlayhead_) {
        const float x = tickToX((int) std::lround(playTick_));
        if (x >= gridX() && x <= getWidth()) {
            g.setColour(Palette::accent.withAlpha(0.85f));
            g.drawVerticalLine((int) x, (float) kKnobRowH, (float) lanesBottom());
        }
    }

    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(tr("pattern-editor.default", "Default:"), 112, lanesBottom() + 3, 60, 22, juce::Justification::centredRight);
}

void PatternEditor::paintRuler(juce::Graphics& g) {
    const int w = getWidth(), top = kKnobRowH;
    const auto* p = pattern();
    int bpb = 4;
    if (p && !p->channels.empty() && !p->channels[0].timeSignatures.empty())
        bpb = std::max(1, p->channels[0].timeSignatures[0].numerator);

    g.setColour(Palette::panel);
    g.fillRect(0, top, w, kRulerH);
    g.setColour(Palette::panel.darker(0.2f));
    g.fillRect(0, top, gridX(), kRulerH);

    const int ticksPerBar = bpb * Pattern::kTicksPerBeat;
    const int ticksPerBeat = Pattern::kTicksPerBeat;
    const int sub = std::max(1, snapTicks());
    const int firstTick = ((int) (scrollTicks_) / sub) * sub;
    for (int tick = firstTick; ; tick += sub) {
        const float x = tickToX(tick);
        if (x > w) break;
        if (x < gridX()) continue;
        const bool bar = (tick % ticksPerBar) == 0;
        const bool beat = (tick % ticksPerBeat) == 0;
        g.setColour(bar ? Palette::border
                        : beat ? Palette::panelLight
                               : Palette::panelLight.withAlpha(0.35f));
        g.drawVerticalLine((int) x, (float) top, (float) lanesBottom());
        if (bar) {
            g.setColour(Palette::textDim);
            g.setFont(juce::FontOptions(10.0f));
            g.drawText(juce::String(tick / ticksPerBar + 1), (int) x + 2, top, 30, kRulerH - 2,
                       juce::Justification::left);
        }
    }

    if (p && p->duration > 0) {
        const float xe = tickToX(p->duration);
        if (xe < w) {
            g.setColour(juce::Colours::black.withAlpha(0.28f));
            g.fillRect(juce::jmax((float) gridX(), xe), (float) top, (float) w - juce::jmax((float) gridX(), xe),
                       (float) (lanesBottom() - top));
        }
        if (xe >= gridX()) {
            g.setColour(Palette::accent);
            g.fillRect(xe - 1.0f, (float) top, 2.0f, (float) kRulerH);
        }
    }
    g.setColour(Palette::border);
    g.drawHorizontalLine(top + kRulerH, 0.0f, (float) w);
}

void PatternEditor::paintLane(juce::Graphics& g, int i) {
    const int w = getWidth(), top = laneTop(i);
    const auto* p = pattern();
    g.setColour((i % 2) ? Palette::panel.darker(0.05f) : Palette::panel);
    g.fillRect(0, top, gridX(), kLaneH);
    g.setColour(Palette::border.withAlpha(0.5f));
    g.drawHorizontalLine(top + kLaneH, 0.0f, (float) w);

    g.setColour(Palette::textDim);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(juce::String(i + 1), 0, top, kNumW, kLaneH, juce::Justification::centred);

    const float cy = top + kLaneH * 0.5f;
    g.setColour(Palette::panelLight);
    g.drawHorizontalLine((int) cy, (float) gridX(), (float) w);
    if (!p) return;
    const auto chans = p->triggerChannels();
    if (i >= (int) chans.size()) return;
    const bool enabled = i >= (int) enableToggles_.size() || enableToggles_[(size_t) i]->getToggleState();
    const int flashWin = Pattern::kTicksPerBeat / 4;
    for (int t : chans[(size_t) i]->triggers) {
        const float x = tickToX(t);
        if (x < gridX() - 2 || x > w) continue;
        const double since = playTick_ - t;
        const bool lit = showPlayhead_ && enabled && since >= 0.0 && since < flashWin;
        g.setColour(!enabled ? Palette::textDim : lit ? juce::Colours::white : Palette::accent);
        const float r = lit ? 6.5f : 5.0f;
        const float ww = r + 4.0f;
        juce::Path tri;
        tri.addTriangle(x - 1.0f, cy - r, x - 1.0f, cy + r, x - 1.0f + ww, cy);
        g.fillPath(tri);
    }
}

int PatternEditor::laneAt(int y) const {
    const int top = kKnobRowH + kRulerH;
    if (y < top || y >= lanesBottom()) return -1;
    return (y - top) / kLaneH;
}

static int resToTicks(const juce::String& r) {
    const int denom = r.fromFirstOccurrenceOf("/", false, false).getIntValue();
    return denom > 0 ? (4 * Pattern::kTicksPerBeat) / denom : Pattern::kTicksPerBeat / 4;
}

int PatternEditor::snapTicks() const {
    return resToTicks(pattern() ? juce::String(pattern()->matrixResolution) : "1/16");
}

juce::String PatternEditor::laneSnapText(int lane) const {
    const auto* p = pattern();
    if (p) {
        const auto ch = p->triggerChannels();
        if (lane >= 0 && lane < (int) ch.size() && !ch[(size_t) lane]->snap.empty())
            return juce::String(ch[(size_t) lane]->snap);
    }
    return p ? juce::String(p->matrixResolution) : "1/16";
}

int PatternEditor::laneSnapTicks(int lane) const { return resToTicks(laneSnapText(lane)); }

int PatternEditor::snapTickForLane(int lane, int tick) const {
    const int s = std::max(1, laneSnapTicks(lane));
    return ((tick + s / 2) / s) * s;
}

int PatternEditor::nearestTrigger(int lane, int tick, int tolTicks) const {
    const auto* p = pattern();
    if (!p) return -1;
    const auto chans = p->triggerChannels();
    if (lane < 0 || lane >= (int) chans.size()) return -1;
    int best = -1, bd = tolTicks + 1;
    for (int t : chans[(size_t) lane]->triggers) {
        const int d = std::abs(t - tick);
        if (d < bd) { bd = d; best = t; }
    }
    return best;
}

}
