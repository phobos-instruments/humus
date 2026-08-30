#include "plugin/HumusProcessor.h"

#include <cmath>

#include "core/ParamSchema.h"
#include "io/PatchLoader.h"
#include "io/PatchWriter.h"
#include "plugin/HumusEditor.h"

namespace hum {

namespace {
constexpr int kStateMagic = 0x484D5833;
constexpr int kStateMagicV2 = 0x484D5832;
}

HumusProcessor::HumusProcessor()
    : juce::AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    for (int i = 0; i < kNumMacros; ++i)
        addParameter(macroParam_[i] = new juce::AudioParameterFloat(
            juce::ParameterID("macro" + juce::String(i + 1), 1),
            "Macro " + juce::String(i + 1), juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
}

bool HumusProcessor::loadPatchFile(const juce::File& file, std::string& error) {
    if (!loadPatchText(file.loadFileAsString().toStdString(), error)) return false;
    patchName_ = file.getFileNameWithoutExtension();
    return true;
}

bool HumusProcessor::loadPatchText(const std::string& amhXml, std::string& error) {
    PatchDocumentModel model;
    if (!parsePatchText(amhXml, model, error)) return false;

    auto g = std::make_unique<AudioGraph>();
    if (!buildGraph(model, *g, error)) return false;
    g->prepare(sampleRate_, blockSize_, model.clock.tempo);
    g->transport().setPlaying(true);

    {
        const juce::ScopedLock sl(stageLock_);
        pending_ = std::move(g);
    }
    hasPending_.store(true);
    docText_ = amhXml;
    docXml_ = juce::XmlDocument::parse(juce::String(juce::CharPointer_UTF8(amhXml.c_str())));
    model_ = std::move(model);
    {
        const juce::ScopedLock sl(editLock_);
        pendingEdits_.clear();
    }
    if (patchName_.isEmpty()) patchName_ = "(embedded patch)";
    return true;
}

void HumusProcessor::setLiveParam(const std::string& organism, const std::string& param,
                                    double v) {
    for (auto& cm : model_.organisms) {
        if (cm.name != organism) continue;
        bool found = false;
        for (auto& p : cm.properties)
            if (p.name == param) {
                p.value = v;
                if (p.isRange) p.rangeMin = p.rangeMax = v;
                p.userEdited = true;
                found = true;
                break;
            }
        if (!found) {
            Parameter np;
            np.name = param;
            np.value = v;
            np.userEdited = true;
            for (const auto& d : schemaFor(cm.displayClass))
                if (d.name == param) {
                    np.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
                    break;
                }
            cm.properties.push_back(np);
        }
        break;
    }
    const juce::ScopedLock sl(editLock_);
    pendingEdits_.push_back({organism, param, v});
}

double HumusProcessor::liveParam(const std::string& organism, const std::string& param,
                                   double fallback) const {
    if (const auto* cm = model_.byName(organism))
        for (const auto& p : cm->properties)
            if (p.name == param) return p.isRange ? p.rangeMin : p.value;
    return fallback;
}

void HumusProcessor::applyParamEdits() {
    const juce::ScopedTryLock sl(editLock_);
    if (!sl.isLocked() || pendingEdits_.empty()) return;
    for (const auto& e : pendingEdits_)
        if (auto* c = graph_->find(e.organism)) {
            if (auto* p = c->params.byName(e.param)) {
                p->value = e.value;
                p->rangeMin = p->rangeMax = e.value;
            } else {
                Parameter np;
                np.name = e.param;
                np.value = np.rangeMin = np.rangeMax = e.value;
                c->params.add(np);
            }
        }
    pendingEdits_.clear();
}

void HumusProcessor::applyMacros() {
    const juce::ScopedTryLock sl(macroLock_);
    if (!sl.isLocked()) return;
    for (int i = 0; i < kNumMacros; ++i) {
        const auto& m = macros_[i];
        if (m.organism.empty() || m.param.empty()) continue;
        const double v = m.lo + (double) macroParam_[i]->get() * (m.hi - m.lo);
        if (auto* c = graph_->find(m.organism))
            if (auto* p = c->params.byName(m.param)) { p->value = v; p->rangeMin = p->rangeMax = v; }
    }
}

void HumusProcessor::setMacroMapping(int i, const std::string& organism, const std::string& param) {
    if (i < 0 || i >= kNumMacros) return;
    float lo = 0.0f, hi = 1.0f;
    if (!organism.empty() && !param.empty())
        if (auto* cm = model_.byName(organism))
            for (auto& d : schemaFor(cm->displayClass))
                if (d.name == param) { lo = (float) d.min; hi = (float) d.max; break; }
    const juce::ScopedLock sl(macroLock_);
    macros_[i] = { organism, param, lo, hi };
}

std::pair<std::string, std::string> HumusProcessor::macroMapping(int i) const {
    if (i < 0 || i >= kNumMacros) return {};
    return { macros_[i].organism, macros_[i].param };
}

float HumusProcessor::macroValue(int i) const {
    return (i >= 0 && i < kNumMacros && macroParam_[i]) ? macroParam_[i]->get() : 0.0f;
}

void HumusProcessor::setMacroValue(int i, float v) {
    if (i >= 0 && i < kNumMacros && macroParam_[i])
        macroParam_[i]->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v));
}

