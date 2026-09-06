#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "io/PatchDocument.h"

namespace hum::pods {

inline constexpr char kSep = '/';
inline constexpr const char* kInletClass = "PodIn";
inline constexpr const char* kOutletClass = "PodOut";
inline constexpr const char* kInletStereoClass = "SPodIn";
inline constexpr const char* kOutletStereoClass = "SPodOut";
inline constexpr const char* kMidiInletClass = "PodMidiIn";
inline constexpr const char* kMidiOutletClass = "PodMidiOut";

inline constexpr const char* kVideoInletClass = "PodVideoIn";
inline constexpr const char* kVideoOutletClass = "PodVideoOut";

inline bool isInletClass(const std::string& c) {
    return c == kInletClass || c == kInletStereoClass || c == kMidiInletClass
        || c == kVideoInletClass
        || c == "PodInlet" || c == "SPodInlet" || c == "PodMidiInlet";
}
inline bool isOutletClass(const std::string& c) {
    return c == kOutletClass || c == kOutletStereoClass || c == kMidiOutletClass
        || c == kVideoOutletClass
        || c == "PodOutlet" || c == "SPodOutlet" || c == "PodMidiOutlet";
}
inline bool isMidiPortClass(const std::string& c) {
    return c == kMidiInletClass || c == kMidiOutletClass
        || c == "PodMidiInlet" || c == "PodMidiOutlet";
}
inline bool isVideoPortClass(const std::string& c) {
    return c == kVideoInletClass || c == kVideoOutletClass;
}
inline int portChannels(const std::string& c) {
    if (isMidiPortClass(c) || isVideoPortClass(c)) return 0;
    return (c == kInletStereoClass || c == kOutletStereoClass
            || c == "SPodInlet" || c == "SPodOutlet")
               ? 2
               : 1;
}

inline std::string parentOf(const std::string& name) {
    const auto i = name.rfind(kSep);
    return i == std::string::npos ? std::string() : name.substr(0, i);
}
inline std::string leafOf(const std::string& name) {
    const auto i = name.rfind(kSep);
    return i == std::string::npos ? name : name.substr(i + 1);
}
inline bool inScope(const std::string& name, const std::string& scope) {
    return parentOf(name) == scope;
}
inline std::string childPodOf(const std::string& name, const std::string& scope) {
    if (scope.empty()) {
        const auto i = name.find(kSep);
        return i == std::string::npos ? std::string() : name.substr(0, i);
    }
    const std::string pre = scope + kSep;
    if (name.rfind(pre, 0) != 0) return {};
    const auto i = name.find(kSep, pre.size());
    return i == std::string::npos ? std::string() : name.substr(0, i);
}

inline bool isPod(const PatchDocumentModel& m, const std::string& name) {
    if (name.empty()) return false;
    const std::string pre = name + kSep;
    for (const auto& c : m.organisms)
        if (c.name.rfind(pre, 0) == 0) return true;
    return false;
}

inline std::vector<std::string> podsIn(const PatchDocumentModel& m, const std::string& scope) {
    std::vector<std::string> out;
    for (const auto& c : m.organisms) {
        const auto p = childPodOf(c.name, scope);
        if (!p.empty() && std::find(out.begin(), out.end(), p) == out.end())
            out.push_back(p);
    }
    return out;
}

inline std::vector<std::string> ports(const PatchDocumentModel& m, const std::string& pod,
                                      bool inletSide) {
    std::vector<std::string> out;
    for (const auto& c : m.organisms)
        if (parentOf(c.name) == pod
            && (inletSide ? isInletClass(c.displayClass) : isOutletClass(c.displayClass)))
            out.push_back(c.name);
    auto num = [](const std::string& s) {
        const auto i = s.find_last_not_of("0123456789");
        return i + 1 < s.size() ? std::atoi(s.c_str() + i + 1) : 0;
    };
    std::sort(out.begin(), out.end(), [&](const std::string& a, const std::string& b) {
        const int na = num(a), nb = num(b);
        return na != nb ? na < nb : a < b;
    });
    return out;
}
inline std::vector<std::string> inlets(const PatchDocumentModel& m, const std::string& pod) {
    return ports(m, pod, true);
}
inline std::vector<std::string> outlets(const PatchDocumentModel& m, const std::string& pod) {
    return ports(m, pod, false);
}
inline int indexOfPort(const std::vector<std::string>& list, const std::string& name) {
    const auto it = std::find(list.begin(), list.end(), name);
    return it == list.end() ? -1 : (int) (it - list.begin());
}

struct PortRef {
    std::string node;
    int chan = 0;
};
inline int indexOfRef(const std::vector<PortRef>& list, const std::string& node, int chan) {
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i].node == node && list[i].chan == chan) return (int) i;
    return -1;
}

