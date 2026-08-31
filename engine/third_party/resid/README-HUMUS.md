# reSID

Vendored from https://github.com/daglem/reSID
at commit ef7873fc8c8379dc14cef8d9ccf9b3d34d0cc439 (GPL-2.0-or-later, see
COPYING; combined with this AGPL-3.0 application under GPLv3 as the
licence's own upgrade clause permits). The src/ tree is taken verbatim
except siddefs.h, which upstream generates from siddefs.h.in at configure
time; ours is that substitution done once, with C++17-safe values (constexpr
disabled under MSVC, which cannot fold the exp() calls in the filter
coefficient constructors), and
the .in kept beside it. The Silt organism drives it: both chip models,
the analog filter, ring modulation and sync, resampled by the core itself
to the graph rate.
