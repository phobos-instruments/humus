#pragma once
#include <cstdint>

#include "Grit/Grit.h"

#include "hum/dsp/SoundFileBuffer.h"

#include "nes_apu.h"
#include "nes_dmc.h"
#include "nes_fds.h"
#include "nes_fme7.h"
#include "nes_mmc5.h"
#include "nes_n106.h"
#include "nes_vrc6.h"
#include "nes_vrc7.h"

namespace hum {

struct Grit::Impl {
    struct DpcmRom : xgm::IDevice {
        std::vector<std::uint8_t> bytes;
        void Reset() override {}
        bool Write(xgm::UINT32, xgm::UINT32, xgm::UINT32) override { return false; }
        bool Read(xgm::UINT32 adr, xgm::UINT32& val, xgm::UINT32) override {
            if (adr < 0xC000u || adr - 0xC000u >= bytes.size()) return false;
            val = bytes[adr - 0xC000u];
            return true;
        }
    };

    xgm::NES_APU apu;
    xgm::NES_DMC dmc;
    xgm::NES_VRC6 vrc6;
    xgm::NES_MMC5 mmc5;
    xgm::NES_FDS fds;
    xgm::NES_N106 n163;
    xgm::NES_FME7 fme7;
    xgm::NES_VRC7 vrc7;
    DpcmRom rom;
};

}
