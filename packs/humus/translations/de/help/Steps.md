# Steps

Ein einreihiger Trigger-Sequencer mit sechzehn Schritten, der eine MIDI-Note mit Swing, Gate und Velocity sendet.

Eine Reihe ist eine Stimme: Gib ihr eine Note und verkable ihren MIDI-Ausgang mit einem Kick, einem Mineral oder einem Sampler; stapel dann mehrere Steps-Organisms, eines pro Drum, jedes mit eigenem Swing und Gate. Es bleibt still, bis der Transport spielt, und die Schritte werden aus der Schlagposition des Transports gelesen, sodass es immer im Timing des Patches landet. Ein Anschlag dauert Gate eines Schritts, außer die Halte-Reihe bindet ihn in die folgenden Schritte. Das Kabel einer Reihe oder ihren Kanal am Mixer stummzuschalten, ist das Stummschalten für die Live-Performance.

## Das Raster

Klicke auf einen Schritt, um einen Anschlag zu setzen, und klicke noch einmal, um ihn zu löschen. Die ^-Reihe darüber hebt diesen Anschlag um eine Oktave an. Die Halte-Reihe darunter bindet einen Anschlag über die nächste Schrittgrenze, und eine Folge von Haltebögen ergibt eine lange Note.

**Schritt-Wahrscheinlichkeit** Klicke auf die %-Taste oben rechts am Raster, um die Wahrscheinlichkeit jedes Schritts als Balken zu sehen, und ziehe dann auf einem Schritt nach oben oder unten, um festzulegen, wie oft er spielt, von nie bis immer in 5-%-Schritten. Cmd-Ziehen (Strg-Ziehen unter Windows und Linux) macht dasselbe, ohne umzuschalten. Die %-Taste ist der Schalter View, also schaltet ihn auch ein zugewiesener Knopf oder ein Pad. Die Wahrscheinlichkeit jedes Schritts ist auch ein Parameter, Chance_1 bis Chance_16, sodass sich Fader darauf legen lassen. Rechtsklicke in der Chance-Ansicht einen Schritt, um ihn zuzuweisen. Ein Schlag unter 100 % erscheint blasser und zeigt seine Wahrscheinlichkeit über sich. Die Würfel gehören der Box: Ein Patch spielt bei jedem Abspielen und jedem Bounce dieselben Schläge, und zwei Boxen würfeln jeweils ihre eigenen. Jede Bank behält ihre Wahrscheinlichkeiten, In Bank kopieren nimmt sie mit und Löschen setzt sie wieder auf sicher.

## Parameter

**Note** Die MIDI-Note, die jeder Anschlag sendet. 36 ist C1, der übliche Kick.

**SwingFollow** Follow lässt die Reihe den Groove des Patches vom Transport übernehmen statt von ihrem eigenen Swing-Regler.

**Swing** Verzögert jede zweite Sechzehntel, bis hin zum Triolengefühl. Wird nur benutzt, solange Follow aus ist; gib den Hats mehr Swing als dem Kick, und der Groove öffnet sich.

**Gate** Länge des Anschlags als Bruchteil des Schritts. Bindungen reichen darüber hinaus.

**Velocity** Velocity jedes Anschlags. Automatisiere sie für Build-ups.

**Bank** Welches der acht Patterns spielt, A bis H. Jede Bank behält ihre eigenen Schritte und ihre eigene ^-Reihe, und ein Wechsel spielt sofort die neue.

**View** Steps oder Chance: was das Raster zeigt und bearbeitet. Die %-Taste schaltet es um.

**Chance_1 bis Chance_16** Wie oft jeder Schritt spielt, von 0 bis 100 %. Jede Bank behält ihre eigenen.

## Rezept

**Hats über einem Kick** Zwei Steps-Organisms. Das erste, Note 36, schlägt auf den Schritten 1, 5, 9 und 13 an und geht in einen Kick, bei dem FourFloor aus ist. Das zweite, Note 42, schlägt auf jedem zweiten Schritt an, mit Follow aus und Swing 0.3, und geht in eine Sampler-Zone mit einer Hi-Hat. Hebe ein paar Hat-Schritte mit der ^-Reihe an, für Akzente.

## Verwandte Organisms

Kick, Riff, Acid
