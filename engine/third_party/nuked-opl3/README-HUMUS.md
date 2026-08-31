# Nuked-OPL3

Vendored from https://github.com/nukeykt/Nuked-OPL3
at commit 765ec962e473aeb767e4cba74ffdc8f588ffbfe8 (LGPL-2.1, see
LICENSE). Unmodified: opl3.c/.h, the YMF262 (OPL3) core. The chip runs
at its own clock; the pH organism renders it at the native rate and
resamples in its ring, and register writes go through the core's own
buffered write path.
