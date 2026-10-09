# Deck

Ein DJ-Deck für Audiodateien und Videos: Pitch-Fader, Beatgrid, Cues, Loops und Steuerung per Timecode-Vinyl.

**Timecode-Vinyl** Getestet und unterstützt mit Traktor Timecode Vinyl MK2 und mit Phase Essential. Schalte Timecode ein, verbinde den Plattenspieler über ein Phono-Organism mit den Eingängen des Decks, und die Platte treibt den Teller an: Scratchen, Backspins, und das Anheben der Nadel stoppt ihn. Das Deck erkennt die Pressung und lernt ihren Ton, sobald die Nadel aufsetzt. Läuft der Teller bei normaler Geschwindigkeit trotzdem zu schnell oder zu langsam, spiel die Platte auf 33 (oder 45) und drück Calibrate: Dieser Ton wird zur normalen Geschwindigkeit des Decks, mit dem Patch gespeichert, und das Deck hört auf, ihn anzuzweifeln. Drück Calibrate mit angehobener Nadel, damit es ihn wieder selbst lernt. Die Platte darf vor dem Anfang des Titels oder über sein Ende hinaus laufen: Das Deck spielt dort Stille und läuft wieder synchron mit, sobald die Platte zurück im Titel ist. Wähl die Platte in der Liste unter Timecode (und 45 rpm, wenn sie auf 45 läuft): Die normale Geschwindigkeit kommt dann von der Platte selbst, und Absolute lässt den Titel der Nadelposition folgen, sodass ein Nadelaufsetzen an der richtigen Stelle im Titel landet und Scratchen nicht wegdriften kann. Wenn du eine Platte zum ersten Mal wählst, braucht ihre Tabelle ein paar Sekunden zum Aufbauen, und das Deck folgt so lange der Geschwindigkeit der Platte. Wenn gar nichts mitläuft, schau auf das Oszilloskop neben dem Schalter: Ein sauberer Ring ist Timecode, eine diagonale Linie heißt, dass beide Kanäle dasselbe führen, eine Wolke heißt, dass es überhaupt kein Timecode ist.

**Beatmatching** Beim Laden eines Titels werden BPM, Raster und Tonart erkannt. Schalte bei zwei Decks Sync ein, und beide folgen dem Tempo des Patches, im Takt mit jedem Sequencer und jedem Delay im Patch.

**Videos** Lade ein Video, und das Deck spielt seinen Ton und schickt sein Bild aus dem Videoausgang; das Bild folgt dem Teller, Scratchen eingeschlossen. PictureSync verschiebt das Bild in Millisekunden gegen den Ton, wenn dein Bildschirm zu spät oder zu früh dran ist.

**Peak** Drück Peak, und das Deck loopt den lautesten Teil des Titels für acht Schläge, beginnend auf dem Schlag davor, wenn Quantize an ist, sodass du die Pegel an der lautesten Stelle einstellen kannst. Drück noch einmal für die zweitlauteste und noch einmal für die drittlauteste; alle drei sind in der Übersicht markiert. Die Zahl neben dem Knopf ist das lauteste Sample im Titel und wird bei Vollaussteuerung rot. Das Bild eines Videos folgt, sodass auch seine lauteste Szene loopt.

**Abspielposition überall** Klick mit rechts auf einen beliebigen Regler im Patch, und Position, Geschwindigkeit und Sekunden des Decks stehen unter Steuern mit.

## Parameter

**File** Die Audiodatei oder das Video, das das Deck spielt.

**AudioTrack** Welche Tonspur eines Videos gespielt wird, wenn es mehr als eine hat.

**Record** Schaltet das Deck scharf: Was an seinen Eingängen ankommt, während das Patch aufnimmt, landet im Ordner Recordings des Projekts und zurück auf diesem Teller.

**FileSound** Der gespielte Ton, wenn es nicht der aus File ist, gesetzt durch einen aufgenommenen Take.

