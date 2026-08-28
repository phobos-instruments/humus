# Nuked-OPN2

Vendored from https://github.com/nukeykt/Nuked-OPN2
at commit 335747d78cb0abbc3b55b004e62dad9763140115 (LGPL-2.1, see
LICENSE). Unmodified: ym3438.c/.h, the cycle-accurate YM3438/YM2612
(OPN2) core. The chip runs at its own clock; the pH organism resamples
its output to the graph rate and feeds register writes through a paced
queue, the way real drivers do.
