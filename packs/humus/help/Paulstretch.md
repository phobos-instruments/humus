# Paulstretch

An extreme time stretch that turns a sound file into a slow, phaseless cloud, after the public-domain Paulstretch algorithm.

It is not a stretcher for tempo work: below about four times it smears, and the smear is the instrument. The playhead crawls through the file at one over Stretch speed; each step is windowed, its spectrum kept and its phases drawn fresh, so the harmony stays and the moment goes. The two channels draw their phases independently, so a mono file comes out wide. Feed it a chord, a voice or a whole track and cord the output to a Fern or a Verbatim for a bed, or straight to a Mixer. The transport strip under the file seeks through the source.

## Parameters

**File** The source sound file.

**Stretch** How many times slower than real time, from 1 to 100. Eight is the classic setting; fifty barely moves.

**Window** The analysis window in seconds. Short keeps some rhythm and gurgle, long dissolves everything into a pad.

**Loop** Wraps the source so the cloud never ends. Off plays through once and settles into silence.

**Active** Switches playback on. Off is silent.

**Level** Output gain.

## Recipe

**Pad from a chord** Load a two-second chord, Stretch 30, Window 0.5, Loop on. Cord the output into a Verbatim with a long tail and bring Level down until it sits under the rest of the patch.

## Related Organisms

SpectralFreeze, FilePlayer, Bloom
