# LAME (vendored)

LAME **3.100**, LGPL-2.0-or-later - https://lame.sourceforge.io

`lame-3.100.tar.gz`, sha256
`ddfe36cab873794038ae2c1210557ad34857a4b6bdc515785d1da9e175b1da1e`.
`COPYING` is the upstream licence text, unaltered.

What was taken, verbatim: `include/lame.h`, every header in `libmp3lame/`,
`libmp3lame/vector/lame_intrin.h`, and the nineteen `.c` files an encoder
needs -

    bitstream encoder fft gain_analysis id3tag lame newmdct presets
    psymodel quantize quantize_pvt reservoir set_get tables takehiro
    util vbrquantize VbrTag version

What was left out, and why:

- `mpglib_interface.c` and the `mpglib/` decoder. Humus only encodes; JUCE
  already decodes MP3. `HAVE_MPGLIB` is not defined, which compiles the
  decode entry points out of `lame.c`.
- `vector/xmm_quantize_sub.c` and `i386/`, the SSE and assembly paths.
  `HAVE_XMMINTRIN_H` and `HAVE_NASM` are not defined, so the portable C is
  used everywhere. A bounce is not bottlenecked on the MP3 encoder; the
  same code on every platform is worth more than the speed.
- The front end (`frontend/`), the tools, and the whole autotools build.
  `config.h` here is written by hand rather than generated: upstream only
  reaches for seven macros in the sources we compile, and they are the
  seven above.

MP3's patents all expired in 2017, so nothing here owes a pool. LGPL-2.0
combines with AGPLv3 through the LGPL's own upgrade clause, and Humus
ships its source either way.

Not modified. To move to a later LAME: replace the files, keep this list,
and re-run `mp3-write-check`.
