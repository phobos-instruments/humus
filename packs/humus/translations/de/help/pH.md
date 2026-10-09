# pH

Ein FM-Synthesizer mit vier Engines: einer Sechs-Operator-Engine, die DX7-.syx-Bänke liest, und Emulationen der Chips OPN2, OPM und OPL3.

Die Bank bestimmt die Engine. Eine .syx enthält Sechs-Operator-Klänge; eine .wopn, .tfi oder .dmp enthält OPN2-Instrumente für den Kern [Nuked-OPN2](https://github.com/nukeykt/Nuked-OPN2); eine .opm enthält OPM-Klänge für [Nuked-OPM](https://github.com/nukeykt/Nuked-OPM); eine .wopl enthält OPL3-Instrumente für [Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3). Lade eine, und die passende Engine spielt sie: acht Stimmen auf der Sechs-Operator-Engine, sechs Kanäle auf dem OPN2, acht auf dem OPM und achtzehn auf dem OPL3. Die Tonhöhe kommt aus der Stimmung des Patches, sodass eine mikrotonale Tonleiter auch auf Chip-Stimmen spielt. Ein Patch nennt seine Bank nur beim Namen, statt sie mitzuführen, also gib die Bank zusammen mit dem Patch weiter. Schließ eine PianoRoll, ein DNA oder ein MidiIn an.

## Parameter

**Patch** Welcher Klang der Bank spielt, nach Name. Wählst du eine Bank, geht es wieder bei ihrem ersten Klang los.

**Bright** Wie stark die Modulatoren ihre Träger antreiben. Die Mitte ist der Klang wie programmiert; höher fügt Obertöne hinzu, niedriger schließt den Ton in Richtung Sinus.

**Attack** Wie lange jeder Operator zum Öffnen braucht, ausgehend von dem, was der Klang verlangt.

**Release** Wie lange der Klang zum Ausklingen braucht, nachdem die Taste losgelassen wird.

**Detune** Spreizung zwischen den Operatoren, in Cent. Kleine Werte machen einen Klang dicker; große schweben gegen sich selbst.

**Vibrato** Tiefe des eigenen langsamen Tonhöhenschwankens des Klangs.

**Speed** Wie schnell dieses Schwanken läuft.

**Transpose** Verschiebt die Note in Halbtönen. Es verändert die klingende Frequenz statt der Notennummer, sodass es in jeder Stimmung stimmt.

**Fine** Verschiebt die Note in Cent, auf dieselbe Weise.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

**Algorithm** Welcher Operator welchen antreibt und welche zum Ausgang gelangen. Die Sechs-Operator-Engine bietet 32; die Chips bieten 8.

**Feedback** Wie stark der erste Operator auf sich selbst zurückwirkt, 0 bis 7.

**Op1_Level** Ausgangspegel von Operator 1, ebenso für die Operatoren 2 bis 6. Wählst du unter Patch einen Klang, landen seine Werte in diesen Zeilen, sodass das, was auf dem Bildschirm steht, auch das ist, was spielt.

**Op1_Ratio** Frequenzverhältnis von Operator 1 zur Note, ebenso für 2 bis 6.

**Op1_Detune** Feinverstimmung von Operator 1, mit der Mitte bei 7, ebenso für 2 bis 6.

**Op1_Attack** Attack-Rate der Hüllkurve von Operator 1, ebenso für 2 bis 6.

**Op1_Decay** Decay-Rate der Hüllkurve von Operator 1, ebenso für 2 bis 6.

**Op1_Sustain** Sustain-Pegel der Hüllkurve von Operator 1, ebenso für 2 bis 6.

**Op1_Release** Release-Rate der Hüllkurve von Operator 1, ebenso für 2 bis 6.

**Op1_On** Nimmt Operator 1 aus dem Klang heraus oder setzt ihn wieder ein und behält dabei seinen Pegel, ebenso für 2 bis 6. Die Chips haben vier Operatoren, deshalb sind Op5_On und Op6_On nicht verfügbar, solange eine Chip-Bank geladen ist.

**File** Die Bank, aus der gespielt wird, und mit ihr die Engine. Leer ist die Werksbank; eine Bank, die du in den Ordner Banks legst, erscheint in der Liste und verdeckt eine mitgelieferte Bank gleichen Namens.

## Rezept

**Chip-E-Piano** Lade die Chip-Werksbank und wähl unter Patch einen E-Piano-Klang. Bright 0.6, Detune 6, Release 0.6, Vibrato 0.1, Speed 0.4. Schließ eine PianoRoll an und setz ein Fern dahinter, mit Feedback um 0.4 für das Echo; schalte Op4_On beim Spielen aus und wieder ein, um zu hören, was der oberste Operator beiträgt.

## Verwandte Organisms

Rhizome, Wave, PianoRoll, DNA
