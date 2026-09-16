# FileRecorder

A pass-through that writes what reaches it to a sound file while the patch plays.

The input goes straight to the outlet unchanged, so it drops into a cord anywhere in the patch and records a stem there rather than only at the master. The Tracks dropdown sets how many stereo files it writes, 1 to 32, with a file row per track; the channels are handed to the files in order, each row's Ch spinner saying how many it takes, and a file reaching past the last inlet records silence. Files are written off the audio path so a slow disk cannot interrupt the sound, and each row meters what arrives whether or not a take is rolling. The transport's own Record captures armed AudioTrack organisms onto the timeline instead; use this organism when you want plain files on disk.

## Parameters

**File_1** Where file 1 goes, and so on for the rows below. Leave it empty and each arm writes a timestamped take into the recordings folder and fills the row in; a file you named yourself is kept and reused.

**RequestedChannelCounts_1** How many channels file 1 takes, and so on for the rows below. Two by default; zero switches the row off.

**FileMode** Overwrite replaces the named file. Append records after the end of it, so a session can be built up over several passes.

**PunchMode** Manual and SoundIn Sync stop on the button. Timed stops itself after RecordDuration and finalises the file.

**RecordDuration** How long a Timed pass runs, in milliseconds.

**Record** Arms and disarms every file at once. It switches itself back off when no row has channels to write, and a take must be stopped here rather than abandoned, since stopping writes the header that makes the file readable.

## Recipe

**Drum stem** Put the FileRecorder between the drum organism and the Mixer, leave File_1 empty, Ch at 2, PunchMode Manual and FileMode Overwrite. Arm Record for the pass and disarm at the end; the row shows where the take landed, and the mix carried on unchanged while it was written.

## Related Organisms

AudioTrack, SoundOut, Mixer, Bus