inline bool isUnder(const std::string& name, const std::string& pod) {
    const std::string pre = pod + kSep;
    return name.rfind(pre, 0) == 0;
}

inline std::vector<PortRef> declaredRefs(const PatchDocumentModel& m, const std::string& pod,
                                         bool inletSide) {
    std::vector<PortRef> out;
    for (const auto& n : ports(m, pod, inletSide)) {
        int ch = 1;
        for (const auto& c : m.organisms)
            if (c.name == n) { ch = portChannels(c.displayClass); break; }
        for (int k = 0; k < ch; ++k) out.push_back({n, k});
    }
    return out;
}
inline std::vector<PortRef> videoDeclaredRefs(const PatchDocumentModel& m, const std::string& pod,
                                             bool inletSide) {
    std::vector<PortRef> out;
    for (const auto& n : ports(m, pod, inletSide))
        for (const auto& c : m.organisms)
            if (c.name == n) {
                if (isVideoPortClass(c.displayClass)) out.push_back({n, 0});
                break;
            }
    return out;
}
inline std::vector<PortRef> midiDeclaredRefs(const PatchDocumentModel& m, const std::string& pod,
                                             bool inletSide) {
    std::vector<PortRef> out;
    for (const auto& n : ports(m, pod, inletSide))
        for (const auto& c : m.organisms)
            if (c.name == n) {
                if (isMidiPortClass(c.displayClass)) out.push_back({n, 0});
                break;
            }
    return out;
}

enum class Domain { Audio, Midi, Video };
inline const std::vector<ConnectionModel>& cordsOf(const PatchDocumentModel& m, Domain dom) {
    return dom == Domain::Midi    ? m.midiConnections
           : dom == Domain::Video ? m.videoConnections
                                  : m.connections;
}
inline std::vector<PortRef> strayRefs(const PatchDocumentModel& m, const std::string& pod,
                                      bool inletSide, Domain dom = Domain::Audio) {
    const std::vector<PortRef> declared =
        dom == Domain::Midi    ? midiDeclaredRefs(m, pod, inletSide)
        : dom == Domain::Video ? videoDeclaredRefs(m, pod, inletSide)
                               : declaredRefs(m, pod, inletSide);
    std::vector<PortRef> out;
    auto note = [&](const std::string& node, int chan) {
        if (indexOfRef(declared, node, chan) >= 0) return;
        if (indexOfRef(out, node, chan) >= 0) return;
        out.push_back({node, chan});
    };
    for (const auto& cn : cordsOf(m, dom)) {
        const bool si = isUnder(cn.src, pod), di = isUnder(cn.dst, pod);
        if (si == di) continue;
        if (si && !inletSide) note(cn.src, cn.srcOutlet);
        if (di && inletSide) note(cn.dst, cn.dstInlet);
    }
    return out;
}
inline int strayCount(const PatchDocumentModel& m, const std::string& pod, bool inletSide,
                      Domain dom = Domain::Audio) {
    return (int) strayRefs(m, pod, inletSide, dom).size();
}

inline std::vector<PortRef> portRefs(const PatchDocumentModel& m, const std::string& pod,
                                     bool inletSide) {
    auto out = declaredRefs(m, pod, inletSide);
    for (auto& r : strayRefs(m, pod, inletSide, Domain::Audio)) out.push_back(std::move(r));
    return out;
}
inline std::vector<PortRef> inletRefs(const PatchDocumentModel& m, const std::string& pod) {
    return portRefs(m, pod, true);
}
inline std::vector<PortRef> outletRefs(const PatchDocumentModel& m, const std::string& pod) {
    return portRefs(m, pod, false);
}

