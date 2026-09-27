# Decomposer

Audio in, MIDI notes out: a pitch tracker that turns what is played into notes as you play, one line at a time or a whole chord.

Set Voices to 1 and it follows one voice, which is the best setting for a bassline, a sung melody, a lead or a guitar played one note at a time. The pitch search is confined to the LowNote to HighNote range, which is its strongest defence against octave errors, single-frame glitches are smoothed away, and a repeated note re-articulates from its onset instead of merging into one held note. It reports what it hears with no key or scale correction; to pull the result into tune, send the notes through a Trellis. The readout on the editor shows the note being sent and how sharp or flat the source is. Cord a SoundIn into it and its MIDI outlet into any instrument, or into a MidiMonitor to watch it. Expect a few tens of milliseconds of tracking latency.

## Parameters

**Voices** How many notes at once. At 1 it follows a single line, with the fastest response and the steadiest tracking. Above 1 it listens for a chord and sends every note it finds, up to six.

At 2 and above it works differently: instead of following one pitch it takes the sound apart, scoring every note it could be hearing by the partials that note would produce, taking the strongest, removing what that note explains, and looking again for what is left. That is what lets it tell a chord from a single note with a rich tone. It costs time: a chord needs about a third of a second of sound to be sure of, against a few tens of milliseconds for a single line, so Voices 1 stays the right setting for anything played one note at a time.

Ask for the number of notes you expect to play, not more. Asking for six on a three note chord does not find three extra notes, but it does give three more chances to be wrong. It is most confident on sounds with a clear harmonic series and a steady level: a piano, a clean guitar, an organ, a synth pad. It is least confident on the bottom octave, where the notes are too close together to separate quickly, and on anything percussive or breathy, where there is no harmonic series to take apart. Sensitivity sets how quiet a note can be, relative to the loudest in the chord, before it is left out.

**Sensitivity** Lowers the level and clarity thresholds. Raise it for quiet or breathy sources, lower it if noise triggers stray notes.

**Response** How many frames a pitch must hold before it commits. Fast is snappier, Accurate is steadier on noisy sources, Balanced sits between.

**LowNote** The lowest note it will report. Raise it to your source's real range so rumble below cannot be chosen.

**HighNote** The highest note it will report. Lower it to your source's real range so octave errors above cannot be chosen.

**Channel** The MIDI channel the notes go out on.

## Recipe

**A chord to a pad** Cord a SoundIn from a guitar or a keyboard into the Decomposer, Voices 4, LowNote 40, HighNote 84, and its outlet into a Rhizome or a Sampler. Play a chord and the whole shape comes out the other side. The readout shows the notes it is sending, lowest first, and the tone bar beside them shows how harmonic what it is hearing is: a full bar is a clean chord, an empty one is noise it will not name.

**Voice to synth** Cord a SoundIn from a microphone into the Decomposer and its outlet into a Rhizome. Response Balanced, Sensitivity 0.5, LowNote 48, HighNote 84 for a typical singing range. Hum a line and the synth doubles it; add a Trellis between the two to keep it in key.

## Related Organisms

Trellis, OscMonitor, MidiMonitor
