# Nuked-OPM

Vendored from https://github.com/nukeykt/Nuked-OPM
at commit f209e6ed3712032b641d53ce8fb24824eae6adc3 (LGPL-2.1, see
LICENSE). Unmodified: opm.c/.h, the die-traced YM2151 (OPM) core, which
also carries the YM2164 (OPP) variant behind a reset flag. The chip runs
at its own clock; the pH organism resamples its output to the graph rate
and feeds register writes through a paced queue, the way real drivers do.
