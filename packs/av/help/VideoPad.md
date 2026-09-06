# VideoPad

Eight pads in a grid, one outlet. Each pad is a clip: drop a video on it or click the folder to load one, click the picture to launch it, and the next launch dissolves into it over the Fade time. The number badge lights on the pad that is on screen. It is the clip rack of a live-visual desk brought into the patch: right-click a pad for its launch control, so a row of hardware pads becomes a video keyboard, and the MIDI inlet launches pads by note - middle C is pad 1, the next semitones the pads after it.

Under the picture are two lines. The upper one is the tape, with the stretch between In and Out lit and a mark at each end: drag a mark to move that point, so the loop can be set by eye and you can see how much of the tape it takes. The lower one is the clip itself, In to Out across the whole width, with the playhead riding it, so a two-second cut of a long tape still reads at a glance. Drag across the picture to scrub inside the cut. The In and Out buttons stamp the playhead into that pad, tightening the cut; the marks on the tape line, and the In and Out knobs in the panel, reach the whole tape again. the loop button decides whether the clip cycles between the two or plays through once and holds its last frame. Out at the end of the file means the whole clip. Launching a pad that is already playing restarts it from In, so it can be played like a drum; the play/pause button holds the picture instead. Drag a pad by its number badge onto another to copy the clip there with its In, Out and loop, so one tape can sit on several pads with different cuts; hold Alt while dropping to move it instead, swapping if that pad is taken. Drag the badge onto the timeline to lay its In-to-Out range down as a clip on a video track; right-click for Clear pad. A clip dragged off the timeline onto a pad loads the pad with that clip's tape, In, Out and loop.

The dice roll the cuts: Random draws a fresh In, Out and Loop for every loaded pad inside that clip's length, then launches one of them. Eight clips, one roll, a new set of loops - the video cousin of a sliced-up break.

Stop dissolves to black and Mute holds black for as long as it is on, a blackout switch that leaves the clips running underneath. Speed runs the motor for every pad at once. Pads chooses four or eight; Outlets adds one video outlet per pad after the main one, so a single pad can feed its own VideoFX or tracker while the main outlet still carries the launched mix. Opacity and Blend set how the main outlet composites when it feeds a Lumen layer, exactly as a VideoPlayer does. Only the pad on screen, the one it is replacing and any pad with its own outlet cabled decode; the others wait paused on their first frame.

## Parameters

**File1 to File8** the clip on each pad.

**Launch1 to Launch8** the launches. Momentary; right-click a pad to MIDI-learn one.

**In1 to In8** where each clip starts, in seconds. Rolled by Random.

**Out1 to Out8** where each clip ends, in seconds; 0 runs to the end of the file. Rolled by Random.

**Loop1 to Loop8** whether the pad cycles between In and Out or plays through once. Rolled by Random.

**Stop** dissolves to black over the Fade time.

**Mute** holds the outlet black while on.

**Fade** how long a dissolve takes, in seconds. The default is 0, a straight cut; turn it up to dissolve.

**Rate** the Speed knob, in percent, for every pad.

**Pads** four or eight pads.

**Outlets** one main outlet, or the main outlet plus one per pad.

**Opacity** how strongly the main outlet shows when it feeds a Lumen layer.

**Blend** how it combines there: Normal, Add, Multiply or Screen.

## Related Organisms

VideoPlayer, VideoFX, VideoMix, Lumen, VideoOut, Leafcutter
