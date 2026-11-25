# Sequence

A sixteen-step drum grid of one to eight rows, where every row is its own MIDI outlet.

Each row sends its notes out of its own MIDI outlet, so row 1 can drive a Kick, row 2 a Sampler and row 3 a Silt, with the name plate on the row showing what it is corded to. The last outlet is the master and carries every row together, for one drum sampler that maps the whole kit by note. Eight banks each hold a full grid; the row notes, velocities and enables are the kit and stay the same across banks. The pattern is one bar of sixteenths and edits land while the transport plays.

## The grid

Click a step to set it, click again to clear it, drag to paint a run. The running column lights while the transport plays. The LED at the row's left is the row's enable, the knob is its velocity, and the mouse wheel over the name plate changes the row's note. Right-click a step for Random, Clear and Copy to another bank. Right-click a step, the LED or the name plate for Exclude Row from Random: every Random, the grid's own included, then leaves that row's steps and velocity as they are, and a padlock shows on its name plate. To keep only a velocity, right-click its knob for Exclude from Random, as on any knob.

**Step chance** Click the % switch under Gate to see each set step's chance as a bar, then drag up or down on a step to set how often it plays, from never to every time in steps of 5%. Cmd-drag a step (Ctrl-drag on Windows and Linux) does the same without switching. The % switch is the View parameter, so a mapped button or pad flips it too. The knob beside each row's velocity is the row's Chance: it scales every step of that row, so one fader thins out a hat line. Right-click the knob, or any of the row's steps in chance view, to map it. A step below 100% draws fainter and shows its chance on it. The dice belong to the box: a patch plays the same steps on every play and every bounce, and two boxes roll their own. Each bank keeps its own chances, Copy to bank takes them along and Clear sets them back to sure.

## Parameters

**SwingFollow** Follow makes the grid take the patch groove from the transport instead of its own Swing knob.

**Swing** Delays the offbeat sixteenths, from straight at 0 to a full triplet shuffle at 1. Used only while Follow is off.

**Gate** Note length as a fraction of a step.

**Enable_1** Whether row 1 plays; steps are kept while it is off. And so on for rows 2 to 8.

**Vel_1** Velocity of row 1's notes. And so on for rows 2 to 8.

**Note_1** The MIDI note row 1 sends, defaulting to the general drum map, 36 for a kick. Synth drums such as Kick ignore it; samplers map it. And so on for rows 2 to 8.

**Bank** Which of the eight grids plays, A to H.

**View** Steps or Chance: what the grid shows and edits. The % switch under Gate changes it.

**Chance_1 to Chance_8** How often each row's steps play, on top of each step's own chance.

**Rows** How many rows the grid has, from 1 to 8. The outlets follow: one per row, and the master always last, so a cord on the master stays on the master when the count changes. Rows you take away keep their steps, notes and velocities, and come back as they were if you add them again; only the cords from their outlets are removed.

## Recipe

**Four on the floor** Cord row 1 to a Kick, row 2 to a Cicada or a Sampler clap, row 3 to a hat sample. Steps 1, 5, 9 and 13 on row 1, 5 and 13 on row 2, every other step on row 3. Swing 0.15 with Follow off, Gate 0.5. Draw a fill in bank B and switch Bank from a Button for the turnaround.

## Related Organisms

Riff, Steps, PianoRoll
