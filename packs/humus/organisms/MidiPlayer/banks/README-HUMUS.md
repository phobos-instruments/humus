# MidiPlayer voice bank

`gm.wopn` is a chip instrument bank by Vitaliy Novichkov ("Wohlstand"),
vendored from https://github.com/Wohlstand/libOPNMIDI (`fm_banks/`, taking
`gm-old.wopn` because upstream's `gm.wopn` is a symlink). It is **MIT
licensed** - the author's own statement is kept verbatim in
`UPSTREAM-readme.txt`: "This bank can be freely used, modified, shared with
any purposes. License for this bank - MIT".

It holds a melodic bank in General MIDI program order plus a percussion
bank keyed by drum note, which is what a player needs to sound a file
nobody wrote for this program.

This is the same file pH ships in its own `banks/` folder. It is copied
rather than shared because bank lookup searches an organism's own folder,
and pointing this organism at pH's would couple the two. If it is ever
replaced, replace both.

Banks a player installs live in `Documents/Humus/Library/Banks/` and shadow
a shipped bank of the same name.
