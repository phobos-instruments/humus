# Banks

Many instruments in one file, as opposed to `Samples/`, which is one sound per
file. The Sampler reads `.sf2`; pH reads `.syx`, `.wopn`, `.tfi` and `.dmp`.

`Documents/Humus/assets/Banks/` is read first, so your own banks win by name.
The voice banks that belong to an organism ship inside its pack instead
(`packs/humus/organisms/Ph/banks/`), because a downloadable pack has to carry
its own.

**Empty in the source distribution.** The released app bundles one soundfont
built from the sample kit above; the zip carries the code, not the content.
Soundfonts and chip voice banks in these formats are widely published under
free licences.
