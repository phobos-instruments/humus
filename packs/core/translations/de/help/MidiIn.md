# MidiIn

Live-MIDI von einem Hardwaregerät oder einer anderen Anwendung, ausgegeben an einem MIDI-Ausgang.

Verbinde seinen Ausgang mit einem Instrument wie Rhizome, mit einem MidiBus oder mit einem MidiMonitor, um zu sehen, was ankommt. Das Patch speichert eine Portnummer statt eines Gerätenamens, deshalb übernimmt ein Patch auf einem anderen Rechner das Gerät, das dieser Rechner demselben Port zuweist; Geräte werden in den Einstellungen den acht Ports zugewiesen, und ein Portwechsel wirkt sofort. Die Bildschirmtastatur unten im Eigenschaften-Editor spielt Noten direkt in den Ausgang dieses Organisms, sodass sich die Instrumente dahinter ohne Hardware ausprobieren lassen. Sie lässt auch die Tasten aufleuchten, die du auf einem angeschlossenen Keyboard hältst, damit du siehst, was ankommt. Ältere Patches verwenden MidiIn1 bis MidiIn8; sie werden als dieses Organism geladen, mit entsprechend voreingestelltem Port.

## Parameter

**Port** Auf welche MIDI-Eingangsports dieses Organism hört. Alle Eingänge, die Voreinstellung für ein neues MidiIn, hört jedes angeschlossene Keyboard und jeden Controller gleichzeitig; ein einzelner Port hört nur das ihm zugewiesene Gerät, und die Auswahlliste zeigt jeden Port mit diesem Gerät.

## Rezept

**Keyboard an Synth** Zieh ein MidiIn herein, wähle den Port, dem dein Keyboard zugewiesen ist, verbinde es mit einem Rhizome und das Rhizome mit dem Mixer. Klick zuerst auf die Bildschirmtasten, um die Stimme zu hören, dann spiel auf der Hardware.

## Verwandte Organisms

MidiOut, MidiMonitor
