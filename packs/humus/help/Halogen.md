# Halogen

Plays a picture. Across is time, up is pitch, brightness is how loud - the same reading a spectrum analyser gives, run backwards. A scan head takes one column of the picture at a time and an inverse transform turns it back into sound, so what you see under the head is what you hear. Drop a file on the panel or click it to browse.

## Anything at all

A file that will not decode as a picture is read as its own bytes instead, laid out in rows. A font, an archive, a binary: they all have a spectrum in them, and this is how you hear it. The picture on the panel is the same field the sound comes out of, so what you see is always what is playing.

## Scanning

Sweep walks the head across the picture at Rate, once every four seconds at the default, and Reverse sends it the other way. Free parks the head at X instead, so a knob, an envelope, a controller or a hand on the picture is the scan head - drag inside the panel to move it.

## The window

X and Span set which part of the width is read; Y and Height set which part of the height. Together they are a box on the picture, drawn on the panel, and everything outside it is silent. Pull them in to play one corner, one face, one cloud.

## Pitch

Low and High are what the bottom and the top of that box sound like, spread logarithmically between them, so a picture spans a musical range rather than a linear one. Narrow them and the picture becomes a chord; open them and it becomes weather.

## Colour

Gate is a brightness floor - below it a pixel is silent, which is how a photograph's dark background stops humming. Blur smooths each partial over time, so a moving head glides instead of stepping; at zero the columns arrive raw and grainy. Tilt leans the balance toward the low end or the high end, which tames a picture with a bright sky in it. Level sets the output.

The dice rolls everything about the reading - where the box sits, how it is tuned, how it is coloured - and never the picture or the output level.

## Parameters

**Picture** the file being read. Any file; pictures decode, the rest are bytes.

**Scan** Sweep walks the head, Free parks it at X.

**Reverse** sweep the other way.

**Rate** sweeps per second.

**X** where the head sits, or where the swept span starts.

**Span** how much of the width is read.

**Y** the bottom of the read band.

**Height** how much of the height is read.

**Low** what the bottom of the band sounds like, in hertz.

**High** what the top of the band sounds like, in hertz.

**Gate** brightness floor. Dark pixels below it are silent.

**Blur** smoothing of each partial over time.

**Tilt** low-end to high-end balance.

**Level** output level.

## Signal flow

Picture -> brightness field -> the column under the scan head -> partials, log-mapped between Low and High -> inverse transform, overlap-added -> Level -> Output. No input; stereo out, the two sides at different phases.

## Related Organisms

Prism, Wave, SoundSpace
