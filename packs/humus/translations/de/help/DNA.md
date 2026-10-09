# DNA

Ein generativer Sequencer, der ein Genom aus sechzehn Basen als MIDI-Noten spielt.

Sechzehn Basen, A, C, G und T, laufen mit Rate im Loop zum Transport, und jede Base spielt ihr Intervall über Root: das aus dem Voicing von Chord oder ein eigenes, das du auf den Basentasten unten im Editor einstellst. In jedem Takt gibt es mit Mutate eine Chance, dass eine Base umschlägt, sodass sich die Linie beim Spielen langsam entwickelt, während derselbe Seed immer denselben Start-Strang wachsen lässt. Der Streifen über den Basenzeilen zeigt den spielenden Schritt und die Pausen des aktuellen Durchlaufs; klick eine Zelle an, um diesen Schritt ganz stummzuschalten, zieh darüber, um mehrere zu erfassen, und klick mit rechts für den ganzen Strang. Verbinde den MIDI-Ausgang mit einem Acid, einem Mineral oder einem Substrate.

## Parameter

**Root** Die Tonart, als MIDI-Note.

**Chord** Das Voicing, das die vier Basen annehmen: Minor, Major, Phrygian, Pentatonic, Dorian, Harmonic Minor, Whole Tone, Hirajoshi oder Harmonic (JI), die Reihe 4:5:6:7 in reiner Stimmung.

**Rate** Die Schrittlänge, von 1/64 bis 1/1, mit Triolen- und punktierten Werten dazwischen.

**Gate** Notenlänge als Bruchteil des Schritts.

**Rest** Wie viel des Strangs zu Pausen ausgedünnt wird. Die Pausen fallen nach Seed, und Rest aufzudrehen fügt derselben Linie immer nur Pausen hinzu.

**Mute** Die von Hand auf dem Streifen stummgeschalteten Schritte, ein Bit pro Schritt. Ein stummgeschalteter Schritt erklingt nie, egal was Rest oder Mutate tun.

**Octaves** Bei 2 oder 3 springen manche Schritte für mehr Umfang eine Oktave nach oben. 1 lässt jede Base, wo sie ist.

**Mutate** Die Chance pro Takt, dass eine Base umschlägt. Null ist ein festes Riff; hoch ist rastlos.

**Velocity** Die Velocity jeder Note.

**Seed** Welcher Strang wächst. Derselbe Seed ergibt immer denselben Anfang.

**Base A** Das Intervall, das Base A über Root spielt, 0 bis 12 Halbtöne. -1 folgt dem Voicing von Chord.

**Base C** Dasselbe für Base C.

**Base G** Dasselbe für Base G.

**Base T** Dasselbe für Base T.

**SwingFollow** Mit Follow übernimmt das Pattern den Groove des Patches vom Transport statt vom Swing-Regler.

**Swing** Verzögert jeden zweiten Schritt in Richtung Triolengefühl. Wirkt nur, solange SwingFollow aus ist.

## Rezept

**Acid im Wandel** Verbinde den MIDI-Ausgang mit einem Acid. Root 45, Chord Phrygian, Rate 1/16, Gate 0.5, Rest 0.3, Octaves 2, Mutate 0.2. Drück Wiedergabe und lass es ein paar Takte laufen; wenn ein Strang erscheint, der dir gefällt, setz Mutate auf null, um ihn zu behalten.

## Verwandte Organisms

Mineral, Substrate, Rhizome
