# Kick

Ein synthetischer Club-Kick mit Tonhöhenhüllkurve, Körper-Decay, Rauschklick und Drive-Stufe.

Ein Sinus auf Tune startet um Punch höher und fällt über PitchDecay ab, sein Körper klingt über Decay aus, ein kurzer Rauschstoß liefert den Transienten, und eine Sättigungsstufe rundet das Ganze ab, sodass der Kick entworfen statt gesampelt ist. Er wird auf drei Arten ausgelöst, die sich kombinieren: mit dem Hit-Button, mit jedem Note-on am MIDI-Eingang von einer PianoRoll oder einem Steps und mit 4/4, das ihn auf jedem Schlag des Transports auslöst, solange er läuft. Verbinde den Ausgang mit einem Compressor oder einer Console, und häng einen Follower daran, dessen Ausgang du für ein Sidechain-Ducking auf einen Gain ziehst.

## Parameter

**Tune** Die Grundfrequenz des Körpers, 30 bis 90 Hz. Das Feld neben dem Regler zeigt sie als Note: Klick darauf und drück eine Taste, auf einem MIDI-Keyboard oder auf dem Klavier, das sich öffnet, um den Kick auf diese Note zu stimmen.

**Punch** Wie weit die Tonhöhe über Tune beginnt. Mehr ergibt einen härteren Anschlag.

**PitchDecay** P.Dec: wie schnell die Tonhöhe auf Tune fällt, in Millisekunden. Kurz ist ein Tick, lang ist ein Laser.

**Decay** Ausklingzeit des Körpers in Millisekunden, die Länge des Booms.

**Click** Pegel des Rauschtransienten, damit er sich in einem dichten Mix durchsetzt.

**Wood** Ein kurzes Klopfen aus zwei unharmonischen Teiltönen, die Tune folgen, nach etwa 35 ms verklungen. Ein wenig gibt einen Schlägel-Charakter, viel geht in Richtung Woodblock. 0 lässt den reinen Klang.

**Drive** Sättigung. Dreh sie auf für ein verzerrtes Grollen.

**Level** Ausgangspegel.

**FourFloor** 4/4: löst den Kick auf jedem Schlag aus, solange der Transport läuft.

**Trigger** Hit: löst den Kick einmal aus. Weise ihn einem Pad zu.

## Rezept

**Four on the floor** Tune 48, Punch 4, PitchDecay 40, Decay 420, Click 0.35, Drive 0.3, 4/4 an. Starte die Wiedergabe, und der Kick folgt dem Transport; häng einen Follower an seinen Ausgang und zieh den env des Followers auf einen Gain am Bass für ein Ducking.

## Verwandte Organisms

SideKick, Microdot, Steps
