#include "gui/EngineHost.h"

#include "core/Categories.h"
#include "core/ClassString.h"
#include "core/CordReclaim.h"
#include "core/PackManifest.h"
#include "core/PluginHost.h"
#include "core/PackRegistry.h"
#include "core/PodModel.h"
#include "core/StripFamily.h"
#include "core/Randomize.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/ParamSchema.h"
#include "gui/PresetLibrary.h"
#include "hum/Capabilities.h"
#include "hum/Registry.h"

namespace hum {

int EngineHost::reconcilePropertyTypes(PatchDocumentModel& doc) {
    int fixed = 0;
    for (auto& cm : doc.organisms) {
        if (isPluginKind(cm.kind)) continue;
        for (const auto& d : schemaFor(cm.classRaw.empty() ? cm.displayClass : cm.classRaw)) {
            if (!d.isText) continue;
            const std::string want = d.isPlainText ? "text" : "soundfile";
            for (auto& p : cm.properties) {
                if (p.name != d.name || p.type == want || p.type == "text" || p.type == "soundfile")
                    continue;
                p.type = want;
                p.userEdited = true;
                ++fixed;
            }
        }
    }
    return fixed;
}

std::string EngineHost::addOrganism(const std::string& className, juce::Point<int> at,
                                       const std::string& podScope) {
    pushUndo();
    const std::string canonical = canonicalClass(className);
    const std::string plain = (podScope.empty() ? std::string() : podScope + "/")
                            + parseClassString(className).display;
    std::string name = plain;
    for (int n = 2; model_.byName(name) != nullptr; ++n)
        name = plain + "_" + std::to_string(n);

    OrganismModel cm;
    cm.name = name;
    cm.classRaw = canonical;
    const auto classId = parseClassString(canonical);
    cm.displayClass = classId.display;
    cm.kind = classId.kind;
    int idx = 0;
    for (const auto& d : schemaFor(canonical)) {
        Parameter p;
        p.index = idx++;
        p.name = d.name;
        if (d.isText) {
            p.type = d.isPlainText ? "text" : "soundfile";
            p.text = d.text;
        } else if (d.isRange) {
            p.type = "range";
            p.isRange = true;
            p.value = d.def;
            p.rangeMin = d.def;
            p.rangeMax = d.defMax;
        } else if (!d.text.empty()) {
            p.type = "rhythmic-unit";
            p.value = d.def;
            p.text = d.text;
        } else {
            p.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
            p.value = rollDefaultValue(d, juce::Random::getSystemRandom());
        }
        cm.properties.push_back(p);
    }
    model_.organisms.push_back(std::move(cm));
    positions_[name] = at;
    rebuild();
    pullVoiceParams(name);
    return name;
}

std::string EngineHost::duplicateTrack(const std::string& name) {
    const auto* src = model_.byName(name);
    if (src == nullptr) return {};
    const auto cls = src->classRaw;
    const auto state = captureNodeState(name);
    const auto at = position(name) + juce::Point<int>(0, 96);

    beginTransaction();
    pushUndo();
    const auto made = addOrganism(cls, at);
    if (made.empty()) { endTransaction(); return {}; }
    if (auto* cm = mutableByName(made)) {
        cm->properties = state.props;
        cm->pattern = state.pattern;
    }
    modelSwapped_ = true;
    rebuild();
    syncPattern(made);

    const auto audio = model_.connections;
    const auto midi = model_.midiConnections;
    for (const auto& c : audio) {
        if (c.src == name) connect(made, c.srcOutlet, c.dst, c.dstInlet);
        else if (c.dst == name) connect(c.src, c.srcOutlet, made, c.dstInlet);
    }
    for (const auto& c : midi) {
        if (c.src == name) connectMidi(made, c.srcOutlet, c.dst, c.dstInlet);
        else if (c.dst == name) connectMidi(c.src, c.srcOutlet, made, c.dstInlet);
    }
    endTransaction();
    return made;
}

void EngineHost::removeOrganism(const std::string& name) {
    pushUndo();
    auto& cs = model_.organisms;
    cs.erase(std::remove_if(cs.begin(), cs.end(),
                            [&](const OrganismModel& c) { return c.name == name; }), cs.end());
    auto& cn = model_.connections;
    cn.erase(std::remove_if(cn.begin(), cn.end(),
                            [&](const ConnectionModel& c) { return c.src == name || c.dst == name; }), cn.end());
    auto& mc = model_.midiConnections;
    mc.erase(std::remove_if(mc.begin(), mc.end(),
                            [&](const ConnectionModel& c) { return c.src == name || c.dst == name; }), mc.end());
    auto& vc = model_.videoConnections;
    vc.erase(std::remove_if(vc.begin(), vc.end(),
                            [&](const ConnectionModel& c) { return c.src == name || c.dst == name; }), vc.end());
    auto& pb = model_.perfBoxes;
    pb.erase(std::remove_if(pb.begin(), pb.end(),
                            [&](const PerformanceBox& b) { return b.organism == name; }), pb.end());
    positions_.erase(name);
    rebuild();
}

void EngineHost::renameReferences(const std::string& oldName, const std::string& newName) {
    for (auto& c : model_.organisms) if (c.name == oldName) c.name = newName;
    for (auto& cn : model_.connections) {
        if (cn.src == oldName) cn.src = newName;
        if (cn.dst == oldName) cn.dst = newName;
    }
    for (auto& cn : model_.midiConnections) {
        if (cn.src == oldName) cn.src = newName;
        if (cn.dst == oldName) cn.dst = newName;
    }
    for (auto& cn : model_.videoConnections) {
        if (cn.src == oldName) cn.src = newName;
        if (cn.dst == oldName) cn.dst = newName;
    }
    for (auto& b : model_.perfBoxes) if (b.organism == oldName) b.organism = newName;
    for (auto& v : model_.views) if (v.organismName == oldName) v.organismName = newName;
    for (auto& av : model_.automationViews) if (av.organismName == oldName) av.organismName = newName;
    for (auto& s : model_.metapad.snapshots)
        for (auto& sc : s.organisms) if (sc.organismName == oldName) sc.organismName = newName;
    for (auto& m : model_.metapad.mask) if (m.organismName == oldName) m.organismName = newName;
    midiMap_.renameOrganism(oldName, newName);
    osc_.renameOrganism(oldName, newName);
    mod_.renameOrganism(oldName, newName);
    auto it = positions_.find(oldName);
    if (it != positions_.end()) { positions_[newName] = it->second; positions_.erase(it); }
    if (graph_ != nullptr)
        if (auto* live = graph_->find(oldName)) live->setName(newName);
}

static bool isAutoName(const std::string& name, const std::string& displayClass) {
    const std::string prefix = displayClass + "_";
    if (name.size() <= prefix.size() || name.compare(0, prefix.size(), prefix) != 0) return false;
    return name.find_first_not_of("0123456789", prefix.size()) == std::string::npos;
}

bool EngineHost::renameOrganism(const std::string& oldName, const std::string& newName) {
    if (oldName == newName) return true;
    if (newName.empty() || model_.byName(newName)) return false;
    pushUndo();
    renameReferences(oldName, newName);
    rebuild();
    return true;
}

std::string EngineHost::replaceOrganism(const std::string& nameIn, const std::string& newClassIn) {
    const std::string newClass = canonicalClass(newClassIn);
    std::string name = nameIn;
    auto* old = model_.byName(name);
    if (!old || old->displayClass == newClass) return name;
    pushUndo();
    const std::string oldDisplay = old->displayClass;

    std::map<std::string, double> kept;
    for (auto& p : old->properties) kept[p.name] = p.value;

    OrganismModel cm;
    cm.name = name;
    cm.classRaw = newClass;
    const auto classId = parseClassString(newClass);
    cm.displayClass = classId.display;
    cm.kind = classId.kind;
    int idx = 0;
    for (const auto& d : schemaFor(newClass)) {
        Parameter p;
        p.index = idx++;
        p.name = d.name;
        p.type = d.isBool ? "bool" : d.isEnum ? "enum" : d.isInt ? "int" : "double";
        auto it = kept.find(d.name);
        p.value = it != kept.end() ? it->second
                                   : rollDefaultValue(d, juce::Random::getSystemRandom());
        cm.properties.push_back(p);
    }
    for (auto& c : model_.organisms) if (c.name == name) { c = std::move(cm); break; }

    OrganismPtr probe;
    if (isPluginKind(parseClassString(newClass).kind)) probe = PluginHost::createOrganism(newClass);
    if (!probe) probe = Registry::instance().create(newClass);
    reclaimCordsForResize(model_.connections, name,
                          probe ? probe->numAudioInputs() : 0,
                          probe ? probe->numAudioOutputs() : 0);
    auto* probeMidi = dynamic_cast<MidiNode*>(probe.get());
    reclaimCordsForResize(model_.midiConnections, name,
                          probeMidi ? probeMidi->numMidiInputs() : 0,
                          probeMidi ? probeMidi->numMidiOutputs() : 0);

    if (isAutoName(name, oldDisplay)) {
        std::string base = classId.display + "_";
        std::string newName;
        for (int n = 1;; ++n) { newName = base + std::to_string(n); if (!model_.byName(newName)) break; }
        renameReferences(name, newName);
        name = newName;
    }

    rebuild();
    return name;
}

namespace {
std::pair<int, int> classIO(const std::string& cls) {
    OrganismPtr c;
    if (isPluginKind(parseClassString(cls).kind)) c = PluginHost::createOrganism(cls);
    if (!c) c = Registry::instance().create(cls);
    return {c ? c->numAudioInputs() : 0, c ? c->numAudioOutputs() : 0};
}

std::pair<int, int> classMidiIO(const std::string& cls) {
    OrganismPtr c;
    if (isPluginKind(parseClassString(cls).kind)) c = PluginHost::createOrganism(cls);
    if (!c) c = Registry::instance().create(cls);
    auto* m = dynamic_cast<MidiNode*>(c.get());
    return {m ? m->numMidiInputs() : 0, m ? m->numMidiOutputs() : 0};
}

std::pair<int, int> classVideoIO(const std::string& cls) {
    OrganismPtr c;
    if (isPluginKind(parseClassString(cls).kind)) c = PluginHost::createOrganism(cls);
    if (!c) c = Registry::instance().create(cls);
    auto* v = dynamic_cast<VideoNode*>(c.get());
    return {v ? v->numVideoInputs() : 0, v ? v->numVideoOutputs() : 0};
}

template <class Remove, class Connect>
void spliceBefore(const std::vector<ConnectionModel>& conns, const std::string& name,
                  const std::string& nn, std::pair<int, int> io, int nameIns,
                  Remove remove, Connect connect) {
    if (io.first <= 0 && io.second <= 0) return;
    std::vector<ConnectionModel> feeders;
    for (const auto& c : conns) if (c.dst == name) feeders.push_back(c);
    for (const auto& f : feeders) {
        remove(f.src, f.srcOutlet, name, f.dstInlet);
        if (io.first > 0) connect(f.src, f.srcOutlet, nn, std::min(f.dstInlet, io.first - 1));
    }
    for (int k = 0; k < std::min(io.second, nameIns); ++k) connect(nn, k, name, k);
}

template <class Remove, class Connect>
void spliceAfter(const std::vector<ConnectionModel>& conns, const std::string& name,
                 const std::string& nn, std::pair<int, int> io, int nameOuts,
                 Remove remove, Connect connect) {
    if (io.first <= 0 && io.second <= 0) return;
    std::vector<ConnectionModel> consumers;
    for (const auto& c : conns) if (c.src == name) consumers.push_back(c);
    for (const auto& u : consumers) {
        remove(name, u.srcOutlet, u.dst, u.dstInlet);
        if (io.second > 0) connect(nn, std::min(u.srcOutlet, io.second - 1), u.dst, u.dstInlet);
    }
    for (int k = 0; k < std::min(nameOuts, io.first); ++k) connect(name, k, nn, k);
}
}

void EngineHost::dropInvalidConnections() {
    auto& cn = model_.connections;
    cn.erase(std::remove_if(cn.begin(), cn.end(), [&](const ConnectionModel& c) {
        auto* s = model_.byName(c.src);
        auto* d = model_.byName(c.dst);
        if (!s || !d) return true;
        return c.srcOutlet >= classIO(s->displayClass).second
            || c.dstInlet  >= classIO(d->displayClass).first;
    }), cn.end());
}

std::string EngineHost::substituteOrganism(const std::string& name, const std::string& newClass) {
    if (!model_.byName(name)) return {};
    beginTransaction();
    const auto pos = position(name);
    const auto nn = addOrganism(newClass, pos, pods::parentOf(name));
    const auto io = classIO(newClass);

    std::vector<ConnectionModel> rerouted;
    for (auto& c : model_.connections) {
        if (c.dst == name && c.dstInlet  < io.first)  rerouted.push_back({c.src, c.srcOutlet, nn, c.dstInlet});
        if (c.src == name && c.srcOutlet < io.second) rerouted.push_back({nn, c.srcOutlet, c.dst, c.dstInlet});
    }
    std::vector<ConnectionModel> old;
    for (auto& c : model_.connections) if (c.src == name || c.dst == name) old.push_back(c);
    for (auto& c : old)      removeConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& c : rerouted) connect(c.src, c.srcOutlet, c.dst, c.dstInlet);

