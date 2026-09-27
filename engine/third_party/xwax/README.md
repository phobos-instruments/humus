# xwax timecode decoder

The timecode half of [xwax](https://xwax.org) by Mark Hills, vendored
unmodified: `timecoder.c/h`, `lut.c/h`, `pitch.h`, `debug.h`.

**GPL-3.0-only** (`COPYING`). Humus is AGPLv3, and AGPLv3 section 13 is what
permits the combination, the same reasoning `packaging/THIRD-PARTY-src.md`
already records for another component. Nothing here may be copied into a
part of Humus that is not distributed under those terms.

## Why this is here rather than our own decoder

A control record does not carry a plain tone. Underneath it is a position
bitstream, encoded by switching the polarity and phase of the carrier, and a
decoder that tracks phase naively cannot tell a bit flip from the record
moving. Measured against a real capture, our own tracker read a steady 1.0x
record as 0.69 and swung between 0.006 and 1.00; this reads it as 0.99.

## Updating

Take the files from upstream unmodified and re-run the checks. The only
local addition is `xwax.cmake`, which builds them as C. If upstream gains
timecode definitions, they arrive with the files - the definition table lives
in `timecoder.c`.
