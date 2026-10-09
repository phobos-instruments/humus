# PianoRoll

Eine Notenquelle im Loop, die ihre Clips über ihre MIDI-Kabel spielt, wo auch immer der Transport gerade steht.

Verkable ihren Ausgang mit einem beliebigen Instrument, einem Rhizome, einem Sampler oder einem MidiOut für Hardware, und starte die Wiedergabe; verkable sie mit dreien zugleich, und ein Riff steuert alle drei. Sie ist ein Loop, den du patchst, kein Part, den du arrangierst: Die Instrumente des Songs tragen ihre Clips auf der Timeline, während eine PianoRoll ihre eigenen im Patch trägt. Eine leere Notenrolle folgt trotzdem der Clock, sodass eine blasse Linie über das Raster läuft und nach dem letzten Takt wieder von vorn beginnt, noch bevor es eine Note zu spielen gibt. Die Aufnahme erfasst, was du spielst, egal was der MIDI-Empfangsmodus des Organisms sagt, und ein ganzer Take lässt sich in einem Schritt widerrufen.

## Die Notenrolle

Zeiger wählt aus und verschiebt, Zeichnen erstellt, Schere teilt, Radierer wischt weg, auf den Tasten 1 bis 4. Zieh am Ende einer Note, um ihre Länge zu ändern, und mit Alt-Ziehen nach oben oder unten änderst du die Velocity; klick mit rechts auf eine Note oder die Auswahl, um ihr eine Farbe zu geben, sie auf ein Raster zu quantisieren, den Groove auf sie anzuwenden oder sie zu löschen, und die Pfeiltasten verschieben die Auswahl um einen Halbton oder einen Rasterschritt. Der Streifen unter dem Raster ist die Velocity-Spur; klick mit rechts darauf, um zu einer Controller- oder Pitch-Bend-Spur zu wechseln und dort die Kurve zu zeichnen. In einer Controller-Spur klickst du einen Punkt an, um ihn auszuwählen, mit Shift-Klick fügst du einen weiteren hinzu, und mit Ctrl- oder Cmd-Ziehen ziehst du einen Rahmen um mehrere; dann klick mit rechts, um sie einzufärben oder zu löschen. Die Loop-Schaltfläche ist ein Looper mit nur einer Taste: Tipp bei einem leeren Clip, um aufzunehmen, tipp noch mal, um den Loop an einem ganzen Takt zu schließen, danach schaltet jedes Tippen zwischen Dub und Play um. Rec nimmt auf, was du auf einem beliebigen MIDI-Eingang spielst, direkt in die Notenrolle, und Q quantisiert es aufs Raster. Keine der beiden wartet auf die Aufnahme des Transports, und beide laufen weiter, wenn der Editor geschlossen ist.

## Parameter

**Bars** Die Clip-Länge in Takten.

**Channel** Der MIDI-Kanal, auf dem die Noten hinausgehen.

**Mute** Schaltet den Ausgang stumm, ohne etwas zu trennen. Gehaltene Noten werden losgelassen, wenn es angeht.

**VelocityScale** Multipliziert die Velocity jeder Note auf dem Weg nach draußen. Bei 1 spielen sie wie gezeichnet.

**SwingFollow** Follow lässt die Notenrolle den Patch-Groove vom Transport übernehmen statt ihres eigenen Swing und SwingUnit.

**Swing** Verzögert jede zweite Einheit, bis hin zu einem Triolen-Feeling. Wird nur verwendet, solange Follow aus ist.

**SwingUnit** Der Notenwert, auf den Swing wirkt, 1/8 oder 1/16.

**Record** Nimmt Live-MIDI in die Notenrolle auf, solange es an ist, und startet den Transport, wenn er steht. Es wird nie eingeschaltet gespeichert. Klick mit rechts auf Rec, um es zuzuweisen.

**Loop** Ein Druck ist ein Looper-Tippen: aufnehmen, Loop schließen, overdubben, abspielen. Klick mit rechts auf Loop, um die Notenrolle zu leeren oder es zuzuweisen.

**Quantize** Rastet aufgenommene Noten auf das Raster der Notenrolle ein. Klick mit rechts auf Q, um es zuzuweisen.

## Rezept

**Ein Riff, drei Stimmen** Bars 2, zeichne eine zweitaktige Bassfigur und verkable den Ausgang zugleich mit einem Rhizome, einem Sampler und einem MidiOut. VelocityScale 0.8, Follow an, damit die Figur den Patch-Groove übernimmt, und weise Mute einem Button zu, um die Linie für einen Break wegzunehmen.

## Verwandte Organisms

MidiIn, Sequence, Sampler
