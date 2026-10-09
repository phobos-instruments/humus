# MidiFilter

Lässt gewählte Noten und Controller durch, hält andere zurück oder macht aus einem etwas anderes.

Verbinde ein Keyboard, ein MidiIn oder einen Sequencer mit dem Eingang und den Ausgang mit einem Instrument oder einem MidiOut. Es beginnt mit einer leeren Regel, die mit einem Plus markiert ist: Wähl darin Note oder CC, um eine Regel anzulegen, und die nächste leere erscheint darunter, bis zu acht. Wähl Off, um eine Regel wieder zu entfernen. Sobald es mehr Regeln gibt, als der Bereich zeigt, scrollt er. Klick ein Notenfeld an und spiel eine Taste auf deinem MIDI-Keyboard, um diese Note zu wählen. Jede Regel horcht auf eine Note oder einen Controller oder auf einen Bereich davon und entscheidet, was passiert: Block hält es auf, To note schickt es als andere Note weiter, To CC schickt es als Controller weiter. Die Regeln werden von oben nach unten gelesen, und die erste, die passt, gewinnt, also macht eine enge Regel über einer weiten eine Ausnahme. Ein Bereich bewegt sich als Ganzes: C1 bis B1, geschickt auf C4, spielt die ganze Oktave drei Oktaven höher. Eine losgelassene Note erreicht immer die Note, zu der ihr Anschlag ging, auch wenn du eine Zeile änderst, während sie gehalten wird.

## Parameter

**In1** Worauf Zeile 1 horcht: Off, Note oder CC. Und so weiter für die Zeilen 2 bis 8.

**Low1** Die tiefste Note oder Controllernummer, die Zeile 1 erfasst. Und so weiter für die Zeilen 2 bis 8.

**High1** Die höchste Note oder Controllernummer, die Zeile 1 erfasst. Setz sie gleich Low für eine einzelne. Und so weiter für die Zeilen 2 bis 8.

**Do1** Was Zeile 1 mit einem Treffer macht. Block hält ihn auf. To note schickt eine Note, und ein Controller wird zu einer Note, die spielt, wenn sein Wert die Hälfte erreicht, und loslässt, wenn er wieder darunter fällt. To CC schickt einen Controller, und eine Note wird zu diesem Controller, beim Anschlag mit ihrer Velocity und beim Loslassen mit null. Und so weiter für die Zeilen 2 bis 8.

**Target1** Die Note oder der Controller, auf den Zeile 1 ihren tiefsten Treffer schickt; der Rest des Bereichs folgt der Reihe nach. Und so weiter für die Zeilen 2 bis 8.

**Others** Was mit allem passiert, das keine Zeile erfasst, einschließlich Pitch Bend, Aftertouch und Programmwechseln. Pass lässt es durch. Block behält nur, was die Zeilen schicken.

## Rezept

**Ein Pad, das einen Filter fährt** Zeile 1: In Note, Low und High C1, Do To CC, Target 74. Verbinde den Pad-Controller mit dem Eingang und den Ausgang mit einem Synth, dessen Cutoff CC 74 zugewiesen ist: C1 härter anschlagen öffnet den Filter weiter, und Loslassen schließt ihn.

**Nur die Drums** Zeile 1: In Note, Low C1, High B2, Do To note, Target C1. Others Block. Alles über den zwei untersten Oktaven und jeder Regler bleibt aus dem Drumcomputer draußen.

## Verwandte Organisms

MidiIn, MidiOut, MidiMonitor, Cluster
