# Riff

Ein Bassline-Sequencer mit sechzehn Schritten im Stil der Acid-Box, der MIDI-Noten mit Gate, Akzent und Slide pro Schritt sendet.

Jeder Schritt trägt eine Note, ein Gate, einen Akzent und einen Slide, die Pattern-Sprache der Acid-Box. Ein akzentuierter Schritt geht mit Velocity 118 raus, das liegt über der Akzentschwelle von Acid, und ein Slide-Schritt hält seine Note bis knapp nach dem nächsten Note-on, was Acid als Legato-Glide liest. Acht Bänke halten je ein eigenes Pattern; die Bank-Schaltflächen wählen, welches spielt und welches das Raster zeigt. Riff steuert auch jedes andere MIDI-Instrument, verbinde seinen MIDI-Ausgang also ruhig auch mit einem Sampler oder einem Rhizome.

## Das Raster

Zieh in der Notenspur, um die Tonhöhe eines Schritts zu setzen, das schaltet auch sein Gate; klick mit rechts auf einen Schritt, um daraus eine Pause zu machen. Die Reihen A und S schalten Akzent und Slide. Ein Slide in dieselbe Note bindet sie, statt sie neu anzuschlagen. Ein Rechtsklick auf die Reihen A oder S bietet Zufall, Löschen, Verschieben, Riffs laden und das Kopieren in eine andere Bank. Riffs laden öffnet einen Browser über deinem Bibliotheksordner Riffs, mit den zuvor geladenen unter Zuletzt verwendet und Datei öffnen für alles andere. Das gewählte Riff oder eine aufs Raster gezogene Datei liest MIDI-Dateien und Pattern-Dumps von Acid-Boxen in die angezeigte Bank und die folgenden ein.

## Parameter

**Transpose** Verschiebt das ganze Pattern in Halbtönen.

**SwingFollow** Follow lässt das Pattern den Groove des Patches vom Transport übernehmen statt vom eigenen Swing-Regler.

**Swing** Verzögert jedes zweite Sechzehntel, bis hin zu einem Triolen-Feeling. Wirkt nur, solange Follow aus ist.

**Gate** Länge einer Note ohne Slide, als Anteil des Schritts.

**Velocity** Velocity der normalen Noten ohne Akzent.

**Bank** Welches der acht Patterns spielt, A bis H.

**Nudge** Spielt das Riff um so viele Schritte später, oder unter null früher, und springt dabei vom Ende wieder an den Anfang. Das Pattern selbst bleibt, wie es geschrieben ist; so verschiebst du, wo eine Linie gegenüber dem Kick landet, ohne sie neu zu zeichnen.

## Rezept

**Acid-Linie** Verbinde den MIDI-Ausgang mit einem Acid. Zeichne einen Grundton auf die Schritte 1, 4, 7, 11 und 14, eine Oktave höher auf 8 und 16, Akzent auf die Schritte 1 und 11, Slide von 7 nach 8. Gate 0.5, Swing 0.2 mit Follow aus. Nimm Transpose für den Breakdown um 5 herunter.

## Verwandte Organisms

Acid, Steps, Microdot
