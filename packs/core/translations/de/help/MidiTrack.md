# MidiTrack

Eine Timeline-Spur, die einen MIDI-Part enthält und ihn in ein beliebiges Instrument im Patch spielt.

Sie hat keinen eigenen Klang und keine Box im Patcher: Der Part liegt auf der Spur, und der Chip der Zeile nennt das Organism, in das sie spielt. Wähl ein anderes Ziel, und dieselben Noten wandern unverändert zu einem anderen Instrument. So kannst du einen Part über mehrere Synths anhören oder ihn sicher aufheben, während das Patch um ihn herum neu gebaut wird. Neue Spuren beginnen mit (nichts) und bleiben still, bis ein Ziel gewählt ist. Mit der Schaltfläche zum Scharfschalten in der Zeile läuft Live-MIDI vom Eingang durch die Spur ins Ziel, und was du spielst, landet während der Aufnahme in einem Clip. Stumm, Solo, Umbenennen und Löschen funktionieren in der Zeile wie bei jeder anderen Spur.

## Parameter

**Target** Das Instrument, in das diese Spur spielt, angezeigt auf dem Chip der Zeile. Die Liste bietet jedes Organism im Patch mit einem MIDI-Eingang an; wählst du eines, wird der MIDI-Ausgang der Spur per Kabel damit verbunden, und ein von Hand gezogenes Kabel von diesem Ausgang setzt Target genauso.

## Rezept

**Ein Part, zwei Synths** Stell Target auf ein Rhizome, schalte die Zeile scharf und nimm eine Phrase auf, oder zeichne eine in den Clip. Füg ein zweites Instrument hinzu, stell Target darauf um, und derselbe Clip spielt den neuen Synth; schalte zurück, sobald du die beiden verglichen hast.

## Verwandte Organisms

MidiBus, PianoRoll, Steps