inline std::vector<PortRef> midiPortRefs(const PatchDocumentModel& m, const std::string& pod,
                                         bool inletSide) {
    auto out = midiDeclaredRefs(m, pod, inletSide);
    for (auto& r : strayRefs(m, pod, inletSide, Domain::Midi)) out.push_back(std::move(r));
    return out;
}
inline std::vector<PortRef> videoPortRefs(const PatchDocumentModel& m, const std::string& pod,
                                          bool inletSide) {
    auto out = videoDeclaredRefs(m, pod, inletSide);
    for (auto& r : strayRefs(m, pod, inletSide, Domain::Video)) out.push_back(std::move(r));
    return out;
}
struct Surface {
    enum Kind { None, Self, Box };
    Kind kind = None;
    std::string box;
    int pin = -1;
};
inline Surface surfaceOf(const PatchDocumentModel& m, const std::string& node, int port,
                         bool isDstSide, const std::string& scope, Domain dom = Domain::Audio) {
    if (inScope(node, scope)) return {Surface::Self, {}, port};
    const auto pod = childPodOf(node, scope);
    if (pod.empty()) return {};
    const auto list = dom == Domain::Midi    ? midiPortRefs(m, pod, isDstSide)
                      : dom == Domain::Video ? videoPortRefs(m, pod, isDstSide)
                                             : portRefs(m, pod, isDstSide);
    const int i = indexOfRef(list, node, port);
    if (i < 0) return {};
    return {Surface::Box, pod, i};
}
inline bool drawnAt(const PatchDocumentModel& m, const ConnectionModel& c,
                    const std::string& scope, Domain dom = Domain::Audio) {
    const auto s = surfaceOf(m, c.src, c.srcOutlet, false, scope, dom);
    const auto d = surfaceOf(m, c.dst, c.dstInlet, true, scope, dom);
    if (s.kind == Surface::None || d.kind == Surface::None) return false;
    const std::string sn = s.kind == Surface::Box ? s.box : c.src;
    const std::string dn = d.kind == Surface::Box ? d.box : c.dst;
    return sn != dn;
}
inline int strayTotal(const PatchDocumentModel& m);

inline bool resolvePodVideoCord(const PatchDocumentModel& m, ConnectionModel& c) {
    bool changed = false;
    if (isPod(m, c.src)) {
        const auto list = videoPortRefs(m, c.src, false);
        if (c.srcOutlet >= 0 && c.srcOutlet < (int) list.size()) {
            c.src = list[(size_t) c.srcOutlet].node;
            c.srcOutlet = list[(size_t) c.srcOutlet].chan;
            changed = true;
        }
    }
    if (isPod(m, c.dst)) {
        const auto list = videoPortRefs(m, c.dst, true);
        if (c.dstInlet >= 0 && c.dstInlet < (int) list.size()) {
            c.dst = list[(size_t) c.dstInlet].node;
            c.dstInlet = list[(size_t) c.dstInlet].chan;
            changed = true;
        }
    }
    return changed;
}

inline void resolvePodVideoCords(PatchDocumentModel& m) {
    for (auto& c : m.videoConnections) resolvePodVideoCord(m, c);
}

inline std::vector<std::string> allScopes(const PatchDocumentModel& m) {
    std::vector<std::string> out{""};
    for (size_t i = 0; i < out.size(); ++i)
        for (const auto& p : podsIn(m, out[i])) out.push_back(p);
    return out;
}
inline int strayTotal(const PatchDocumentModel& m) {
    int n = 0;
    for (const auto& pod : allScopes(m)) {
        if (pod.empty()) continue;
        for (auto dom : {Domain::Audio, Domain::Midi, Domain::Video})
            n += strayCount(m, pod, true, dom) + strayCount(m, pod, false, dom);
    }
    return n;
}

inline std::vector<PortRef> midiInletRefs(const PatchDocumentModel& m, const std::string& pod) {
    return midiPortRefs(m, pod, true);
}
inline std::vector<PortRef> midiOutletRefs(const PatchDocumentModel& m, const std::string& pod) {
    return midiPortRefs(m, pod, false);
}

}
