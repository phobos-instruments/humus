# Hands

Ein Hand-Tracker, der Finger- und Handbewegungen in MIDI und Steuerwerte verwandelt.

Zeig der Kamera eine Hand, und jeder Finger, die Position des Handgelenks und eine Pinch-Geste werden zu geglätteten Signalen an einem MIDI-Ausgang, ohne Audio. Verbinde Hands per Kabel mit einem Synth-Organism oder einem MidiOut, oder klick mit rechts auf einen beliebigen Regler und wähl unter Steuern mit den Eintrag Hands, um einen Parameter ganz ohne Kabel zu steuern. Zwei Hände werden in festen Bänken links und rechts verfolgt: Die linke Bank sendet CC-Basis +0 bis +8 (Öffnung von Thumb, Index, Middle, Ring und Pinky, dazu X, Y, Present und Pinch), die rechte Bank +9 bis +17. Vier Gesten-Slots senden jeweils eine Note, wenn die Hand vor der Kamera einer gelernten Pose entspricht. Der Video-Eingang nimmt ein Kabel von CameraIn, VideoPlayer oder einem beliebigen Video-Organism an; solange ein Kabel steckt, liest der Tracker dieses Bild, und seine eigene Kamera bleibt geschlossen.

## Gesten lernen

Drück auf einem Slot auf Lernen und halte die Pose etwa anderthalb Sekunden; die gemittelte Form wird zur Vorlage und im Patch gespeichert. Drück auf Verst. und zeig dieselbe Pose noch einmal aus einem anderen Winkel, um sie in die Vorlage einzuarbeiten. Die Markierung auf dem Trefferbalken ist die Auslöseschwelle dieses Slots; zieh sie nach rechts, damit nur eine klar gehaltene Pose die Note sendet. Wenn mehrere Vorlagen passen, unterdrückt die beste die anderen.

## Parameter

**Enabled** Öffnet die Kamera. Solange es aus ist, wird nichts aufgenommen, und ein eingestecktes Video-Kabel hält die Kamera in jedem Fall geschlossen.

**Camera** Welches Aufnahmegerät geöffnet wird, wenn es mehr als eines gibt.

**Mirror** Spiegelt das Bild, sodass eine Bewegung nach rechts auch den Wert nach rechts bewegt. Wenn das Quell-Organism schon spiegelt, schalte eines der beiden aus.

**BuiltIn** Nutzt den eingebauten Tracker statt des System-Trackers. Er ist langsamer und läuft ohnehin überall dort, wo das System keinen eigenen Tracker hat.

**TwoHands** Verfolgt eine zweite Hand in der rechten Bank. Aus folgt einer einzigen Hand.

**Confidence** Wie sicher sich der Detektor sein muss, bevor er eine Hand annimmt. Erhöh den Wert, wenn Lampen oder Gesichter Phantomhände erzeugen, senk ihn bei Handschuhen oder wenig Licht.

**Smooth** Glättung der Reaktion in Millisekunden. Klein ist zappelig, groß ist ruhig und langsamer.

**Tolerance** Wie locker eine Live-Pose zu einer Vorlage passen darf und trotzdem als Treffer zählt. Treffer sind stufenlos, sie können also als Velocity anschwellen, statt hart umzuschalten.

**SendCC** Sendet die achtzehn Bewegungswerte als Continuous Controller. Aus lässt nur die Gesten-Noten übrig.

**SendNotes** Sendet für jeden Gesten-Treffer eine Note. Ausschalten lässt jede gerade gehaltene Note los.

**SendOSC** Veröffentlicht zusätzlich jeden Wert unter dem OSC-Namen des Organisms.

**MidiCC** Die erste CC-Nummer; die achtzehn Werte zählen von ihr aus aufwärts.

**MidiChannel** Der MIDI-Kanal für die CCs und Noten.

**Gestures** Die gelernten Vorlagen, im Patch gespeichert. Lernen und Verst. schreiben sie; hier gibt es nichts einzutippen.

**GNote_1** Die Note, die Gesten-Slot 1 sendet, mit einer Velocity je nach Trefferstärke; ebenso für 2 bis 4.

**GThresh_1** Die Trefferstärke, ab der Slot 1 auslöst; das Loslassen liegt knapp darunter, damit eine wackelige Pose nicht flattert; ebenso für 2 bis 4.

## Rezept

**Pinch-Filter** Enabled an, TwoHands aus, Smooth 150. Schick eine Audioquelle durch einen Filter, klick mit rechts auf den Cutoff-Regler des Filters, wähl unter Steuern mit den Eintrag Hands und dann pinch. Drück Daumen und Zeigefinger zusammen, um den Filter zu schließen, und öffne die Hand, um den Klang durchzulassen.

## Verwandte Organisms

CameraIn, Skeleton, SoundSpace, MidiOut
