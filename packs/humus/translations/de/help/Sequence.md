# Sequence

Ein Drum-Raster mit sechzehn Schritten und einer bis acht Zeilen, in dem jede Zeile ihr eigener MIDI-Ausgang ist.

Jede Zeile schickt ihre Noten aus ihrem eigenen MIDI-Ausgang, sodass Zeile 1 einen Kick steuern kann, Zeile 2 einen Sampler und Zeile 3 ein Silt, und das Namensschild auf der Zeile zeigt, womit sie verbunden ist. Der letzte Ausgang ist der Master und führt alle Zeilen zusammen, für einen einzelnen Drum-Sampler, der das ganze Kit nach Noten verteilt. Acht Bänke enthalten je ein volles Raster; die Noten, Velocity-Werte und Aktivierungen der Zeilen sind das Kit und bleiben in allen Bänken gleich. Das Pattern ist ein Takt aus Sechzehnteln, und Änderungen greifen, während der Transport läuft.

## Das Raster

Klick auf einen Schritt, um ihn zu setzen, klick noch einmal, um ihn zu löschen, und zieh, um mehrere Schritte am Stück zu malen. Die laufende Spalte leuchtet, während der Transport läuft. Die LED links an der Zeile schaltet die Zeile ein und aus, der Regler ist ihre Velocity, und das Mausrad über dem Namensschild ändert die Note der Zeile. Ein Rechtsklick auf einen Schritt bietet Zufall, Löschen und In Bank kopieren, um ihn in eine andere Bank zu kopieren. Ein Rechtsklick auf einen Schritt, die LED oder das Namensschild bietet Zeile von Zufall ausnehmen: Jeder Zufall, auch der des Rasters, lässt dann die Schritte und die Velocity dieser Zeile, wie sie sind, und auf ihrem Namensschild erscheint ein Schloss. Um nur eine Velocity zu behalten, klick mit rechts auf ihren Regler und wähle Von Zufall ausnehmen, wie bei jedem Regler.

**Schritt-Wahrscheinlichkeit** Klicke auf den %-Schalter unter Gate, um die Wahrscheinlichkeit jedes gesetzten Schritts als Balken zu sehen, und ziehe dann auf einem Schritt nach oben oder unten, um festzulegen, wie oft er spielt, von nie bis immer in 5-%-Schritten. Cmd-Ziehen auf einem Schritt (Strg-Ziehen unter Windows und Linux) macht dasselbe, ohne umzuschalten. Der %-Schalter ist der Parameter View, also schaltet ihn auch ein zugewiesener Knopf oder ein Pad. Der Regler neben der Velocity jeder Reihe ist die Chance der Reihe: Sie gilt für alle Schritte dieser Reihe, sodass ein einziger Fader eine Hi-Hat-Linie ausdünnt. Rechtsklicke den Regler oder in der Chance-Ansicht einen Schritt der Reihe, um ihn zuzuweisen. Ein Schritt unter 100 % erscheint blasser und zeigt seine Wahrscheinlichkeit auf sich. Die Würfel gehören der Box: Ein Patch spielt bei jedem Abspielen und jedem Bounce dieselben Schritte, und zwei Boxen würfeln jeweils ihre eigenen. Jede Bank behält ihre Wahrscheinlichkeiten, In Bank kopieren nimmt sie mit und Löschen setzt sie wieder auf sicher.

## Parameter

**SwingFollow** Follow lässt das Raster den Groove des Patches aus dem Transport übernehmen statt den eigenen Swing-Regler.

**Swing** Verzögert die Offbeat-Sechzehntel, von gerade bei 0 bis zu einem vollen Triolen-Shuffle bei 1. Wirkt nur, solange Follow aus ist.

**Gate** Notenlänge als Bruchteil eines Schritts.

**Enable_1** Ob Zeile 1 spielt; die Schritte bleiben erhalten, solange sie aus ist. Ebenso für die Zeilen 2 bis 8.

**Vel_1** Velocity der Noten von Zeile 1. Ebenso für die Zeilen 2 bis 8.

**Note_1** Die MIDI-Note, die Zeile 1 sendet, standardmäßig nach der üblichen Drum-Belegung, 36 für einen Kick. Synth-Drums wie Kick ignorieren sie; Sampler ordnen sie zu. Ebenso für die Zeilen 2 bis 8.

**Bank** Welches der acht Raster spielt, A bis H.

**View** Steps oder Chance: was das Raster zeigt und bearbeitet. Der %-Schalter unter Gate schaltet es um.

**Chance_1 bis Chance_8** Wie oft die Schritte jeder Reihe spielen, zusätzlich zur Wahrscheinlichkeit jedes Schritts.

**Rows** Wie viele Zeilen das Raster hat, von 1 bis 8. Die Ausgänge folgen: einer pro Zeile, und der Master immer zuletzt, sodass ein Kabel am Master auch am Master bleibt, wenn sich die Anzahl ändert. Zeilen, die du wegnimmst, behalten ihre Schritte, Noten und Velocity-Werte und kommen so zurück, wie sie waren, wenn du sie wieder hinzufügst; nur die Kabel an ihren Ausgängen werden entfernt.

## Rezept

**Four on the Floor** Verbinde Zeile 1 mit einem Kick, Zeile 2 mit einem Cicada oder einem Clap im Sampler, Zeile 3 mit einem Hi-Hat-Sample. Schritte 1, 5, 9 und 13 in Zeile 1, 5 und 13 in Zeile 2, jeder zweite Schritt in Zeile 3. Swing 0.15 mit Follow aus, Gate 0.5. Zeichne in Bank B ein Fill und schalte Bank für den Übergang über einen Button um.

## Verwandte Organisms

Riff, Steps, PianoRoll