void HumusProcessor::applyPending() {
    const juce::ScopedTryLock sl(stageLock_);
    if (!sl.isLocked()) return;
    graph_ = std::move(pending_);
    hasPending_.store(false);
    masters_ = graph_ ? findMasterTaps(*graph_) : std::vector<MasterTap*>{};
    auxes_ = graph_ ? findHardwareOuts(*graph_) : std::vector<HardwareOut*>{};
    midiPorts_ = graph_ ? graphmidi::findPorts(*graph_) : graphmidi::Ports{};
    if (graph_)
        for (const auto& cm : model_.organisms)
            if (auto* hp = dynamic_cast<PluginNode*>(graph_->find(cm.name)))
                midiPorts_.hosted.push_back({hp, cm.name, cm.midiReceiveMode,
                                             cm.midiReceiveChannel});
    lastPpq_ = -1.0;
}

void HumusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    blockSize_ = samplesPerBlock;
    inScratch_.setSize(2, std::max(1, samplesPerBlock));
    if (graph_) graph_->prepare(sampleRate, samplesPerBlock, graph_->transport().tempo());
    if (pending_) pending_->prepare(sampleRate, samplesPerBlock, pending_->transport().tempo());
}

void HumusProcessor::driveTransportFromHost() {
    auto& t = graph_->transport();
    bool hostPlaying = false;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (pos->getBpm()) t.setTempo(*pos->getBpm());
            hostPlaying = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition(); ppq && hostPlaying) {
                const bool resync =
                    !hostWasPlaying_ || lastPpq_ < 0.0
                    || std::abs(*ppq - (lastPpq_ + (double) blockSize_ / t.samplesPerBeat()))
                           > 0.1;
                if (resync) t.setBeatPosition(*ppq);
                lastPpq_ = *ppq;
            }
            if (!hostPlaying) lastPpq_ = -1.0;
            hostWasPlaying_ = hostPlaying;
        }
    }
    t.setPlaying(hostPlaying || freeRun_.load(std::memory_order_relaxed));
}

void HumusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    if (hasPending_.load()) applyPending();
    const int n = buffer.getNumSamples();

    if (!graph_) { midi.clear(); return; }

    graphmidi::deliver(midi, midiPorts_);
    midi.clear();

    if (rewindReq_.exchange(false)) graph_->transport().setBeatPosition(0.0);
    driveTransportFromHost();
    applyMacros();
    applyParamEdits();

    const int nCopy = std::min(n, inScratch_.getNumSamples());
    for (int c = 0; c < 2; ++c)
        inScratch_.copyFrom(c, 0, buffer,
                            std::min(c, buffer.getNumChannels() - 1), 0, nCopy);
    const float* ins[2] = { inScratch_.getReadPointer(0), inScratch_.getReadPointer(1) };
    buffer.clear();
    float* outs[2] = { buffer.getWritePointer(0),
                       buffer.getNumChannels() > 1 ? buffer.getWritePointer(1)
                                                   : buffer.getWritePointer(0) };
    renderGraphBlock(*graph_, ins, 2, outs, std::min(2, buffer.getNumChannels()), nCopy,
                     masters_, auxes_);

    graphmidi::collect(midiPorts_, midi);

    for (int c = 0; c < 2 && c < buffer.getNumChannels(); ++c)
        peak_[c].store(buffer.getMagnitude(c, 0, n));
}

juce::AudioProcessorEditor* HumusProcessor::createEditor() {
    return new HumusEditor(*this);
}

void HumusProcessor::getStateInformation(juce::MemoryBlock& dest) {
    juce::MemoryOutputStream os(dest, false);
    os.writeInt(kStateMagic);
    const juce::ScopedLock sl(macroLock_);
    for (int i = 0; i < kNumMacros; ++i) {
        os.writeString(macros_[i].organism);
        os.writeString(macros_[i].param);
    }
    os.writeBool(freeRun_.load());
    os.writeString(docText_.empty() ? juce::String()
                                    : juce::String(writePatchString(model_, docXml_.get())));
}

void HumusProcessor::setStateInformation(const void* data, int size) {
    if (data == nullptr || size <= 0) return;
    juce::MemoryInputStream is(data, (size_t) size, false);
    if (const int magic = size >= 4 ? is.readInt() : 0;
        magic == kStateMagic || magic == kStateMagicV2) {
        std::pair<std::string, std::string> maps[kNumMacros];
        for (int i = 0; i < kNumMacros; ++i) {
            maps[i].first = is.readString().toStdString();
            maps[i].second = is.readString().toStdString();
        }
        if (magic == kStateMagic) freeRun_.store(is.readBool());
        std::string error;
        loadPatchText(is.readString().toStdString(), error);
        for (int i = 0; i < kNumMacros; ++i)
            setMacroMapping(i, maps[i].first, maps[i].second);
        return;
    }
    std::string error;
    loadPatchText(std::string(static_cast<const char*>(data), (size_t) size), error);
}

}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new hum::HumusProcessor(); }
