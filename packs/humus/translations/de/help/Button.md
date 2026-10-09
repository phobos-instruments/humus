# Button

Ein Taster als patchbares Organism, mit einem Steuerausgang und einer MIDI-Note.

Drück ihn, und der Ausgang wandert von Off nach On, sodass jeder Regler im Patch ihm über ein Steuerkabel oder über eine Steuern-mit-Route folgen kann, und eine Note verlässt den MIDI-Ausgang. Lass los, und beides kehrt zurück. Latch macht aus dem Drücken einen Umschalter, und damit wird er zu einer einfachen Stummschaltung, zu einem Szenenwechsler oder zu einer Art, ein Gate offen zu halten, ohne den Finger draufzulassen. Weise Press per MIDI-Learn einem Pad, einer Taste oder einem Fußschalter zu, und der Button lebt auf deinem Controller; verkable das Gate eines Followers mit der Press-Buchse, und das Patch drückt ihn für dich. Alles, was per Kabel in seinen Eingang kommt, wird zum Ausgang addiert.

## Parameter

**Press** Der Button selbst. Tastend, außer Latch ist an.

**Latch** Jeder Druck schaltet den Zustand um, statt dem Finger zu folgen.

**Off** Der Pegel des Ausgangs, solange der Button aus ist. Er darf größer sein als On, oder negativ für einen bipolaren Schalter.

**On** Der Pegel des Ausgangs, solange der Button an ist.

**Slew** Wie lange der Ausgang braucht, um den neuen Pegel zu erreichen, in Millisekunden. Null springt sofort, was durch einen Gain hindurch klickt, aber genau das ist, was ein Trigger will.

**Note** Die MIDI-Note, die gesendet wird, solange der Button an ist: Note-on, wenn er aufleuchtet, Note-off, wenn er dunkel wird.

**Velocity** Die Velocity dieser Note.

## Rezept

**Kill-Switch** Latch an, Off 0, On 1, Slew 40. Verkable den Ausgang mit dem Mute eines Gains auf einem Drum-Bus und weise Press per MIDI-Learn einem Pad zu. Ein Tippen schneidet die Drums weg, das nächste bringt sie zurück, ohne Klick.

## Verwandte Organisms

Slider, Number, Gate, Kick
