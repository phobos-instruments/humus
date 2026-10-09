# Acid

Ein monophoner Linien-Synth in der Tradition der Acid-Boxen, mit Accent und Slide.

Eine bandbegrenzte Sägezahn- oder Rechteckwelle läuft in einen resonanten Dioden-Ladder-Filter, den eine exponentielle Hüllkurve bei jeder Note aufreißt. Zwei Dinge bringen seine Linien zum Sprechen: Eine Note mit Velocity 110 oder mehr ist ein Accent, der den Filter weiter öffnet und lauter zuschlägt, und zwei überlappende Noten gleiten in der Tonhöhe, ohne die Hüllkurve neu zu starten. Der Ladder-Filter und die Ausgangsstufen, die seinen Bassbereich formen, sind unsere eigene Version derer in [Open303](https://github.com/RobinSchmidt/Open303) von Robin Schmidt. Verbinde ein Riff, ein Steps oder eine PianoRoll mit seinem MIDI-Eingang; wenn sein Editor den Fokus hat, spielt ihn auch die Computertastatur direkt.

## Parameter

**Wave** Form des Oszillators: SAW, SQR, PLS oder TRI.

**Cutoff** Die Grundfrequenz des Filters. Von hier aus fährt die Hüllkurve nach oben.

**Resonance** Betonung des Filters, von sanftem Abfall bis zum vollen Squelch.

**EnvMod** Wie weit die Hüllkurve die Filterfrequenz über Cutoff hinaus öffnet.

**Decay** Wie schnell der Filter-Sweep zurückfällt, in Millisekunden.

**Accent** Wie viel härter eine Note mit Accent zuschlägt. Null macht jede Note schlicht.

**Glide** Wie lange ein Slide zwischen überlappenden Noten braucht, um anzukommen, in Millisekunden. 60 ist die klassische Einstellung.

**Attack** Wie schnell die Lautstärke am Anfang einer Note steigt, in Millisekunden.

**AmpDecay** Wie schnell eine gehaltene Note in Richtung Sustain abfällt. Ganz oben im Bereich fällt die Note überhaupt nicht ab.

**AccentDecay** Das Filter-Decay bei Noten mit Accent, in Millisekunden, damit Accents länger klingen oder kürzer zuschnappen können als normale Noten.

**FilterFM** Führt den Ausgang zurück auf den Cutoff, sodass der Filter sich selbst moduliert. Null ist sauber, höher ist rauer.

**Track** Wie weit der Cutoff der gespielten Tonhöhe folgt. Null hält den Filter fest, egal welche Note.

**Sustain** Wo eine gehaltene Note nach AmpDecay stehen bleibt. Null ist wie die Box, bei der jede Note verklingt; weiter oben hält die Note, bis sie losgelassen wird.

**Squelch** Wie viel Bass die Rückkopplung des Filters wegnehmen darf. Niedrig hält den Bass auch bei starker Resonanz; hoch dünnt ihn zum klassischen Squelch aus.

**Velocity** Wie stark die Velocity den Accent formt. Bei null ist ein Accent über Velocity 110 alles oder nichts; bei eins bestimmt jede Velocity ihre eigene Accent-Stärke.

**Punch** Die Filterhüllkurve hebt auch die Lautstärke an, bei Accents stärker. Null schaltet das ab.

**Muffler** Ein Tiefpass am Ausgang, um die Höhen zu zähmen, wenn Drive aufgedreht ist.

**Drive** Sättigung auf dem Weg nach draußen.

**LfoWave** Die Form eines zusätzlichen Wobbles auf dem Cutoff: SIN, TRI, SQR, SAW, DSAW oder S&H.

**LfoRate** Das Tempo des Wobbles in Hz, solange LfoSync aus ist.

**LfoDepth** Wie weit der Wobble den Cutoff bewegt. Null schaltet ihn ab.

**LfoSync** Koppelt den Wobble an den Transport statt an LfoRate.

**LfoBeats** Die Länge eines Wobble-Zyklus in Schlägen, solange LfoSync an ist.

**Level** Ausgangslautstärke.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

## Rezept

**Squelch-Linie** Wave SAW, Cutoff 400, Resonance 8, EnvMod 0.7, Decay 250, Accent 0.6, Glide 60, Drive 0.4. Schick ein Riff mit ein paar Accent-Schritten und einem Slide hinein, dann fahr Cutoff über den Takt von Hand.

## Verwandte Organisms

Riff, Steps, Microdot