**Active** Wiedergabe oder Stopp, mit einer kurzen Blende, damit nichts klickt.

**Loop** Loopt zwischen LoopIn und LoopOut.

**Sync** Folgt dem Tempo des Patches und richtet die Schläge am Raster aus.

**Keylock** Ändert das Tempo, ohne die Tonhöhe zu ändern. Wirkungslos, solange Timecode an ist.

**PictureSync** Angezeigt als Audio delay: verschiebt das Bild gegen den Ton, bis zu einer Sekunde in jede Richtung, pro Deck. Positiv hält den Ton hinter dem Bild zurück; kommt der Ton nach dem Bild an, geh ins Negative.

**Timestamp** Brennt die Zeit der Abspielposition des Decks in das Bild, das es ausgibt.

**Quantize** Lässt Cues und Loops auf dem nächsten Rasterschlag einrasten.

**Timecode** Lässt eine Timecode-Platte den Teller antreiben.

**TimecodeHz** Bleibt erhalten, damit ältere Patches unverändert öffnen; das Deck liest es nicht mehr. TimecodeRest hält die Kalibrierung.

**TimecodeFlip** Schalte es ein, wenn der Titel rückwärts läuft, während die Platte vorwärts läuft.

**TimecodeRest** Der Steuerton bei normaler Geschwindigkeit, gesetzt durch Calibrate. 0 heißt, das Deck lernt ihn selbst, sobald die Nadel aufsetzt.

**TimecodeCalibrate** Nimmt den gerade ankommenden Ton als normale Geschwindigkeit. Kommt kein Ton an, lernt das Deck ihn wieder selbst.

**TimecodeRecord** Welche Timecode-Platte auf dem Teller liegt. Learn from the record liest die Geschwindigkeit jeder Platte; nennst du sie beim Namen, kennt das Deck die exakte normale Geschwindigkeit, und Absolute kann die Nadel finden.

**Timecode45** Die Platte läuft auf 45 statt 33.

**TimecodeAbsolute** Der Titel folgt der Position der Nadel auf der Platte, abzüglich der Einlaufrille. Zwischen zwei Messungen, beim Scratchen und beim Rückwärtslaufen folgt er der Geschwindigkeit der Platte.

**BPM** Das Tempo des Titels.

**PitchPercent** Der Pitch-Fader, in Prozent.

**PitchRange** Wie weit der Fader reicht, 8 bis 50 Prozent.

**Volume** Ausgangspegel, nicht auf der Frontplatte: Stell den Pegel am Mixer ein oder weise diesen Parameter zu.

**GridOffset** Wo die erste Eins des Rasters sitzt, in Samples.

**LoopIn** Loop-Anfang, in Samples.

**LoopOut** Loop-Ende, in Samples.

**LoopBeats** Länge des Beat-Loops, von einem Viertelschlag bis zu 32 Schlägen.

**Peak** Loopt den lautesten Teil des Titels; jeder Druck geht zum nächsten der drei lautesten weiter.

**Cue** Der Haupt-Cue-Punkt, in Samples. Cue springt dorthin, ohne das Deck zu starten oder zu stoppen.

**Key** Die Tonart, die beim Laden des Titels erkannt wurde.

**HotCue_1** Der erste Hot Cue, in Samples; -1 heißt nicht gesetzt. Ein Druck auf einen leeren speichert die Abspielposition, ein Druck auf einen gespeicherten spielt von dort, Shift-Klick löscht ihn. Und so weiter für 2 bis 8.

## Rezept

**Mix mit zwei Decks** Lade auf jedes Deck einen Titel und prüf, ob das Raster auf den Schlägen liegt. Schalte bei beiden Sync ein, verbinde sie mit einem Mixer und lass das zweite über einen Hot Cue auf seiner ersten Eins einsteigen.

## Verwandte Organisms

AudioTrack, Sampler, Mixer, Console, Phono