    const auto mio = classMidiIO(newClass);
    std::vector<ConnectionModel> midiRerouted, midiOld;
    for (auto& c : model_.midiConnections) {
        if (c.dst == name && c.dstInlet  < mio.first)  midiRerouted.push_back({c.src, c.srcOutlet, nn, c.dstInlet});
        if (c.src == name && c.srcOutlet < mio.second) midiRerouted.push_back({nn, c.srcOutlet, c.dst, c.dstInlet});
        if (c.src == name || c.dst == name) midiOld.push_back(c);
    }
    for (auto& c : midiOld)      removeMidiConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& c : midiRerouted) connectMidi(c.src, c.srcOutlet, c.dst, c.dstInlet);

    const auto vio = classVideoIO(newClass);
    std::vector<ConnectionModel> videoRerouted, videoOld;
    for (auto& c : model_.videoConnections) {
        if (c.dst == name && c.dstInlet  < vio.first)  videoRerouted.push_back({c.src, c.srcOutlet, nn, c.dstInlet});
        if (c.src == name && c.srcOutlet < vio.second) videoRerouted.push_back({nn, c.srcOutlet, c.dst, c.dstInlet});
        if (c.src == name || c.dst == name) videoOld.push_back(c);
    }
    for (auto& c : videoOld)      removeVideoConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& c : videoRerouted) connectVideo(c.src, c.srcOutlet, c.dst, c.dstInlet);

    setPosition(name, {pos.x + 24, pos.y + 70});
    endTransaction();
    return nn;
}

