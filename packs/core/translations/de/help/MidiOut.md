# MidiOut

Schickt MIDI, das an seinem Eingang ankommt, an ein Hardwaregerät oder eine andere Anwendung.

Verbinde eine PianoRoll, ein Microdot, einen Arpeggiator oder ein MidiIn damit, und die Noten und Controller verlassen es über den Port, den du wählst. Das Patch speichert die Portnummer statt des Gerätenamens, sodass es sich auf einem anderen Rechner mit dem Gerät öffnet, das dort diesem Port zugewiesen ist. Die Nachrichten-Parameter machen das Organism außerdem zu einer eigenen MIDI-Quelle: Eine Änderung von Value sendet den gewählten Controller auf dem gewählten Channel, und Umschalten von Gate sendet ein Note-On und ein Note-Off, sodass eine Automationsspur oder ein zugewiesener Regler externe Hardware ganz ohne MIDI-Kabel steuern kann. Kombinier es mit einem MidiMonitor, um zu sehen, was hinausgeht.

## Parameter

**Port** Welcher der acht MIDI-Ausgangsports die Nachrichten trägt. Weise jedem Port in den MIDI-Einstellungen ein Gerät zu; das Dropdown zeigt jeden Port mit seinem Gerät.

**Channel** Der MIDI-Kanal, 1 bis 16, auf dem die Controller- und Gate-Nachrichten hinausgehen. Noten, die am Eingang ankommen, behalten ihren eigenen Kanal.

**Controller** Die Controller-Nummer, die Value sendet. Eine Änderung sendet den aktuellen Value sofort auf der neuen Nummer.

**Value** Der Controller-Wert, 0 bis 127. Jede Änderung sendet eine Controller-Nachricht, also automatisiere ihn oder weise ihn einem Regler zu, um einen Hardware-Parameter aus dem Patch heraus zu fahren.

**Note** Die Notennummer, die Gate spielt, standardmäßig 60.

**Velocity** Die Velocity des Note-On, das Gate sendet.

**Gate** An sendet ein Note-On für Note mit Velocity; Aus sendet das passende Note-Off. Weise es einer Taste oder einem Pad zu.

## Rezept

**Hardware-Filterfahrt** Stell Port auf den Port, dem dein Synth zugewiesen ist, Channel auf den Kanal, auf dem er hört, und Controller auf die Nummer seines Filter-Cutoffs. Automatisiere Value auf der Timeline oder moduliere es mit einem LFO, und der Filter des Synths folgt dem Patch; verbinde eine PianoRoll mit dem Eingang, um die Noten über denselben Port zu spielen.

## Verwandte Organisms

MidiIn, MidiMonitor
