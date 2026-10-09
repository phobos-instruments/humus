# Atom

Vier Ringe aus gleichmäßig verteilten Schlägen, die die Noten arpeggieren, die du hältst, oder eine Tonleiter abschreiten, wenn du keine hältst.

Jeder Ring beantwortet eine Frage: wann. Er nimmt eine Anzahl Schritte und eine Anzahl Schläge und verteilt die Schläge so gleichmäßig wie möglich über den Kreis. Das ist die Regel hinter sehr vielen Tanzrhythmen der Welt: Drei Schläge in acht sind der Tresillo, fünf in acht der Cinquillo, sieben in zwölf die westafrikanische Standard-Glockenfigur. Welche Note erklingt, beantworten deine Hände. Verkable ein Keyboard oder eine PianoRoll mit dem Eingang, und jeder Schlag spielt die nächste Note des Akkords, den du hältst; hältst du nichts, schreiten die Ringe die gewählte Tonart und Tonleiter von selbst ab. Noten, die du spielst, werden zuerst auf die Tonleiter gezogen, sodass nichts außerhalb der Tonart landet. Atom läuft, solange der Transport spielt, und schickt alles aus einem MIDI-Ausgang.

Die Frontplatte zeigt die Ringe von außen nach innen, Ring 1 ganz außen. Leuchtende Punkte sind Schläge, hohle Punkte Pausen, und der helle Punkt ist der Schritt, der gerade spielt.

Die Presets gibt es in drei Familien. Rhythms sind die Tanzmuster von oben. Elements behandeln die Ringe als Elektronenschalen: Der innerste Ring ist die erste Schale, und jeder Ring ist so lang, wie seine Schale fassen kann (2, 8, 18 und 32 Schritte, von innen nach außen), seine Schläge sind die Elektronen darin, und jede Schale dreht sich einmal pro Takt, sodass schwerere Atome dichtere, schnellere Schichten füllen. Planets machen die Ringe zu Umlaufbahnen, deren Längen die Proportionen der echten wahren: acht Umläufe des Erdrings gegen dreizehn der Venus, Jupiters zwölf gegen Saturns neunundzwanzig, Neptun dreimal herum, während Pluto zweimal herumgeht, sodass die Ringe auseinanderdriften und wieder zusammenfinden, so wie die Planeten.

## Parameter

**Steps** Wie viele Schritte rund um den Ring, von 1 bis 32.

**Hits** Wie viele dieser Schritte erklingen. Der Ring verteilt sie so gleichmäßig, wie er kann. 0 bringt den Ring zum Schweigen.

**Turn** Dreht die ganze Figur um den Ring, einen Schritt nach dem anderen, sodass derselbe Rhythmus an einer anderen Stelle beginnt.

**Rate** Wie lange ein Schritt dauert: eine Viertel-, Achtel-, Sechzehntel- oder Zweiunddreißigstelnote. Fit bar dehnt stattdessen den ganzen Ring über einen Takt, egal wie viele Schritte er hat, und genau das ergibt einen echten Kreuzrhythmus: Ein Ring mit 3 und ein Ring mit 4, beide auf Fit bar, spielen drei gegen vier. Ringe auf einem Notenwert mit unterschiedlicher Schrittzahl laufen einfach weiter und driften gegeneinander, bis sie erst nach vielen Takten wieder zusammenkommen.

**Fix** Lässt den Ring bei jedem Schlag seine eigene Note spielen statt des Arpeggios, um eine Trommel oder eine Glocke anzusteuern.

**Note** Die Note, die ein fixierter Ring spielt.

**Order** Der Weg, den das Arpeggio durch die Noten nimmt: Up, Down, Up-down, Random oder As played, das die Reihenfolge behält, in der deine Finger gedrückt haben. Jeder Ring merkt sich seine eigene Stelle auf dem Weg.

**Octaves** Wiederholt die Noten über so viele Oktaven, aufwärts.

**Key** Der Grundton der Tonleiter, die die Ringe abschreiten.

**Scale** Die Tonleiter, die die Ringe abschreiten, wenn keine Note gehalten wird, und auf die gehaltene Noten gezogen werden. Chromatic lässt gehaltene Noten unverändert.

**Latch** Hält den Akkord, nachdem du losgelassen hast. Der nächste Akkord, den du spielst, ersetzt ihn.

**Gate** Wie viel eines Schritts jede Note dauert.

**Vel** Die Velocity jeder Note.

**Swing** Der eigene Swing dieses Organisms, verwendet, wenn Follow aus ist. Ein Ring auf Fit bar wird nie geswingt.

**Follow** Übernimmt den Swing vom Transport; schalte es aus, um ihn hier mit Swing einzustellen.

## Rezept

**Ein Arpeggiator** Verkable ein Keyboard mit Atom und Atom mit einem Synth. Lass Ring 1 auf 5 Schlägen in 16, setz Order auf Up-down und Octaves auf 2, starte die Wiedergabe und halte einen Akkord. Erhöh Hits für eine dichtere Linie, und verschieb die Akzente mit Turn.

**Drei gegen vier** Setz Ring 1 auf 4 Schritte und 4 Schläge und Ring 2 auf 3 Schritte und 3 Schläge, beide auf Fit bar mit Fix an und zwei verschiedenen Noten, und verkable den Ausgang mit einem Sampler. Nimm Ring 3 als Arpeggio obendrüber dazu.

**Eine Glockenfigur** Ein Ring mit 12 Schritten und 7 Schlägen auf Fit bar ergibt die lange Glockenfigur vieler west- und zentralafrikanischer Musik. Leg einen zweiten Ring mit 12 Schritten und 4 Schlägen darunter, für den Puls der Tanzenden, und dreh die Glocke, bis sie richtig sitzt.

## Verwandte Organisms

Steps, Sequence, Riff, DNA, PianoRoll