std::string EngineHost::insertBefore(const std::string& name, const std::string& newClass) {
    if (!model_.byName(name)) return {};
    const int nameIns = inletsOf(name);

    std::vector<ConnectionModel> feeders;
    for (auto& c : model_.connections) if (c.dst == name) feeders.push_back(c);

    const std::string canon = canonicalClass(newClass);
    const auto* cm = PackRegistry::instance().classManifest(canon);
    const auto fam = strips::parse(canon);
    std::vector<std::string> sources;
    for (auto& f : feeders)
        if (std::find(sources.begin(), sources.end(), f.src) == sources.end())
            sources.push_back(f.src);

    if (cm != nullptr && cm->strips && fam.count > 0 && sources.size() >= 2) {
        const int channels = strips::sizeFor(fam, (int) sources.size());
        const std::string sized = strips::sized(fam, channels);
        const int width = std::max(1, classIO(sized).first / std::max(1, channels));

        beginTransaction();
        const auto pos = position(name);
        const auto nn = addOrganism(sized, {pos.x, juce::jmax(0, pos.y - 70)},
                                    pods::parentOf(name));
        for (int s = 0; s < (int) sources.size(); ++s) {
            const int ch = std::min(s, channels - 1);
            std::vector<int> outs;
            for (auto& f : feeders)
                if (f.src == sources[(size_t) s]
                    && std::find(outs.begin(), outs.end(), f.srcOutlet) == outs.end())
                    outs.push_back(f.srcOutlet);
            for (auto& f : feeders)
                if (f.src == sources[(size_t) s])
                    removeConnection(f.src, f.srcOutlet, name, f.dstInlet);
            for (int ci = 0; ci < width; ++ci) {
                const int o = outs.empty() ? 0 : outs[(size_t) std::min(ci, (int) outs.size() - 1)];
                connect(sources[(size_t) s], o, nn, ch * width + ci);
            }
        }
        for (int k = 0; k < std::min(width, nameIns); ++k) connect(nn, k, name, k);
        endTransaction();
        return nn;
    }

    const int midiIns = midiInletsOf(name);
    const int videoIns = videoInletsOf(name);
    beginTransaction();
    const auto pos = position(name);
    const auto nn = addOrganism(newClass, {pos.x, juce::jmax(0, pos.y - 70)},
                                pods::parentOf(name));
    auto cutAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        removeConnection(a, b, c, d);
    };
    auto joinAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        connect(a, b, c, d);
    };
    auto cutMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        removeMidiConnection(a, b, c, d);
    };
    auto joinMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        connectMidi(a, b, c, d);
    };
    auto cutVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        removeVideoConnection(a, b, c, d);
    };
    auto joinVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        connectVideo(a, b, c, d);
    };
    spliceBefore(model_.connections, name, nn, classIO(newClass), nameIns, cutAudio, joinAudio);
    spliceBefore(model_.midiConnections, name, nn, classMidiIO(newClass), midiIns, cutMidi,
                 joinMidi);
    spliceBefore(model_.videoConnections, name, nn, classVideoIO(newClass), videoIns, cutVideo,
                 joinVideo);
    endTransaction();
    return nn;
}

