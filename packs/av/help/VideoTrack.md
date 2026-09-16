# VideoTrack

A video track on the timeline that plays its clips against the transport and records what reaches its inlet.

Add one from the timeline's add-track menu, or drop a movie onto the arrangement and a track appears for it. Clips cut, trim, move, loop and fade like audio clips, and the outlet shows whatever sits under the playhead: stop and it holds that frame, locate and it jumps there, loop the song and it loops with it. Warp stretches a clip to the song's tempo against its source tempo, Reverse plays it backwards and fades are fades to black. Each clip wears a strip of frames from its tape, and right-click a clip for Media Info to read its codec, size and frame rate. Cord the outlet into a VideoOut, a VideoFX or a Lumen layer; two tracks are two layers.

## Recording

Cord a CameraIn, a VideoFX chain, a VideoPad or another track into the inlet, arm the track with Record and press record on the transport: whatever reaches the inlet is written as a movie in the recordings folder while the transport rolls and lands on the timeline as a clip at the beat it started, one lap per pass of a loop. Sound is not part of the picture; arm an audio track beside it. The recording size is set in Settings, under Video.

## Parameters

**Record** Arms the track. While it is armed, the next transport record writes a take from the inlet.

**Monitor** What the outlet shows. In always shows the inlet, Off always shows the clips, and Auto shows the inlet while the track is armed or the transport is stopped and the clips while it rolls.

**Opacity** How strongly the track's picture shows over the layers beneath it in a Lumen.

**Blend** How it combines with those layers: Normal, Add, Multiply or Screen.

## Recipe

**Live capture over a loop** Cord a CameraIn into the track's inlet and the track's outlet into a VideoOut. Set Monitor to Auto, arm Record, loop four bars and press record: you see the camera while it records, and once you stop the take plays back in the arrangement cut to the loop.

## Related Organisms

VideoPad, VideoPlayer, VideoOut, AudioTrack
