# CLAP (vendored)

CLAP **1.2.10**, MIT - https://github.com/free-audio/clap
(tag `1.2.10`, commit `195b42a004144fab0b3cf95e9c067187d15365b7`).

`include/` is copied verbatim from that tag; `LICENSE` is upstream's.

CLAP is a plug-in standard defined entirely by these C headers: there is no
library to link. Humus hosts CLAP plug-ins through them
(`engine/src/core/plugins/clap/` for the standard itself,
`engine/src/gui/plugins/clap/` for the JUCE side), and the hosting tests build
a small CLAP plug-in from `engine/tests/fixtures/testclap/` against them.

To update: copy `include/` from a newer tag, update the version above, and run
the hosting tests on every platform.
