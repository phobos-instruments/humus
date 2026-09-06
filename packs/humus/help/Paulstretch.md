# Paulstretch

Extreme time stretch, after the public-domain Paulstretch algorithm by Nasca Octavian Paul - the sound of a song slowed fifty times without turning into a slideshow of clicks. It is not a time-stretcher for tempo work: below about 4x it smears, and that smear is the instrument. Feed it anything - a chord, a voice, a whole track - and it becomes a shimmering, phaseless cloud that keeps the harmony and lets go of the moment.

Load a file, set Stretch, and it plays. The playhead crawls through the source at one over Stretch speed; each step is windowed, its spectrum kept and its phases thrown away and drawn fresh, which is the whole trick - amplitude is memory, phase is time, and forgetting time is what makes it vast. The two channels draw their phases independently, so even a mono file blooms into wide stereo.

Window sets the grain of the cloud: short windows keep more rhythm and gurgle, long windows dissolve everything into pad. Loop wraps the source seamlessly for infinite beds; with Loop off the cloud drifts once through the file and settles into silence.

## Parameters

**File** the source sound.

**Stretch** how many times slower than life. Eight is classic; fifty is geology.

**Window** the analysis window in seconds - the size of the moment it remembers at once.

**Loop** wrap the source forever, or play it once.

**Active** the on switch.

**Level** output gain.

## Related Organisms

SpectralFreeze, FilePlayer, Bloom
