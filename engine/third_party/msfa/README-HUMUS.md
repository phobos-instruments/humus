# music-synthesizer-for-android (engine subset)

Vendored from https://github.com/google/music-synthesizer-for-android
at commit f67d41d313b7dc85f6fb99e79e515cc9d208cfff (Apache License 2.0,
see LICENSE). This is the six-operator FM engine only - the subset of
app/src/main/jni needed to render notes: tables (sin/exp2/freqlut),
envelopes (env/pitchenv), lfo, the FM core, the packed-patch unpacker,
and Dx7Note.

Local modifications (per Apache-2.0 section 4b, changes are marked with
"Humus:" comments in the files):
- dx7note.h/.cc: added Dx7Note::initLogfreq(), an init taking the base
  pitch as a Q24 log2-frequency instead of a MIDI note number, so the
  host's microtonal Tuning drives the oscillators exactly; the nearest
  note number is still passed for the keyboard level/rate scaling
  curves.
- synth.h: added an MSVC branch for SynthMemoryBarrier(). Upstream only
  knows __APPLE__ and __GNUC__, and the #warning it falls through to is a
  hard error on MSVC (C1188), so the engine did not compile on Windows -
  and had it compiled, the barrier would have expanded to nothing.
- aligned_buf.h: include <stddef.h>/<stdint.h>. size_t and intptr_t reach
  this header transitively on libc++ and MSVC but not on libstdc++, where
  the class template did not compile at all.
- dx7note.cc: dropped `using namespace std;` (the two VERBOSE streams it
  covered are now qualified). libstdc++ pulls std::min/std::max in through
  <math.h>, which made every min()/max() call here ambiguous against
  synth.h's own templates.
