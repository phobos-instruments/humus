// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#include "common/GmBank.h"

#include <cstdint>

#include "common/FmBank.h"

namespace hum {

namespace {

int signedWord(const uint8_t* e) {
    const int raw = (e[32] << 8) | e[33];
    return raw >= 0x8000 ? raw - 0x10000 : raw;
}

void readEntry(const uint8_t* e, GmBank::Entry& out) {
    out.name = fmbank::wopnName(e);
    out.patch = fmbank::wopnPatch(e);
    out.noteOffset = signedWord(e);
    out.percussionKey = e[34];
}

}

bool GmBank::load(const juce::File& file) {
    ready_ = false;
    hasPercussion_ = false;
    juce::MemoryBlock raw;
    if (!file.existsAsFile() || !file.loadFileAsData(raw)) return false;
    const auto* d = static_cast<const uint8_t*>(raw.getData());

    fmbank::WopnLayout layout;
    if (!fmbank::wopnLayout(d, raw.getSize(), layout)) return false;
    if (layout.melodicBanks < 1) return false;

    for (int i = 0; i < kPrograms; ++i)
        readEntry(d + layout.first + (size_t) i * layout.stride, melodic_[(size_t) i]);

    if (layout.percussionBanks >= 1) {
        const size_t base = layout.first
                          + (size_t) layout.melodicBanks * kPrograms * layout.stride;
        for (int i = 0; i < kPrograms; ++i)
            readEntry(d + base + (size_t) i * layout.stride, percussion_[(size_t) i]);
        hasPercussion_ = true;
    }

    name_ = file.getFileNameWithoutExtension().toStdString();
    ready_ = true;
    return true;
}

const GmBank::Entry& GmBank::melodic(int program) const {
    return melodic_[(size_t) wrap(program)];
}

const GmBank::Entry& GmBank::percussion(int note) const {
    return hasPercussion_ ? percussion_[(size_t) wrap(note)] : melodic_[(size_t) wrap(note)];
}

}
