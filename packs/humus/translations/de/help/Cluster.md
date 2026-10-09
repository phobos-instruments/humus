# Cluster

Acht Akkord-Slots, die als ganze Voicings aus einem MIDI-Ausgang feuern.

Jeder Slot enthält eine Handvoll Noten, als Namen eingetippt, etwa C3 E3 G3; Kreuze, Bs und reine Notennummern gehen auch. Ein Slot feuert, wenn du seine Taste drückst, seine Trigger-Note spielst oder seinen Fire-Parameter über eine Steuerroute ansteuerst. Im Modus PADS liegen die acht Slots auf acht eingehenden Noten ab TriggerNote und klingen mit der Velocity, mit der du spielst. Im Modus FOLLOW wird ein Slot zu einer Form, die deinem Spiel folgt: Jede eingehende Note wird zum Grundton, und der Akkord wandert mit. Noten, die zwei klingende Slots gemeinsam haben, werden mitgezählt, sodass das Loslassen eines Pads nie eine Note abschneidet, die ein anderes noch hält. Verbinde den Ausgang mit einem Rhizome, einer Wave oder einem anderen Instrument.

## Parameter

**Mode** PADS spielt die acht Slots über acht Tasten. FOLLOW transponiert einen Slot nach dem, was du spielst.

**Hold** Was ein Druck bedeutet. GATE lässt den Akkord klingen, solange der Trigger gedrückt ist. LATCH macht jeden Trigger zu einem Schalter, sodass sich Pads überlagern können. PEDAL spielt immer nur einen Akkord: Jeder Druck ersetzt den letzten, und Loslassen wird ignoriert, sodass ein leerer Slot zur Stopp-Taste wird.

**TriggerNote** Die eingehende Note für Slot 1. Die nächsten sieben Noten aufwärts gehören zu den Slots 2 bis 8.

**Slot** Welchen Slot der Modus FOLLOW mitnimmt.

**Velocity** Die Velocity, wenn ein Slot über seine Taste oder eine Steuerroute ausgelöst wird.

**Notes1** Die Noten von Slot 1, als Namen oder Nummern eingetippt. Ebenso für 2 bis 8.

**Fire1** Der Trigger hinter der Taste von Slot 1, erreichbar für Steuerrouten und Automation. Ebenso für 2 bis 8.

## Rezept

**Akkordwechsel mit einem Finger** Mode PADS, Hold PEDAL, TriggerNote 36. Tipp eine Akkordfolge in die Slots 1 bis 4 und lass Slot 5 leer. Verbinde eine PianoRoll mit dem Eingang und den Cluster mit einem Rhizome, dann zeichne einzelne Noten auf 36 bis 39 dort, wo der Akkord wechseln soll, und eine 40 dort, wo er aufhören soll.

## Verwandte Organisms

PianoRoll, Steps, Riff, Harmonizer
