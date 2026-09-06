# Hands

Hand tracking as a control source: show the webcam a hand and every finger becomes a patchable signal. Video never leaves the process, and the camera opens only while Enabled is on, with the system asking permission on first use. The preview draws the live skeleton, green bones and red joints.

Hands is a MIDI controller - one MIDI outlet, no audio. Cord it into a plugin, a synth organism or a MidiOut for hardware; to drive native parameters, map its CCs in Parameter Control like any controller.

## What it sends

Two hands are tracked in stable L/R banks: your physical left hand is always the L bank, wherever it wanders in the frame. L takes CC base +0 to +8 and R +9 to +17, each bank carrying the same nine values. All are smoothed 0..1 before being scaled to 0..127.

**+0 to +4** Thumb, Index, Middle, Ring, Pinky - finger openness, 0 curled into the palm and 1 fully extended. Scale-invariant.

**+5, +6** X and Y, the wrist position in the frame, Y up.

**+7** Present - 1 while that hand is seen, falling when it leaves.

**+8** Pinch - thumb tip to index tip, 1 when touching. The classic precision controller: map it to a filter and squeeze.

Every live value is also a control source: right-click any knob - or a transport button - and Hands appears under Control with, fingers, x/y, pinch and the four gesture matches alike, no MIDI cords needed. Each learned gesture sends its own note, with velocity set by match strength. The marker on the match bar is that gesture's fire threshold: drag it right and only a committed pose sends the note, with the release just under it so a wavering pose does not flutter. Either hand can fire a gesture, and the best match wins.

## Video in

The video inlet takes a cord from any video contraption - CameraIn, VideoPlayer, a VideoMix blend - and the tracker reads that picture instead of opening its own camera. A patched cord always feeds; Enabled only governs the built-in camera, which stays closed for as long as a cord is patched. A cord is read by whichever tracker Built-in selects, exactly as the camera is. Mirror still applies to whatever arrives, so if the source contraption mirrors too, the two flips cancel - turn one off.

## Learning gestures

Four slots. Press Learn and hold, or keep repeating, the pose for about a second and a half; the average shape becomes the template and is stored in the patch. Then press Reinf and show the same pose again - other angle, other distance, lazier fingers - and each reinforcement folds into the template as a running average, the xN on the bar being its depth, so recognition drifts toward how your hand actually makes the pose. Learn always starts fresh; Reinf refines.

Similar gestures stay apart. Whenever the set changes, each template re-derives its core features, so the fingers that distinguish it from its nearest neighbour count for more and a "1" and a "3" stop competing on the index they share. Live matches compete too: the best suppresses the rest, so holding a "3" cannot leave a similar "1" hovering over its fire threshold.

## Parameters

**Enabled** opens the camera. Nothing is captured while this is off.

**L+R** two-hand tracking. Off drops to a single hand.

**Confidence** how sure the detector must be before a hand is accepted. Raise it if lamps or faces cause phantom hands, lower it for gloves or low light.

**Smooth** response smoothing in milliseconds: small is twitchy, large is silky.

**Tolerance** how sloppy a pose may be and still count. Matches are continuous, so a gesture can swell in as velocity or gate as a note on and off.

**MIDI row** the CC base and channel. The CCs and Notes streams enable independently - CCs off for a notes-only pad controller, Notes off for pure continuous control. Switching Notes off releases anything currently held.

## Notes

Curated MIDI rather than 21 raw points: notes for events, CCs for movement. Nine movements plus four events say everything a hand does, without the soup.

## Related Organisms

CameraIn, SoundSpace, MidiOut
