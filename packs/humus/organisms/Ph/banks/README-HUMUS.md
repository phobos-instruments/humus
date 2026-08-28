# pH voice banks

`gm.wopn` and `xg.wopn` are chip instrument banks by Vitaliy Novichkov
("Wohlstand"), vendored from https://github.com/Wohlstand/libOPNMIDI
(`fm_banks/`, taking `gm-old.wopn` as `gm.wopn` because upstream's `gm.wopn`
is a symlink). They are **MIT licensed** - the author's own statement, kept
verbatim in `UPSTREAM-readme.txt`: "This bank can be freely used, modified,
shared with any purposes. License for this bank - MIT".

Between them they carry roughly 185 and 700 named instruments; the reader
loads at most 512 from one file.

Banks a player installs live in `Documents/Humus/Library/Banks/` instead, and
shadow a shipped bank of the same name.

## genesis.wopn

`genesis.wopn` (upstream `gems-fmlib-gmize.wopn`, same repository) holds 417
instruments imported from the stock set of GEMS, which was Sega's official
music creation system for the Mega Drive, re-ordered into General MIDI order
by the same author, with some instruments taken from `xg.wopn`. Its readme is
kept verbatim as `genesis-readme.txt`.

Unlike `gm.wopn` and `xg.wopn`, **this bank carries no licence statement** -
neither a grant from its packager nor one from Sega. It is bundled at the
project owner's explicit direction, twice asked and given after the exposure
was set out. Anyone re-evaluating what ships should start here.

## Adding more banks

Anything dropped in **this folder** ships with Humus: the pack is copied
whole into the app bundle, the plugin bundles, the AppImage and the
`.humpack`, so a `.syx`, `.wopn`, `.tfi` or `.dmp` here appears in the Bank
browser on every install with no code change. Rebuild to restage.

Anything dropped in **`Documents/Humus/Library/Banks/`** appears the same way
but stays on that machine, and shadows a shipped bank of the same name. That is
the right home for banks whose licence does not permit redistribution - a bank
has to be installed for a patch that names it to resolve, so shipped banks
travel with a document and personal ones do not.

Bank references are stored as `asset:Banks/<name>`, so keep filenames stable
once patches start pointing at them.

If a bank you drop in here does not appear in the app, `touch` it and build
again. Staging the pack into the bundle is a stamped build step keyed on file
times, so a file whose timestamp predates the last stamp can be skipped even
once the build knows about it.
