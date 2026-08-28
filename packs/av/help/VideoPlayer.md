# VideoPlayer

The video deck. Load a tape (any .mp4/.mov/.m4v) into its File slot, cord the video outlet - the video-coloured pin - into one of Lumen's video inlets, and the tape plays there, looping. Video cords are the third cord domain beside audio and MIDI, drawn in their own colour.

Rate is the motor speed (0 freezes the frame, 2 doubles it), Opacity and Blend (Normal, Add, Multiply, Screen) set how the layer composites at the display, and Wear is how chewed the tape is: tracking jitter, a rolling dropout band of static, chroma bleed. All ordinary params - automatable and modulation-routable, so a Follower can age the tape with the music.

The dotted MIDI inlet is the relaunch trigger: ANY note-on rewinds and restarts the tape. One deck per launch lane - cord a Sequence, PianoRoll or keyboard into each deck to cut video in time.

Video decode is macOS-only for now; elsewhere the deck loads but the layer stays dark.

## Parameters

**File** the video to play.

**Rate** playback speed. 1 is as recorded; 0 holds a still frame.

**Opacity** how strongly this layer shows over the ones beneath it.

**Blend** how it combines with those layers: Normal, Add, Multiply or Screen.

**Wear** analog rot - the tape damage that makes a clean file look played.

## Related Organisms

Lumen, VideoMix, VideoOut
