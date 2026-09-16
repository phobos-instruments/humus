# Halogen

A spectral player that turns a picture into sound: across is time, up is pitch, brightness is loudness.

A scan head reads one column of the picture at a time and an inverse transform turns it back into sound, so what sits under the head is what you hear. Drop an image on the panel or click it to browse; a file that is not a picture is read as raw bytes laid out in rows, so a font or an archive has a spectrum too. A video cord from a VideoPlayer, a CameraIn or a VideoTrack replaces the picture with the moving image, frame by frame, and the file comes back when the cord is removed. The panel shows the picture as the head hears it, after Gate, Blur and Tilt. Cord the outlet into a Fern or a Verbatim for space.

## Parameters

**File** The picture being read. Any file: pictures decode, anything else is read as bytes.

**Scan** Sweep walks the head across the window at Rate. Free parks it at X, so a knob, an envelope or a drag on the panel moves it.

**Reverse** Sweeps the other way.

**Rate** Sweeps per second, 0.01 to 100. At the top the sweep itself becomes a tone.

**X** Where the head sits in Free, or where the swept span starts.

**Y** The bottom of the read band.

**Width** How much of the picture's width is swept.

**Height** How much of the picture's height is read.

**Lowest** What the bottom of the band sounds like, in hertz.

**Highest** What the top of the band sounds like, in hertz. The rows are spread logarithmically between the two.

**Gate** A brightness floor. Pixels below it are silent, which stops a dark background humming.

**Blur** Softens the picture so a moving head glides through edges instead of stepping.

**Tilt** Leans the balance towards the low end or the high end.

**Level** Output level.

## Recipe

**Slow drift** Load a photograph with a dark background. Scan Sweep, Rate 0.1, Lowest 55, Highest 4000, Gate 0.15, Blur 0.4. Cord the outlet into a Verbatim and let one sweep take ten seconds; then set Scan to Free and cord an LFO onto X to move the head.

## Related Organisms

Prism, Wave, SoundSpace
