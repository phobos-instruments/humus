# MidiPlayer

A score player that loads a MIDI file and sounds all sixteen of its channels from its own audio outlets.

It reads .mid and .midi score files, .kar karaoke files and .mus, the compact score format of the early first-person shooters, and needs nothing else corded: choose a file and press Play. The voices are frequency modulation by default, a small bank of operator settings in the tradition of the home computer and the sound card, which is why it sounds the way it does; drop a sampled bank into the Bank slot and it plays recordings instead. Host tempo plays the score against the transport, or off lets it keep its own tempo map. Cord the outlets into a Fern or a Console; see FilePlayer for recorded audio and MidiIn for a score arriving from outside.

## Parameters

**File** The score to play. Click the slot to choose one or drag a file in.

**Bank** The instruments the channels play. The .wopn chip bank that ships with it is frequency modulation; an .sf2 sampled bank replaces the chips with recordings and ignores Chips.

**Play** Starts and stops the score. Stopping silences every sounding note; pressing Play after the score has run out starts from the top.

**Loop** Returns to the beginning when the score ends instead of falling silent.

**FollowHost** Host tempo: on, the transport's tempo drives the score; off, the score keeps the tempo it was written with, including changes the composer wrote in.

**Chips** How many sound chips play at once, one to four. One is six voices and a busy arrangement drops notes; four gives twenty-four.

**Level** Output level.

**Program1** The instrument channel 1 plays. 0 plays what the file asks for; anything else overrides it for that channel. The same for Program2 to Program16. Channel 10 is the drum channel and stays drums.

**Mute1** Silences channel 1, and so on for Mute2 to Mute16.

## Recipe

**Period score** Load a .mid, Host tempo off, Chips 4, Loop on, Level 0.7, and press Play. Mute the channel you want to play yourself and cord the outlets through a Fern with a short slap.

## Related Organisms

FilePlayer, MidiIn, pH
