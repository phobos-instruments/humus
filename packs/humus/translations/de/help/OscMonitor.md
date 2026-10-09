# OscMonitor

Ein Log der OSC-Nachrichten, die durch die App laufen, in beide Richtungen, als lesbare Zeilen.

Er hat keine Pins: Leg ihn irgendwo ab, und er listet jede Nachricht, die am OSC-Eingangsport ankommt, ob einem Parameter zugewiesen oder nicht, und jeden Wert, den ein OSC sendendes Organism wie Hands überträgt, mit der Adresse, an die er ging. Nutze ihn, um zu prüfen, ob ein Handy-Controller oder eine Sensor-Bridge den Rechner wirklich erreicht, um die Adresse zu finden, die ein Fader sendet, bevor du ihn zuweist, und um zu bestätigen, was ein Empfänger erwarten sollte. Der OSC-Eingang muss in den Einstellungen an sein, damit empfangene Zeilen erscheinen; die Kopfzeile zeigt an, wenn er aus ist. Pause hält die Liste an, und Leeren leert sie.

## Parameter

**Direction** Zeigt die Zeilen für All, Received oder Sent. Eine getrackte Hand sendet ununterbrochen, sodass Sent die Ansicht fluten kann; herausgefilterte Zeilen werden verworfen, nicht aufbewahrt.

## Rezept

**Eine Adresse finden** Stell Direction auf Received, richte den Controller auf den OSC-Port des Rechners und beweg einen Fader. Die Adresse, die erscheint, ist die, die du der OSC-Zuweisung des Parameters gibst.

## Verwandte Organisms

Hands, MidiMonitor