std::string EngineHost::insertAfter(const std::string& name, const std::string& newClass) {
    if (!model_.byName(name)) return {};
    const int nameOuts = outletsOf(name);
    const int midiOuts = midiOutletsOf(name);
    const int videoOuts = videoOutletsOf(name);
    beginTransaction();
    const auto pos = position(name);
    const auto nn = addOrganism(newClass, {pos.x, pos.y + 70},
                                pods::parentOf(name));
    auto cutAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        removeConnection(a, b, c, d);
    };
    auto joinAudio = [this](const std::string& a, int b, const std::string& c, int d) {
        connect(a, b, c, d);
    };
    auto cutMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        removeMidiConnection(a, b, c, d);
    };
    auto joinMidi = [this](const std::string& a, int b, const std::string& c, int d) {
        connectMidi(a, b, c, d);
    };
    auto cutVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        removeVideoConnection(a, b, c, d);
    };
    auto joinVideo = [this](const std::string& a, int b, const std::string& c, int d) {
        connectVideo(a, b, c, d);
    };
    spliceAfter(model_.connections, name, nn, classIO(newClass), nameOuts, cutAudio, joinAudio);
    spliceAfter(model_.midiConnections, name, nn, classMidiIO(newClass), midiOuts, cutMidi,
                joinMidi);
    spliceAfter(model_.videoConnections, name, nn, classVideoIO(newClass), videoOuts, cutVideo,
                joinVideo);
    endTransaction();
    return nn;
}

std::string EngineHost::insertRecorderFor(const std::string& node) {
    if (!model_.byName(node)) return {};
    std::string tk;
    if (outletsOf(node) > 0)      tk = insertAfter(node, "AudioTrack");
    else if (inletsOf(node) > 0)  tk = insertBefore(node, "AudioTrack");
    if (!tk.empty()) setParam(tk, "Record", 1.0);
    return tk;
}

