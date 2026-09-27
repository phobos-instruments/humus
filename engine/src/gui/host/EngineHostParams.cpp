// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/host/EngineHost.h"

#include <limits>

#include <algorithm>
#include <string>
#include <vector>

#include "core/library/BankLibrary.h"
#include "core/packs/ClassString.h"
#include "core/params/ParamSchema.h"
#include "core/params/RangeEnd.h"
#include "core/params/SpanMove.h"
#include "core/graph/RtWord.h"
#include "core/packs/Roles.h"
#include "gui/host/NodeRoll.h"
#include "hum/caps/Params.h"

namespace hum {

double EngineHost::prePassValue(const std::string& organism, const std::string& param) {
    if (!(capturing_ && playing_)) return std::numeric_limits<double>::quiet_NaN();
    return liveParamValue(organism, param);
}

bool EngineHost::storedSpan(const std::string& organism, const std::string& param,
                            double& lo, double& hi) const {
    if (graph_)
        if (auto* c = graph_->find(organism))
            if (auto* p = c->params.byName(param)) {
                lo = rtLoadWord(p->rangeMin);
                hi = rtLoadWord(p->rangeMax);
                return true;
            }
    if (const auto* cm = model_.byName(organism))
        for (const auto& p : cm->properties)
            if (p.name == param) { lo = p.rangeMin; hi = p.rangeMax; return true; }
    return false;
}

bool EngineHost::setRangeEnd(const std::string& organism, const std::string& param, double value) {
    const auto end = rangeEndOf(param);
    const auto base = rangeBaseOf(param);
    const auto* cm = model_.byName(organism);
    if (cm == nullptr) return false;
    const ParamDesc* desc = nullptr;
    for (const auto& d : schemaFor(cm->classRaw))
        if (d.name == base) desc = &d;
    if (desc == nullptr || !desc->isRange) return false;

    double lo = desc->min, hi = desc->max;
    storedSpan(organism, base, lo, hi);
    const auto key = spanKey(organism, base);
    const auto known = spanIntent_.find(key);
    double want = known != spanIntent_.end() ? known->second.first : 0.5 * (lo + hi);
    double spread = known != spanIntent_.end() ? known->second.second : std::max(0.0, hi - lo);

    const auto mode = rangeMode(organism, base);
    if (mode == RangeMode::Single || end == RangeEnd::Whole) {
        want = value;
    } else if (end == RangeEnd::Spread) {
        spread = std::max(0.0, value);
    } else {
        if (end == RangeEnd::Low) lo = std::min(value, hi);
        else                      hi = std::max(value, lo);
        want = 0.5 * (lo + hi);
        spread = hi - lo;
    }
    const double half = mode == RangeMode::Single ? 0.0 : 0.5 * spread;
    lo = want - half;
    hi = want + half;
    slideInto(desc->min, desc->max, lo, hi);

    spanIntent_[key] = {want, spread};
    ownSpanIntent_ = true;
    setParamRange(organism, base, lo, hi);
    ownSpanIntent_ = false;
    return true;
}

void EngineHost::setParam(const std::string& organism, const std::string& param, double value) {
    if (setRangeEnd(organism, param, value)) return;
    if (param == kBypassParam) {
        if (!liveControl_) recordParamRevert(organism, param);
        applyBypass(organism, value >= 0.5);
        dirty_ = true;
        ++changeStamp_;
        return;
    }
    if (param == kTrackMuteParam) {
        if (!liveControl_) recordParamRevert(organism, param);
        applyTrackMute(organism, value >= 0.5);
        dirty_ = true;
        ++changeStamp_;
        return;
    }
    if (param == kSoloParam) { setSoloed(organism, value >= 0.5); return; }
    if (midi_.recorderSwitch(organism, param, value)) {
        ++changeStamp_;
        return;
    }
    if (param == kArmParam) {
        const bool on = value >= 0.5;
        if (nodeRecordsMedia(organism)) setParam(organism, "Record", on ? 1.0 : 0.0);
        else midi().armInlet(organism, on);
        ++changeStamp_;
        return;
    }
    if (param == kRandomAction) { fireRandom(organism, value >= 0.5); return; }
    if (rollscope::isPodAction(param)) { firePodRandom(rollscope::podOfAction(param), value >= 0.5); return; }
    if (const int slot = markerOfAction(param); slot > 0) {
        if (const auto* cm = model_.byName(organism);
            cm != nullptr && isClockPseudo(cm->displayClass)) {
            fireMarker(organism, slot, value >= 0.5);
            return;
        }
    }
    if (isMarkerStepAction(param)) {
        if (const auto* cm = model_.byName(organism);
            cm != nullptr && isClockPseudo(cm->displayClass)) {
            fireMarkerStep(organism, param == kMarkerNextAction ? +1 : -1, value >= 0.5);
            return;
        }
    }
    if (isTransportAction(param)) {
        if (const auto* cm = model_.byName(organism);
            cm != nullptr && isClockPseudo(cm->displayClass)) {
            fireTransport(param, value >= 0.5);
            return;
        }
    }
    const auto* reloader = dynamic_cast<ReloadOnParam*>(liveOrganism(organism));
    const bool reload = reloader != nullptr && reloader->reloadsOn(param)
                        && liveParamValue(organism, param) != value;
    if (param == "Patch" && liveParamValue(organism, param) != value)
        scheduler_.post([this, alive = hostAlive_, organism] {
            if (*alive) pullVoiceParams(organism);
        });
    if (isPresetStepAction(param)) {
        firePresetStep(organism, param == kPresetNextAction ? +1 : -1, value >= 0.5);
        return;
    }
    if (isMeterParam(param)) {
        if (const auto* cm = model_.byName(organism); cm != nullptr && isClockPseudo(cm->displayClass)) {
            const Meter cur = automation_.meterAt(positionBeats());
            const Meter next = param == kMeterBeatsParam ? Meter::clamped(value, cur.unit)
                                                         : Meter::clamped(cur.beats, value);
            if (next != cur) automation_.setTimeSignature(next.beats, next.unit);
            if (liveControl_) ++liveControlGen_;
            return;
        }
    }
    if (isGrooveParam(param) && setClockGroove(organism, param, value)) return;
    if (param == kTempoParam) {
        if (const auto* cm = model_.byName(organism); cm != nullptr && isClockPseudo(cm->displayClass)) {
            const double bpm = juce::jlimit(kTempoMin, kTempoMax, value);
            const double was = prePassValue(organism, param);
            setTempo(bpm);
            if (liveControl_) ++liveControlGen_;
            else { dirty_ = true; ++changeStamp_; }
            noteTouch(organism, param);
            capturePoint(organism, param, bpm, bpm, false, was);
            if (playing_ && !capturing_ && !derivedControl_)
                perfRing_.push_back({positionBeats(), organism, param, bpm, bpm, false});
            return;
        }
    }

    if (const auto* cm = model_.byName(organism);
        cm != nullptr && isMetapadPseudo(cm->displayClass)) {
        const double was = prePassValue(organism, param);
        metapad_.applyTarget(param, value);
        if (param != kMetaXParam && param != kMetaYParam && param != kMetaRecallParam) return;
        noteTouch(organism, param);
        capturePoint(organism, param, value, value, false, was);
        if (playing_ && !capturing_ && !derivedControl_)
            perfRing_.push_back({positionBeats(), organism, param, value, value, false});
        return;
    }

    const double was = prePassValue(organism, param);

    const bool live = liveControl_;
    if (!live) recordParamRevert(organism, param);
    if (!live) { dirty_ = true; ++changeStamp_; }
    else ++liveControlGen_;
    if (auto* c = model_.byName(organism)) {
        bool found = false;
        for (auto& p : c->properties)
            if (p.name == param) {
                p.value = value; p.rangeMin = p.rangeMax = value;
                if (!live) p.userEdited = true;
                found = true;
            }
        if (!found)
            for (const auto& d : schemaFor(c->classRaw))
                if (d.name == param) {
                    Parameter p;
                    p.index = (int) c->properties.size();
                    p.name = param;
                    p.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
                    p.value = value; p.rangeMin = p.rangeMax = value;
                    p.userEdited = !live;
                    c->properties.push_back(p);
                    break;
                }
        if (!live) c->presetDirty = true;
    }
    if (graph_)
        if (auto* c = graph_->find(organism)) {
            if (auto* p = c->params.byName(param)) {
                rtStoreWord(p->value, value);
                rtStoreWord(p->rangeMin, value);
                rtStoreWord(p->rangeMax, value);
            } else {
                const juce::ScopedLock sl(lock_);
                Parameter np;
                const auto* cm = model_.byName(organism);
                np.index = cm && isPluginKind(cm->kind) ? -1
                                                        : (int) c->params.all().size();
                np.name = param;
                np.value = value; np.rangeMin = np.rangeMax = value;
                c->params.add(np);
            }
        }
    if (reload)
        scheduler_.post([this, alive = hostAlive_, organism] {
            if (*alive) onFileNodeChanged(organism, {}, false);
        });
    if (value >= 0.5) noteFired(organism, param);
    if (param == "Record") {
        files().setRecorderActive(organism, value >= 0.5);
        record().noteRecordSwitch(organism, value >= 0.5);
    } else {
        record().noteTransport(organism, param, value);
    }
    if (decks().fireTrigger(organism, param, value)) return;
    if (files().fireTrigger(organism, param, value)) return;
    if (param == "Channel") scheduleAuxChannelCheck();
    sendMidiOut(organism, param, value);
    noteTouch(organism, param);
    capturePoint(organism, param, value, value, false, was);
    if (playing_ && !capturing_ && !derivedControl_)
        perfRing_.push_back({positionBeats(), organism, param, value, value, false});
}

void EngineHost::pullVoiceTexts(const std::string& organism) {
    auto* src = dynamic_cast<TextVoiceSource*>(liveOrganism(organism));
    if (src == nullptr) return;
    std::vector<std::pair<std::string, std::string>> vals;
    if (!src->takeVoiceTexts(vals)) return;
    for (const auto& [param, text] : vals) setParamText(organism, param, text);
}

void EngineHost::pullVoiceParams(const std::string& organism) {
    auto* src = dynamic_cast<VoiceSource*>(liveOrganism(organism));
    if (src == nullptr) return;
    std::vector<std::pair<std::string, double>> vals;
    if (!src->voiceParams(vals)) return;
    for (const auto& [param, v] : vals) setParam(organism, param, v);
    if (onNodeRolled)
        scheduler_.post([this, alive = hostAlive_, organism] {
            if (*alive && onNodeRolled) onNodeRolled(organism);
        });
}

void EngineHost::setRollLocked(const std::string& organism, const std::string& param,
                               bool locked) {
    auto* cm = model_.byName(organism);
    if (cm == nullptr) return;
    if (locked == (cm->rollLocked.count(param) > 0)) return;
    pushUndo();
    if (locked) cm->rollLocked.insert(param);
    else        cm->rollLocked.erase(param);
    dirty_ = true;
    ++changeStamp_;
}

void EngineHost::setRangeMode(const std::string& organism, const std::string& param, RangeMode mode) {
    auto* cm = model_.byName(organism);
    if (cm == nullptr) return;
    const auto base = rangeBaseOf(param);
    if (mode == rangeMode(organism, base)) return;
    if (mode == RangeMode::ValueSpread) cm->rangeModes.erase(base);
    else                                cm->rangeModes[base] = rangeModeName(mode);
    dirty_ = true;
    ++changeStamp_;
    double lo = 0.0, hi = 0.0;
    if (!storedSpan(organism, base, lo, hi)) return;
    const auto key = spanKey(organism, base);
    const auto known = spanIntent_.find(key);
    const double want = known != spanIntent_.end() ? known->second.first : 0.5 * (lo + hi);
    setRangeEnd(organism, base, want);
}

void EngineHost::setParamRange(const std::string& organism, const std::string& param,
                               double min, double max) {
    double wasLo = std::numeric_limits<double>::quiet_NaN(), wasHi = wasLo;
    if (capturing_ && playing_)
        if (const auto* cm = model_.byName(organism))
            for (const auto& p : cm->properties)
                if (p.name == param) { wasLo = p.rangeMin; wasHi = p.rangeMax; }
    const bool live = liveControl_;
    if (live) ++liveControlGen_;
    else      { recordParamRevert(organism, param); dirty_ = true; ++changeStamp_; }
    if (!ownSpanIntent_) spanIntent_[spanKey(organism, param)] = {0.5 * (min + max), max - min};
    if (auto* c = model_.byName(organism)) {
        for (auto& p : c->properties)
            if (p.name == param) {
                p.value = min;
                p.rangeMin = min;
                p.rangeMax = max;
                p.isRange = true;
                if (!live) p.userEdited = true;
            }
        if (!live) c->presetDirty = true;
    }
    if (graph_)
        if (auto* c = graph_->find(organism))
            if (auto* p = c->params.byName(param)) {
                rtStoreWord(p->value, min);
                rtStoreWord(p->rangeMin, min);
                rtStoreWord(p->rangeMax, max);
                rtStoreWord(p->isRange, true);
            }
    noteTouch(organism, param);
    capturePoint(organism, param, min, max, true, wasLo, wasHi);
    if (playing_ && !capturing_ && !derivedControl_)
        perfRing_.push_back({positionBeats(), organism, param, min, max, true});
}

double EngineHost::liveParamValue(const std::string& organism, const std::string& param) {
    if (param == kTempoParam)
        if (const auto* cm = model_.byName(organism); cm != nullptr && isClockPseudo(cm->displayClass))
            return liveTempo();
    if (isMeterParam(param))
        if (const auto* cm = model_.byName(organism); cm != nullptr && isClockPseudo(cm->displayClass)) {
            const Meter m = automation_.meterAt(positionBeats());
            return param == kMeterBeatsParam ? (double) m.beats : (double) m.unit;
        }
    if (isGrooveParam(param))
        if (const auto* cm = model_.byName(organism); cm != nullptr && isClockPseudo(cm->displayClass))
            return param == kGrooveParam ? liveGroove() : (double) liveGrooveGrid();
    if (graph_)
        if (auto* c = graph_->find(organism))
            if (auto* p = c->params.byName(param)) return rtLoadWord(p->value);
    if (auto* cm = model_.byName(organism))
        for (auto& pr : cm->properties) if (pr.name == param) return pr.value;
    return 0.0;
}

void EngineHost::batchLiveParamValues(const std::string& name,
                                       const std::vector<hum::Parameter>& params,
                                       std::vector<double>& out) const {
    out.assign(params.size(), 0.0);
    bool foundInGraph = false;
    if (graph_) {
        if (auto* c = graph_->find(name)) {
            foundInGraph = true;
            for (size_t i = 0; i < params.size(); ++i)
                if (auto* p = c->params.byName(params[i].name))
                    out[i] = rtLoadWord(p->value);
        }
    }
    if (!foundInGraph) {
        if (auto* cm = model_.byName(name))
            for (size_t i = 0; i < params.size(); ++i)
                for (auto& pr : cm->properties)
                    if (pr.name == params[i].name) { out[i] = pr.value; break; }
    }
}

bool EngineHost::isExternallyControlled(const std::string& organism,
                                       const std::string& param) const {
    if (automation().isAutomated(organism, param)) return true;
    if (!varDrivingKnob(organism, param).empty()) return true;
    for (const auto& e : midiState_.map.entries())
        if (e.organism == organism && e.param == param) return true;
    for (const auto& e : mod().map().entries())
        if (e.organism == organism && e.param == param) return true;
    for (const auto& e : osc().map().entries())
        if (e.organism == organism && e.param == param) return true;
    return false;
}

std::string EngineHost::translated(const std::string& key, const std::string& written) const {
    return juce::translate(juce::String::fromUTF8(key.c_str()), juce::String::fromUTF8(written.c_str()))
        .toStdString();
}

bool EngineHost::isLiveTracked(const std::string& organism, const std::string& param) const {
    if (isExternallyControlled(organism, param)) return true;
    if (auto* cm = model_.byName(organism)) {
        int idx = -1;
        for (auto& pr : cm->properties) if (pr.name == param) { idx = pr.index; break; }
        if (idx >= 0)
            for (const auto& m : model_.metapad.mask)
                if (m.restore && m.organismName == organism && m.propertyIndex == idx) return true;
    }
    return false;
}

double EngineHost::liveParamMax(const std::string& organism, const std::string& param) {
    if (graph_)
        if (auto* c = graph_->find(organism))
            if (auto* p = c->params.byName(param))
                return rtLoadWord(p->isRange) ? rtLoadWord(p->rangeMax) : rtLoadWord(p->value);
    if (auto* cm = model_.byName(organism))
        for (auto& pr : cm->properties) if (pr.name == param) return pr.isRange ? pr.rangeMax : pr.value;
    return 0.0;
}

void EngineHost::adoptTextParam(OrganismModel& cm, const std::string& param, const std::string& text) {
    for (const auto& d : schemaFor(cm.classRaw)) {
        if (d.name != param || !d.isText) continue;
        Parameter p;
        p.index = (int) cm.properties.size();
        p.name = d.name;
        p.type = d.isPlainText ? "text" : "soundfile";
        p.text = text;
        p.userEdited = true;
        cm.properties.push_back(p);
        return;
    }
}

void EngineHost::setParamText(const std::string& organism, const std::string& param,
                              const std::string& text) {
    dirty_ = true; ++changeStamp_; ++textStamp_;
    if (auto* c = model_.byName(organism)) {
        bool held = false;
        for (auto& p : c->properties)
            if (p.name == param) { p.text = text; p.userEdited = true; held = true; }
        if (!held) adoptTextParam(*c, param, text);
        c->presetDirty = true;
    }
    const auto forDsp = textForDsp(organism, text);
    {
        const juce::ScopedLock sl(lock_);
        if (graph_)
            if (auto* c = graph_->find(organism))
                if (auto* p = c->params.byName(param)) p->text = forDsp;
    }
    if (graph_)
        if (auto* c = graph_->find(organism)) {
            c->onTextChanged(param, forDsp);
            if (dynamic_cast<Tagged*>(c) != nullptr || dynamic_cast<NamedInlet*>(c) != nullptr) {
                const juce::ScopedLock sl(lock_);
                graph_->rebindNamedInlets();
            }
        }
    if (param.rfind("File", 0) == 0) onFileNodeChanged(organism, text, true, param);
    if (!audioRunning_) primeOffline(1);
}

std::string EngineHost::textForDsp(const std::string& organism,
                                   const std::string& text) const {
    if (text.empty()) return text;
    const auto* cm = model_.byName(organism);
    return cm == nullptr ? text : banks::resolve(text, cm->displayClass);
}

std::string EngineHost::liveParamText(const std::string& organism, const std::string& param) {
    if (graph_)
        if (auto* c = graph_->find(organism))
            if (auto* p = c->params.byName(param)) return p->text;
    if (auto* cm = model_.byName(organism))
        for (auto& pr : cm->properties) if (pr.name == param) return pr.text;
    return {};
}

void EngineHost::beginParamDrag(const std::string& organism, const std::string& param) {
    paramDragActive_ = true;
    paramDragPushed_ = false;
    paramEditKey_ = organism + '\x01' + param;
}

void EngineHost::endParamDrag() {
    paramDragActive_ = false;
    paramEditTime_ = juce::Time::getMillisecondCounter();
}

void EngineHost::editParam(const std::string& organism, const std::string& param, double value) {
    const std::string key = organism + '\x01' + param;
    const juce::uint32 now = juce::Time::getMillisecondCounter();

    if (paramDragActive_) {
        if (!paramDragPushed_) { pushParamStep(); paramDragPushed_ = true; }
    } else {
        const bool sameRun = (key == paramEditKey_) && (now - paramEditTime_ < 600);
        if (!sameRun) pushParamStep();
    }
    paramEditKey_ = key;
    paramEditTime_ = now;
    setParam(organism, param, value);
    if (param == "Target")
        if (const auto* cm = model_.byName(organism); cm && classHasRole(cm->classRaw, role::kMidiTrack))
            applyMidiTrackTarget(organism, (int) value);
}

NodeState EngineHost::captureNodeState(const std::string& name) const {
    NodeState s;
    if (const auto* cm = model_.byName(name)) {
        s.props = settingsOnly(cm->properties);
        s.pattern = cm->pattern;
    }
    return s;
}

void EngineHost::applyNodeState(const std::string& name, const NodeState& s) {
    const auto* cm = model_.byName(name);
    if (cm == nullptr) return;
    pushParamStep();
    for (const auto& p : s.props) {
        const Parameter* cur = nullptr;
        for (const auto& q : cm->properties)
            if (q.name == p.name) { cur = &q; break; }
        if (cur != nullptr && cur->text != p.text)
            setParamText(name, p.name, p.text);
        if (p.isRange) {
            if (cur == nullptr || cur->rangeMin != p.rangeMin || cur->rangeMax != p.rangeMax)
                setParamRange(name, p.name, p.rangeMin, p.rangeMax);
        } else if (cur == nullptr || cur->value != p.value) {
            setParam(name, p.name, p.value);
        }
    }
    if (auto* mc = mutableByName(name); mc != nullptr
                                        && (mc->pattern.present || s.pattern.present)
                                        && !samePattern(mc->pattern, s.pattern)) {
        mc->pattern = s.pattern;
        syncPattern(name);
        dirty_ = true;
    }
}

}
