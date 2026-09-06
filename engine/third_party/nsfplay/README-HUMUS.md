# NSFPlay sound devices (vendored)

The Famicom / NES sound chip emulation behind the Grit organism: the
Ricoh 2A03/2A07 APU (`nes_apu`, `nes_dmc`) and the cartridge expansion
audio - Konami VRC6 (`nes_vrc6`), VRC7 (`nes_vrc7`, riding Mitsutaka
Okazaki's emu2413 OPLL core), Famicom Disk System (`nes_fds`), Nintendo
MMC5 (`nes_mmc5`), Namco 163 (`nes_n106`) and Sunsoft 5B (`nes_fme7`,
riding emu2149).

Taken from NSFPlay as maintained by Brad Smith,
https://github.com/bbbradsmith/nsfplay - the `xgm/devices/Sound` tree
plus the headers it needs (`device.h`, `xtypes.h`, the km6502 CPU
headers that `nes_dmc.h` includes for its IRQ hook; the CPU itself is
neither compiled nor used). Nothing upstream is modified; only unused devices and the player
shell were left behind, and `humus_cpu_stubs.cpp` (ours, not
upstream's) gives the two never-run NES_CPU methods that nes_dmc
names empty bodies, since no 6502 is vendored or wanted.

**Licence:** NSFPlay's own terms, from its readme (carried verbatim in
`README-nsfplay.txt` beside this file): the code descends from
NSFPlay/NSFPlug by Brezza, distributed freely with modification and
redistribution permitted, and Brad Smith maintains his fork under the
same permissive terms - "You may reuse this code without restriction",
no warranty. The embedded emu2413 and emu2149 cores are by Mitsutaka
Okazaki (see their file headers).

Grit drives the devices directly - register writes at audio rate,
`Tick` per output sample at the chip clock - rather than through
NSFPlay's NSF player, so no NSF parsing or CPU emulation is vendored.
