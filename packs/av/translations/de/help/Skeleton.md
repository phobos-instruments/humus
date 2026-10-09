# Skeleton

Ein Ganzkörper-Tracker, der Haltung und Bewegung in MIDI- und Steuerwerte verwandelt.

Stell dich vor die Kamera, und dein Kopf, deine Hände, gehobene Arme, Neigung, Hocke und Stand werden zu geglätteten Signalen an einem MIDI-Ausgang, ohne Audio. Verbinde es per Kabel mit einem Synth-Organism oder einem MidiOut, oder klick mit der rechten Maustaste auf einen beliebigen Regler und wähl unter Steuern mit Skeleton, um ihn ohne Kabel zu steuern. Zwölf Werte liegen auf CC-Basis +0 bis +11: Kopf X und Y, linke Hand X und Y, rechte Hand X und Y, Heben links und rechts, Neigung, Hocke, Stand und Anwesenheit. Links und rechts sind deine eigenen, egal wie Mirror das Bild spiegelt. Es wird immer nur eine Person erfasst, im Bild von den Knien oder Füßen an aufwärts, und ein Wert, dessen Gelenke verdeckt sind, hält seinen letzten Stand. Der Video-Eingang nimmt ein Kabel von CameraIn, VideoPlayer oder einem beliebigen Video-Organism an; solange es angeschlossen ist, liest der Tracker dieses Bild und seine eigene Kamera bleibt zu.

## Posen lernen

Drück bei einem Slot auf Lernen und halte die Pose, etwa Arme verschränkt oder eine Faust gehoben, rund anderthalb Sekunden lang; die gemittelte Form wird zur Vorlage und im Patch gespeichert. Drück auf Verst. und nimm die Pose an einer anderen Stelle noch einmal ein, um auch das in die Vorlage einzurechnen. Eine Pose ist ihre Armgeometrie, keine Position im Bild, also löst sie aus, wo immer du stehst. Die Markierung auf dem Trefferbalken ist die Auslöseschwelle, und der beste Treffer unterdrückt die übrigen.

## Parameter

**Enabled** Öffnet die Kamera. Solange es aus ist, wird nichts erfasst, und ein angeschlossenes Videokabel hält die Kamera in jedem Fall geschlossen.

**Camera** Welches Aufnahmegerät geöffnet wird, wenn es mehr als eins gibt.

**Mirror** Spiegelt das Bild, sodass eine Bewegung nach rechts den Wert nach rechts bewegt. Spiegelt das Quell-Organism schon, schalte eins von beiden aus.

**Confidence** Wie sicher sich der Tracker sein muss, bevor er eine Person akzeptiert. Erhöh es, wenn Möbel oder ein Poster immer wieder für eine Person gehalten werden, senk es bei schlechtem Licht.

**Smooth** Glättung der Reaktion in Millisekunden. Höher ist ruhiger und langsamer.

**Tolerance** Wie locker eine Live-Pose zu einer Vorlage passen darf, um noch als Treffer zu gelten.

**SendCC** Sendet die zwölf Körperwerte als Continuous Controller. Aus lässt nur die Posen-Noten übrig.

**SendNotes** Sendet für jeden Posen-Treffer eine Note. Ausschalten gibt jede gerade gehaltene Note frei.

**SendOSC** Veröffentlicht zusätzlich jeden Wert unter dem OSC-Namen des Organisms.

**MidiCC** Die erste CC-Nummer; die zwölf Werte zählen von ihr aus aufwärts.

**MidiChannel** Der MIDI-Kanal für die CCs und Noten.

**Gestures** Die gelernten Vorlagen, gespeichert im Patch. Lernen und Verst. schreiben sie; hier gibt es nichts einzutippen.

**GNote_1** Die Note, die Posen-Slot 1 sendet, mit einer Velocity nach Trefferstärke, ebenso für 2 bis 4.

**GThresh_1** Die Trefferstärke, ab der Slot 1 auslöst; die Freigabe liegt knapp darunter, damit eine wackelnde Pose nicht flattert, ebenso für 2 bis 4.

## Rezept

**Anschwellen per Armheben** Enabled an, SendNotes aus, Smooth 200. Schick eine Audioquelle durch einen Gain, klick mit der rechten Maustaste auf seinen Gain-Regler, wähl unter Steuern mit Skeleton und dann raise/r. Heb den rechten Arm, um den Klang lauter zu machen, und senk ihn, um ihn wegzunehmen.

## Verwandte Organisms

Hands, CameraIn, MidiOut