std::string EngineHost::insertOnCord(const std::string& src, int outlet, const std::string& dst,
                                     int inlet, const std::string& newClass, juce::Point<int> at,
                                     const std::string& podScope) {
    beginTransaction();
    const auto nn = addOrganism(newClass, at, podScope);
    const auto io = classIO(newClass);
    removeConnection(src, outlet, dst, inlet);
    if (io.first  > 0) connect(src, outlet, nn, 0);
    if (io.second > 0) connect(nn, 0, dst, inlet);
    endTransaction();
    return nn;
}

std::string EngineHost::createPod(int ins, int outs, juce::Point<int> at,
                                  const std::string& scope, bool stereoPairs,
                                  int midiIns, int midiOuts) {
    ins = juce::jlimit(0, 16, ins);
    midiIns = juce::jlimit(0, 8, midiIns);
    midiOuts = juce::jlimit(0, 8, midiOuts);
    outs = juce::jlimit(1, 16, outs);
    beginTransaction();
    const std::string base = (scope.empty() ? std::string() : scope + "/") + "Pod_";
    std::string pod;
    for (int n = 1;; ++n) {
        pod = base + std::to_string(n);
        if (!model_.byName(pod) && !pods::isPod(model_, pod)) break;
    }
    auto place = [&](int channels, bool inletSide, int y) {
        int x = 40;
        for (; channels > 0; x += 160) {
            const bool st = stereoPairs && channels >= 2;
            addOrganism(inletSide ? (st ? pods::kInletStereoClass : pods::kInletClass)
                                     : (st ? pods::kOutletStereoClass : pods::kOutletClass),
                           {x, y}, pod);
            channels -= st ? 2 : 1;
        }
        return x;
    };
    const int inX = place(ins, true, 40);
    const int outX = place(outs, false, 420);
    auto placeMidi = [&](int count, bool inletSide, int y, int x) {
        x += 40;
        for (; count > 0; x += 160, --count)
            addOrganism(inletSide ? pods::kMidiInletClass : pods::kMidiOutletClass,
                           {x, y}, pod);
    };
    placeMidi(midiIns, true, 40, inX);
    placeMidi(midiOuts, false, 420, outX);
    positions_[pod] = at;
    endTransaction();
    return pod;
}

void EngineHost::promotePodStrays(const std::string& pod, int domain,
                                  juce::Point<int> topLeft, juce::Point<int> bottomLeft) {
    const bool midi = domain == 1, video = domain == 2;
    auto& cords = video ? model_.videoConnections
                        : midi ? model_.midiConnections : model_.connections;
    const auto dom = video ? pods::Domain::Video
                           : midi ? pods::Domain::Midi : pods::Domain::Audio;
    for (const bool inletSide : {true, false}) {
        const auto strays = pods::strayRefs(model_, pod, inletSide, dom);
        int k = 0;
        for (const auto& ref : strays) {
            const juce::Point<int> base = inletSide ? topLeft : bottomLeft;
            const char* cls =
                video ? (inletSide ? pods::kVideoInletClass : pods::kVideoOutletClass)
                : midi ? (inletSide ? pods::kMidiInletClass : pods::kMidiOutletClass)
                       : (inletSide ? pods::kInletClass : pods::kOutletClass);
            const auto port = addOrganism(cls, base + juce::Point<int>{160 * k++, 0}, pod);
            if (port.empty()) continue;
            for (auto& c : cords) {
                if (inletSide && c.dst == ref.node && c.dstInlet == ref.chan
                    && !pods::isUnder(c.src, pod)) {
                    c.dst = port;
                    c.dstInlet = 0;
                } else if (!inletSide && c.src == ref.node && c.srcOutlet == ref.chan
                           && !pods::isUnder(c.dst, pod)) {
                    c.src = port;
                    c.srcOutlet = 0;
                }
            }
            ConnectionModel inner;
            if (inletSide) { inner.src = port; inner.dst = ref.node; inner.dstInlet = ref.chan; }
            else           { inner.src = ref.node; inner.srcOutlet = ref.chan; inner.dst = port; }
            cords.push_back(inner);
        }
    }
}

