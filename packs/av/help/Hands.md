# Hands

A hand tracker that turns finger and hand movement into MIDI and control values.

Show the camera a hand and each finger, the wrist position and a pinch become smoothed signals on one MIDI outlet, with no audio. Cord it into a synth organism or a MidiOut, or right-click any knob and pick Hands under Control to drive a parameter without a cord. Two hands track in fixed left and right banks: the left bank sends CC base +0 to +8 (Thumb, Index, Middle, Ring and Pinky openness, X, Y, Present, Pinch) and the right bank +9 to +17. Four gesture slots each send a note when the live hand matches a learned pose. The video inlet accepts a cord from CameraIn, VideoPlayer or any video organism; while a cord is patched the tracker reads that picture and its own camera stays closed.

## Learning gestures

Press Learn on a slot and hold the pose for about a second and a half; the averaged shape becomes the template and is saved in the patch. Press Reinf and show the same pose again from another angle to fold it into the template. The marker on the match bar is that slot's fire threshold; drag it right so only a committed pose sends the note. When several templates match, the best one suppresses the others.

## Parameters

**Enabled** Opens the camera. Nothing is captured while it is off, and a patched video cord keeps the camera closed regardless.

**Camera** Which capture device to open, when there is more than one.

**Mirror** Flips the picture so moving right moves the reading right. If the source organism already mirrors, turn one of the two off.

**BuiltIn** Uses the built-in tracker instead of the system one. It is slower, and it already runs wherever the system has no tracker of its own.

**TwoHands** Tracks a second hand in the right bank. Off follows a single hand.

**Confidence** How sure the detector must be before a hand is accepted. Raise it if lamps or faces cause phantom hands, lower it for gloves or low light.

**Smooth** Response smoothing in milliseconds. Small is twitchy, large is calm and slower.

**Tolerance** How loosely a live pose may fit a template and still match. Matches are continuous, so they can swell in as velocity rather than snap.

**SendCC** Sends the eighteen movement values as continuous controllers. Off leaves only the gesture notes.

**SendNotes** Sends a note for each gesture match. Switching it off releases any note currently held.

**SendOSC** Also publishes every value under the organism's OSC name.

**MidiCC** The first CC number; the eighteen values count up from it.

**MidiChannel** The MIDI channel for the CCs and notes.

**Gestures** The learned templates, stored in the patch. Learn and Reinf write it; there is nothing to type.

**GNote_1** The note gesture slot 1 sends, with velocity set by match strength, and so on for 2 to 4.

**GThresh_1** The match strength above which slot 1 fires; release sits just under it so a wavering pose does not flutter, and so on for 2 to 4.

## Recipe

**Pinch filter** Enabled on, TwoHands off, Smooth 150. Cord an audio source through a Filter, right-click the Filter's cutoff knob, choose Hands under Control and pick pinch. Squeeze thumb and index together to close the filter and open the hand to let it through.

## Related Organisms

CameraIn, Skeleton, SoundSpace, MidiOut
