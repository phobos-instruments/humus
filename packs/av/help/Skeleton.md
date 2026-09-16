# Skeleton

A whole-body tracker that turns posture and movement into MIDI and control values.

Step in front of the camera and your head, hands, arm raises, lean, crouch and stance become smoothed signals on one MIDI outlet, with no audio. Cord it into a synth organism or a MidiOut, or right-click any knob and pick Skeleton under Control to drive it without a cord. Twelve values ride CC base +0 to +11: head X and Y, left hand X and Y, right hand X and Y, raise left and right, lean, crouch, stance and present. Left and right are your own, whichever way Mirror flips the picture. One performer is tracked at a time, framed from the knees or feet up, and a reading whose joints are hidden holds its last value. The video inlet takes a cord from CameraIn, VideoPlayer or any video organism; while it is patched the tracker reads that picture and its own camera stays closed.

## Learning poses

Press Learn on a slot and hold the pose, arms crossed or one fist raised, for about a second and a half; the averaged shape becomes the template and is saved in the patch. Press Reinf and strike it again from another spot to fold that into the template. A pose is its arm geometry, not a position in the frame, so it fires wherever you stand. The marker on the match bar is the fire threshold, and the best match suppresses the rest.

## Parameters

**Enabled** Opens the camera. Nothing is captured while it is off, and a patched video cord keeps the camera closed regardless.

**Camera** Which capture device to open, when there is more than one.

**Mirror** Flips the picture so moving right moves the reading right. If the source organism already mirrors, turn one of the two off.

**Confidence** How sure the tracker must be before a person is accepted. Raise it if furniture or a poster keeps being taken for a person, lower it in bad light.

**Smooth** Response smoothing in milliseconds. Higher is calmer and slower.

**Tolerance** How loosely a live pose may fit a template and still match.

**SendCC** Sends the twelve body values as continuous controllers. Off leaves only the pose notes.

**SendNotes** Sends a note for each pose match. Switching it off releases any note currently held.

**SendOSC** Also publishes every value under the organism's OSC name.

**MidiCC** The first CC number; the twelve values count up from it.

**MidiChannel** The MIDI channel for the CCs and notes.

**Gestures** The learned templates, stored in the patch. Learn and Reinf write it; there is nothing to type.

**GNote_1** The note pose slot 1 sends, with velocity set by match strength, and so on for 2 to 4.

**GThresh_1** The match strength above which slot 1 fires; release sits just under it so a wavering pose does not flutter, and so on for 2 to 4.

## Recipe

**Arm-raise swell** Enabled on, SendNotes off, Smooth 200. Cord an audio source through a Gain, right-click its Gain knob, choose Skeleton under Control and pick raise/r. Raise your right arm to bring the sound up and drop it to take it away.

## Related Organisms

Hands, CameraIn, MidiOut