std::string EngineHost::makePod(const std::vector<std::string>& nodes, juce::Point<int> at,
                                const std::string& scope) {
    std::vector<std::string> members;
    for (const auto& n : nodes) {
        if (!pods::inScope(n, scope)) continue;
        if (model_.byName(n) != nullptr || pods::isPod(model_, n)) members.push_back(n);
    }
    if (members.empty()) return {};

    const std::string base = (scope.empty() ? std::string() : scope + "/") + "Pod_";
    std::string pod;
    for (int n = 1;; ++n) {
        pod = base + std::to_string(n);
        if (!model_.byName(pod) && !pods::isPod(model_, pod)) break;
    }

    beginTransaction();
    pushUndo();
    juce::Rectangle<int> bounds;
    bool first = true;
    auto grow = [&](juce::Point<int> p) {
        const juce::Rectangle<int> r(p.x, p.y, 1, 1);
        bounds = first ? r : bounds.getUnion(r);
        first = false;
    };
    for (const auto& n : members) {
        grow(position(n));
        const std::string pre = n + "/";
        for (const auto& c : model_.organisms)
            if (c.name.rfind(pre, 0) == 0) grow(position(c.name));
    }

    std::vector<std::pair<std::string, std::string>> moves;
    for (const auto& n : members) {
        const std::string leaf = pods::leafOf(n);
        moves.emplace_back(n, pod + "/" + leaf);
        const std::string pre = n + "/";
        for (const auto& c : model_.organisms)
            if (c.name.rfind(pre, 0) == 0)
                moves.emplace_back(c.name, pod + "/" + leaf + "/" + c.name.substr(pre.size()));
    }
    for (const auto& m : moves) renameReferences(m.first, m.second);
    for (const auto& n : members)
        if (auto it = positions_.find(n); it != positions_.end()) {
            positions_[pod + "/" + pods::leafOf(n)] = it->second;
            positions_.erase(it);
        }
    positions_[pod] = at;

    const juce::Point<int> above{bounds.getX(), bounds.getY() - 140};
    const juce::Point<int> below{bounds.getX(), bounds.getBottom() + 140};
    promotePodStrays(pod, 0, above, below);
    promotePodStrays(pod, 1, above + juce::Point<int>{0, -80}, below + juce::Point<int>{0, 80});
    promotePodStrays(pod, 2, above + juce::Point<int>{0, -160}, below + juce::Point<int>{0, 160});
    endTransaction();
    return pod;
}

bool EngineHost::renamePod(const std::string& pod, const std::string& newLeaf) {
    if (newLeaf.empty() || newLeaf.find(pods::kSep) != std::string::npos) return false;
    const std::string parent = pods::parentOf(pod);
    const std::string target = parent.empty() ? newLeaf : parent + "/" + newLeaf;
    if (target == pod) return true;
    if (model_.byName(target) || pods::isPod(model_, target)) return false;
    pushUndo();
    const std::string pre = pod + "/";
    std::vector<std::string> inner;
    for (auto& c : model_.organisms)
        if (c.name.rfind(pre, 0) == 0) inner.push_back(c.name);
    for (auto& n : inner)
        renameReferences(n, target + "/" + n.substr(pre.size()));
    if (auto it = positions_.find(pod); it != positions_.end()) {
        positions_[target] = it->second;
        positions_.erase(it);
    }
    rebuild();
    return true;
}

void EngineHost::splicePodPorts(const std::string& pod, int domain) {
    const bool midi = domain == 1, video = domain == 2;
    auto& cords = video ? model_.videoConnections
                        : midi ? model_.midiConnections : model_.connections;
    std::vector<std::pair<std::string, int>> ports;
    for (const auto& c : model_.organisms) {
        if (pods::parentOf(c.name) != pod) continue;
        if (!pods::isInletClass(c.displayClass) && !pods::isOutletClass(c.displayClass)) continue;
        const bool isMidi = pods::isMidiPortClass(c.displayClass);
        const bool isVideo = pods::isVideoPortClass(c.displayClass);
        if (midi != isMidi || video != isVideo) continue;
        ports.emplace_back(c.name, midi || video ? 1 : pods::portChannels(c.displayClass));
    }

    for (const auto& [port, channels] : ports) {
        std::vector<ConnectionModel> spliced;
        for (int k = 0; k < channels; ++k) {
            std::vector<const ConnectionModel*> in, out;
            for (const auto& c : cords) {
                if (c.dst == port && c.dstInlet == k) in.push_back(&c);
                if (c.src == port && c.srcOutlet == k) out.push_back(&c);
            }
            for (const auto* a : in)
                for (const auto* b : out) {
                    ConnectionModel n;
                    n.src = a->src;
                    n.srcOutlet = a->srcOutlet;
                    n.dst = b->dst;
                    n.dstInlet = b->dstInlet;
                    if (midi) {
                        if (a->midiChannel != 0 && b->midiChannel != 0
                            && a->midiChannel != b->midiChannel) continue;
                        n.midiChannel = a->midiChannel != 0 ? a->midiChannel : b->midiChannel;
                    }
                    spliced.push_back(n);
                }
        }
        cords.erase(std::remove_if(cords.begin(), cords.end(),
                                   [&p = port](const ConnectionModel& c) {
                                       return c.src == p || c.dst == p;
                                   }),
                    cords.end());
        for (auto& n : spliced) cords.push_back(std::move(n));
        auto& cs = model_.organisms;
        cs.erase(std::remove_if(cs.begin(), cs.end(),
                                [&p = port](const OrganismModel& c) { return c.name == p; }),
                 cs.end());
        positions_.erase(port);
    }
}

