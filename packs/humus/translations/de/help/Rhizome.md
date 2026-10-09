# Rhizome

Ein sechsstimmiger MIDI-Synth, der über jeder Note zusätzliche Stimmen stapelt, im Abstand von Tonleiterstufen der Stimmung des Patches.

Jede gespielte Note treibt eine Reihe Ausläufer: weitere Oszillatoren, die Interval Tonleiterstufen über dem Grundton stehen, wobei ihre Tonhöhen aus der Stimmung des Patches gelesen werden statt aus der Obertonreihe. Ändere die Stimmung am Transport, und die Klangfarbe ändert sich mit, nicht nur die Tonhöhe. Die Ausläufer wechseln sich für mehr Breite links und rechts ab, und ein Tiefpass über der Summe hält einen hohen Stapel Sägezähne im Zaum. Spiel es von einer PianoRoll, einem DNA oder der Tastatur aus, wenn der Editor den Fokus hat, und schick es durch Fern oder SoundSpace.

## Parameter

**Runners** Wie viele Stimmen aus jeder Note wachsen, vom Grundton allein bis zu acht.

**Interval** Der Abstand zwischen aufeinanderfolgenden Ausläufern, in Tonleiterstufen. Auf dem Panel als Degrees angezeigt.

**Creep** Langsames, unabhängiges Driften der Tonhöhe pro Ausläufer, in Cent. Ein wenig verhindert, dass der Stapel phasenstarr wird.

**Tilt** Die Pegelneigung entlang der Kette. Niedrig hält den Klang nah am Grundton; hoch lässt die fernen Ausläufer durchkommen.

**Wave** Die Wellenform der Ausläufer: SIN, SAW oder SQR.

**Bloom** Attack-Zeit in Millisekunden, also wie lange eine Note braucht, um sich zu öffnen.

**Decay** Release-Zeit in Millisekunden, also wie lange eine Note nach dem Note-off zum Ausklingen braucht.

**Cutoff** Ein Tiefpass über dem summierten Ausgang, in Hz. Senk ihn, wenn viele Sägezahn-Ausläufer harsch werden.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

## Rezept

**Cluster** Interval 1, Runners 6, Creep 0, Wave SIN. Spiel einzelne Noten aus einer PianoRoll: Jede wird zu einem Cluster benachbarter Tonleiterstufen, der in der eigenen Schrittweite der Tonleiter schwebt. Stimm das Patch am Transport um, und die Schwebung ändert sich mit.

## Verwandte Organisms

Wave, Mineral, Substrate
