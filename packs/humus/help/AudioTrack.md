# AudioTrack

The recording sequencer's audio channel: a row on the Timeline that plays its clips against the transport and records takes of whatever is cabled into it.

It sits inline in the signal path, stereo in and stereo out, so it goes downstream of the source you want to capture. What leaves it is its clips, its input, or both, depending on Monitor. A take lands as a clip on the track's row, and a second pass over the same bars lands on top of the first, so nothing is lost. A sound file dropped on the Timeline lands on an AudioTrack too, with a new row made and cabled to the master if none is under the cursor. Cord it before a Mixer, or straight into a SoundOut.

## Parameters

**Record** Arms the track. The transport's record drives this, so it is normally armed from the transport rather than by hand.

**Monitor** What reaches the outlets. In passes the input at all times and adds the clips while playing. Auto passes the input while armed or stopped and the clips while playing unarmed, which is right for nearly all recording. Off plays the clips only.

**Gain** Level of the track's output.

**Mute** Silences the output. Because the track is inline, this also silences whatever is cabled through it.

## Recipe

**Capture a synth** Cord a Rhizome into the AudioTrack and the AudioTrack into the master. Leave Monitor on Auto, arm from the transport, press play and play the part. Stop, and the take is a clip on the row where it can be moved and trimmed like any other.

## Related Organisms

Deck, Sampler, PianoRoll, Sequence