void EngineHost::ungroupPod(const std::string& pod) {
    if (!pods::isPod(model_, pod)) return;
    beginTransaction();
    pushUndo();
    for (int dom = 0; dom < 3; ++dom) splicePodPorts(pod, dom);

    const std::string scope = pods::parentOf(pod);
    const std::string prefix = scope.empty() ? std::string() : scope + "/";
    const std::string pre = pod + "/";
    std::vector<std::string> directChildren;
    for (const auto& c : model_.organisms) {
        if (c.name.rfind(pre, 0) != 0) continue;
        const auto rest = c.name.substr(pre.size());
        const auto i = rest.find(pods::kSep);
        const auto seg = i == std::string::npos ? rest : rest.substr(0, i);
        if (std::find(directChildren.begin(), directChildren.end(), seg) == directChildren.end())
            directChildren.push_back(seg);
    }
    std::vector<std::pair<std::string, std::string>> moves;
    for (const auto& seg : directChildren) {
        std::string target = prefix + seg;
        for (int n = 2; model_.byName(target) != nullptr || pods::isPod(model_, target); ++n)
            target = prefix + seg + "_" + std::to_string(n);
        const std::string from = pre + seg;
        for (const auto& c : model_.organisms) {
            if (c.name == from) moves.emplace_back(c.name, target);
            else if (c.name.rfind(from + "/", 0) == 0)
                moves.emplace_back(c.name, target + c.name.substr(from.size()));
        }
    }
    for (const auto& m : moves) renameReferences(m.first, m.second);
    positions_.erase(pod);
    endTransaction();
    rebuild();
}

void EngineHost::deletePod(const std::string& pod) {
    pushUndo();
    const std::string pre = pod + "/";
    auto under = [&](const std::string& n) { return n.rfind(pre, 0) == 0; };
    auto& cs = model_.organisms;
    cs.erase(std::remove_if(cs.begin(), cs.end(),
                            [&](const OrganismModel& c) { return under(c.name); }), cs.end());
    for (auto* list : {&model_.connections, &model_.midiConnections, &model_.videoConnections})
        list->erase(std::remove_if(list->begin(), list->end(),
                                   [&](const ConnectionModel& c) {
                                       return under(c.src) || under(c.dst);
                                   }),
                    list->end());
    for (auto it = positions_.begin(); it != positions_.end();)
        it = (under(it->first) || it->first == pod) ? positions_.erase(it) : std::next(it);
    rebuild();
}

EngineHost::PodClip EngineHost::capturePod(const std::string& pod) const {
    PodClip k;
    k.leaf = pods::leafOf(pod);
    const std::string pre = pod + "/";
    auto under = [&](const std::string& n) { return n.rfind(pre, 0) == 0; };
    auto rel = [&](const std::string& n) { return n.substr(pre.size()); };
    for (const auto& c : model_.organisms)
        if (under(c.name)) {
            auto m = c;
            m.name = rel(c.name);
            k.nodes.push_back(std::move(m));
        }
    auto capture = [&](const std::vector<ConnectionModel>& from,
                       std::vector<ConnectionModel>& to) {
        for (const auto& c : from)
            if (under(c.src) && under(c.dst)) {
                auto c2 = c;
                c2.src = rel(c.src);
                c2.dst = rel(c.dst);
                to.push_back(std::move(c2));
            }
    };
    capture(model_.connections, k.cords);
    capture(model_.midiConnections, k.midiCords);
    capture(model_.videoConnections, k.videoCords);
    for (const auto& [n, p] : positions_)
        if (under(n)) k.layout.emplace_back(rel(n), p);
    if (auto it = positions_.find(pod); it != positions_.end()) k.boxPos = it->second;
    return k;
}

std::string EngineHost::pastePod(const PodClip& clip, juce::Point<int> at,
                                 const std::string& scope) {
    if (clip.nodes.empty()) return {};
    pushUndo();
    const std::string want = (scope.empty() ? std::string() : scope + "/") + clip.leaf;
    std::string pod = want;
    for (int n = 2; model_.byName(pod) || pods::isPod(model_, pod); ++n)
        pod = want + "_" + std::to_string(n);
    for (const auto& m : clip.nodes) {
        auto c = m;
        c.name = pod + "/" + m.name;
        model_.organisms.push_back(std::move(c));
    }
    for (const auto& cn : clip.cords) {
        auto c = cn;
        c.src = pod + "/" + cn.src;
        c.dst = pod + "/" + cn.dst;
        model_.connections.push_back(std::move(c));
    }
    for (const auto& cn : clip.midiCords) {
        auto c = cn;
        c.src = pod + "/" + cn.src;
        c.dst = pod + "/" + cn.dst;
        model_.midiConnections.push_back(std::move(c));
    }
    for (const auto& cn : clip.videoCords) {
        auto c = cn;
        c.src = pod + "/" + cn.src;
        c.dst = pod + "/" + cn.dst;
        model_.videoConnections.push_back(std::move(c));
    }
    for (const auto& [n, p] : clip.layout) positions_[pod + "/" + n] = p;
    positions_[pod] = at;
    rebuild();
    return pod;
}

