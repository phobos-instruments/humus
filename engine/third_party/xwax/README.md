# xwax timecode decoder

The timecode half of [xwax](https://xwax.org) by Mark Hills, taken from the
copy the [Mixxx](https://mixxx.org) project maintains in `lib/xwax` (Mixxx
commit `b8f9407`, 2026-09-28). Mixxx's copy adds Traktor Scratch MK2 (side A,
side B and CD), a Kalman pitch filter, and the filters MK2 needs.

**Licences.** xwax is **GPL-3.0-only** (`COPYING`). The MK2 files Mixxx
contributors added (`timecoder_mk2`, `lut_mk2`, `filters`, `pitch_kalman`,
`fmatrix`, `ringbuffer`, `types.h`, `complex.h`) come under Mixxx's
**GPL-2.0-or-later**, which is compatible. Humus is AGPLv3, and AGPLv3
section 13 is what permits the combination, the same reasoning
`packaging/THIRD-PARTY-src.md` records for another component. Nothing here may
be copied into a part of Humus that is not distributed under those terms.

## Why this is here rather than our own decoder

A control record does not carry a plain tone. Underneath it is a position
bitstream, encoded by switching the polarity and phase of the carrier (MK2
shifts each cycle up or down instead), and a decoder that tracks phase naively
cannot tell a bit flip from the record moving. Measured against a real
capture, our own tracker read a steady 1.0x record as 0.69 and swung between
0.006 and 1.00; this reads it as 0.99. On a real MK2 needle-drop capture it
locates the needle within about a second of each drop.

## The one local change

`patches/0001-keep-the-table-when-the-cache-cannot-be-written.patch`: when the
MK2 lookup table cannot be written to its cache folder, the table is kept
instead of every table being freed. The Deck runs other decoders while an MK2
table builds, so freeing them would pull the tables out from under a running
decoder; a full disk then means only that the table is rebuilt next time.

## Updating

Copy the files from Mixxx's `lib/xwax` over these, re-apply the patch, and
re-run the checks. The only other local addition is the `msvc/unistd.h` shim
the Windows build uses. Timecode definitions live in `timecoder.c` and arrive
with the files.
