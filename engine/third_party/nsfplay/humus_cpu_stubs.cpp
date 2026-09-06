#include "xgm/devices/CPU/nes_cpu.h"

// Humus addition, not upstream code: Grit drives the sound devices with a null
// CPU pointer (no NSF, no 6502), but nes_dmc.cpp still names these two symbols
// behind its null checks. Empty bodies satisfy the linker; they are never run.
namespace xgm {

void NES_CPU::StealCycles(UINT32) {}

void NES_CPU::UpdateIRQ(int, bool) {}

}