void EngineHost::disconnectPod(const std::string& pod) {
    const auto ins = pods::inletRefs(model_, pod);
    const auto outs = pods::outletRefs(model_, pod);
    if (ins.empty() && outs.empty()) return;
    beginTransaction();
    const std::string pre = pod + "/";
    auto external = [&](const std::string& n) { return n.rfind(pre, 0) != 0; };
    const int n = (int) std::max(ins.size(), outs.size());
    std::vector<ConnectionModel> bridges, old;
    for (int k = 0; k < n; ++k) {
        std::vector<std::pair<std::string, int>> srcs, dsts;
        for (auto& c : model_.connections) {
            if (k < (int) ins.size() && c.dst == ins[(size_t) k].node
                && c.dstInlet == ins[(size_t) k].chan && external(c.src))
                srcs.push_back({c.src, c.srcOutlet});
            if (k < (int) outs.size() && c.src == outs[(size_t) k].node
                && c.srcOutlet == outs[(size_t) k].chan && external(c.dst))
                dsts.push_back({c.dst, c.dstInlet});
        }
        for (auto& s : srcs)
            for (auto& d : dsts) bridges.push_back({s.first, s.second, d.first, d.second});
    }
    auto isBoundary = [](const std::vector<pods::PortRef>& refs, const std::string& node) {
        for (const auto& r : refs) if (r.node == node) return true;
        return false;
    };
    for (auto& c : model_.connections) {
        const bool in = isBoundary(ins, c.dst) && external(c.src);
        const bool out = isBoundary(outs, c.src) && external(c.dst);
        if (in || out) old.push_back(c);
    }
    for (auto& c : old) removeConnection(c.src, c.srcOutlet, c.dst, c.dstInlet);
    for (auto& b : bridges) connect(b.src, b.srcOutlet, b.dst, b.dstInlet);
    endTransaction();
}

std::string EngineHost::insertBeforePod(const std::string& pod, const std::string& newClass) {
    const auto ins = pods::declaredRefs(model_, pod, true);
    if (ins.empty()) return {};
    beginTransaction();
    const auto pos = position(pod);
    const auto nn = addOrganism(newClass, {pos.x, juce::jmax(0, pos.y - 70)},
                                   pods::parentOf(pod));
    const auto io = classIO(newClass);
    const std::string pre = pod + "/";
    auto external = [&](const std::string& n) { return n.rfind(pre, 0) != 0; };
    struct Feeder { std::string src; int srcOutlet; int chan; };
    std::vector<Feeder> feeders;
    for (int k = 0; k < (int) ins.size(); ++k)
        for (auto& c : model_.connections)
            if (c.dst == ins[(size_t) k].node && c.dstInlet == ins[(size_t) k].chan
                && external(c.src))
                feeders.push_back({c.src, c.srcOutlet, k});
    for (auto& f : feeders) {
        const auto& r = ins[(size_t) f.chan];
        removeConnection(f.src, f.srcOutlet, r.node, r.chan);
        if (io.first > 0) connect(f.src, f.srcOutlet, nn, std::min(f.chan, io.first - 1));
    }
    for (int k = 0; k < std::min(io.second, (int) ins.size()); ++k)
        connect(nn, k, ins[(size_t) k].node, ins[(size_t) k].chan);
    endTransaction();
    return nn;
}

std::string EngineHost::insertAfterPod(const std::string& pod, const std::string& newClass) {
    const auto outs = pods::declaredRefs(model_, pod, false);
    if (outs.empty()) return {};
    beginTransaction();
    const auto pos = position(pod);
    const auto nn = addOrganism(newClass, {pos.x, pos.y + 70}, pods::parentOf(pod));
    const auto io = classIO(newClass);
    const std::string pre = pod + "/";
    auto external = [&](const std::string& n) { return n.rfind(pre, 0) != 0; };
    struct Consumer { std::string dst; int dstInlet; int chan; };
    std::vector<Consumer> consumers;
    for (int k = 0; k < (int) outs.size(); ++k)
        for (auto& c : model_.connections)
            if (c.src == outs[(size_t) k].node && c.srcOutlet == outs[(size_t) k].chan
                && external(c.dst))
                consumers.push_back({c.dst, c.dstInlet, k});
    for (auto& u : consumers) {
        const auto& r = outs[(size_t) u.chan];
        removeConnection(r.node, r.chan, u.dst, u.dstInlet);
        if (io.second > 0) connect(nn, std::min(u.chan, io.second - 1), u.dst, u.dstInlet);
    }
    for (int k = 0; k < std::min(io.first, (int) outs.size()); ++k)
        connect(outs[(size_t) k].node, outs[(size_t) k].chan, nn, k);
    endTransaction();
    return nn;
}

void EngineHost::swapOrganisms(const std::string& a, const std::string& b) {
    if (a == b || !model_.byName(a) || !model_.byName(b)) return;
    beginTransaction();
    pushUndo();
    const auto pa = position(a), pb = position(b);
    setPosition(a, pb);
    setPosition(b, pa);
    for (auto& c : model_.connections) {
        if (c.src == a) c.src = b; else if (c.src == b) c.src = a;
        if (c.dst == a) c.dst = b; else if (c.dst == b) c.dst = a;
    }
    dropInvalidConnections();
    rebuild();
    endTransaction();
}

}
