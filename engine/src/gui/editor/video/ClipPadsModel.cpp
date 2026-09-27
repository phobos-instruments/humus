// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "gui/editor/video/ClipPadsModel.h"

#include <array>
#include <cmath>
#include <string>
#include <utility>

namespace hum::video {

void ClipPadsModel::launch(int i) {
    host_.setParam(organism_, p_.launch + suffix(i), 1.0);
    release_[(size_t) i] = true;
}

bool ClipPadsModel::releaseLaunch(int i) {
    if (!release_[(size_t) i]) return false;
    release_[(size_t) i] = false;
    host_.setParam(organism_, p_.launch + suffix(i), 0.0);
    return true;
}

void ClipPadsModel::playPause(int i, PlaybackTarget* target) {
    auto* p = pads();
    if (p != nullptr && p->clipState().active == i && target != nullptr) target->setPaused(!target->isPaused());
    else launch(i);
}

void ClipPadsModel::clear(int i) {
    host_.beginTransaction();
    host_.setParamText(organism_, p_.file + suffix(i), {});
    host_.setParam(organism_, p_.in + suffix(i), 0.0);
    host_.setParam(organism_, p_.out + suffix(i), 0.0);
    host_.setParam(organism_, p_.loop + suffix(i), 1.0);
    host_.endTransaction();
}

void ClipPadsModel::copy(int a, int b, bool move) {
    if (a == b || a < 0 || b < 0 || a >= kPads || b >= kPads) return;
    struct Pad {
        std::string file;
        double in, out, loop;
    };
    auto read = [this](int i) {
        return Pad{file(i), in(i), out(i), host_.liveParamValue(organism_, p_.loop + suffix(i))};
    };
    auto writeRange = [this](int i, const Pad& pad) {
        host_.setParam(organism_, p_.in + suffix(i), pad.in);
        host_.setParam(organism_, p_.out + suffix(i), pad.out);
        host_.setParam(organism_, p_.loop + suffix(i), pad.loop);
    };
    const Pad from = read(a), to = read(b);
    host_.beginTransaction();
    host_.setParamText(organism_, p_.file + suffix(b), from.file);
    if (move) host_.setParamText(organism_, p_.file + suffix(a), to.file);
    writeRange(b, from);
    if (move) writeRange(a, to);
    host_.endTransaction();
}

void ClipPadsModel::take(int i, const PadRange& r) {
    if (r.file.empty()) return;
    host_.setParamText(organism_, p_.file + suffix(i), r.file);
    setIn(i, r.inSeconds);
    setOut(i, r.outSeconds);
    host_.setParam(organism_, p_.loop + suffix(i), r.looped ? 1.0 : 0.0);
}

files::FilePick ClipPadsModel::request(int i, const std::string& videosDir) const {
    files::FilePick pick;
    pick.title = "Load a clip";
    const auto cur = file(i);
    pick.startDir = !cur.empty() ? files::parentOf(cur) : videosDir;
    pick.patterns = "*.mov;*.mp4;*.m4v;*.avi";
    pick.current = cur;
    return pick;
}

void ClipPadsModel::armPark(int i, double seconds) {
    parkIn_[(size_t) i] = seconds;
    parkWant_[(size_t) i] = seconds;
    parkTries_[(size_t) i] = 0;
}

ClipPadsModel::Park ClipPadsModel::parkStep(int i, bool hasFrame, double framePts) {
    const double want = parkWant_[(size_t) i];
    if (want < 0.0 || ++parkTries_[(size_t) i] > kParkTries) return Park::Idle;
    if (hasFrame && framePts >= 0.0 && std::abs(framePts - want) < 0.2) {
        parkWant_[(size_t) i] = -1.0;
        return Park::Parked;
    }
    return Park::Chase;
}

}
