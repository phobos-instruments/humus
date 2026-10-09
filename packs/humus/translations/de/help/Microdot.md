# Microdot

Ein rollender Offbeat-Bass-Synth, angetrieben von einem Trigger-Raster mit sechzehn Schritten, das synchron zum Transport läuft.

Jeder Anschlag ist ein bandbegrenzter Oszillator mit optionalem Sub-Oktav-Rechteck, durch eine schnelle Filterhüllkurve und eine Drive-Stufe. Ein frisches Organism startet mit dem klassischen Roll, drei Sechzehntel nach jedem Downbeat, sodass es einrastet, sobald der Transport spielt, und im Stillstand schweigt es. Das Raster ist immer das Gate; Seq entscheidet, wem die Tonhöhe gehört. Mit Seq an spielt jeder Anschlag das Note-Feld. Mit Seq aus folgen die Anschläge dem, was der MIDI-Eingang gerade hält, sodass bei einem verkabelten Riff, Steps oder einer verkabelten PianoRoll die Linie vom Raster neu zerhackt wird. Kombinier es mit einem Kick in 4/4 und verbinde beide mit einem Compressor.

## Das Raster

Klick auf einen Schritt, um ihn umzuschalten; die Spalte, auf der der Transport gerade steht, leuchtet. Die ^-Zeile hebt den Anschlag dieses Schritts um eine Oktave, und die Halte-Zeile ganz unten zieht die vorige Note durch diesen Schritt, sodass eine Reihe davon eine einzige Note hält. Ein Rechtsklick bietet die Stempel: Roll -111, Offbeat --1-, Voll 1111, Zufall und Löschen. Das Pattern wird im Patch gespeichert.

## Parameter

**Note** Die Tonhöhe, die jeder Anschlag spielt, solange Seq an ist.

**Wave** SAW, SQR, PLS (ein dünner Puls), TRI, SIN oder FAT, ein verstimmtes Sägezahnpaar für Breite.

**Seq** An: Das Note-Feld bestimmt die Tonhöhe; aus: Die gehaltene MIDI-Note bestimmt sie. Das Raster gatet in beiden Fällen.

**Cutoff** Die Ruhefrequenz des Filters.

**Reso** Resonanz. Dreh sie auf, und der Hüllkurven-Sweep beginnt zu singen.

**EnvMod** Wie weit die Hüllkurve den Filter bei jedem Anschlag über Cutoff öffnet.

**Decay** Länge der Filterhüllkurve in Millisekunden; sie formt auch den Pegel der Note. 80 bis 150 hält den straffen, rollenden Charakter.

**Gate** Anschlaglänge als Bruchteil des Schritts. Kürzer ist hüpfender.

**Sub** Sub-Oktav-Rechteck darunter, für Gewicht.

**Drive** Sättigung nach dem Filter.

**Level** Ausgangspegel.

**SwingFollow** Follow: übernimmt den Groove des Patches vom Transport statt vom Swing-Schieberegler.

**Swing** Verzögert jedes zweite Sechzehntel, bis hin zu einem Triolen-Feel. Wirkt nur, solange Follow aus ist.

## Rezept

**Rollender Bass** Note A1, Wave SAW, Cutoff 700, Reso 0.35, EnvMod 0.5, Decay 120, Gate 0.8, Sub 0.4, Drive 0.35, Follow an. Füg einen Kick mit 4/4 an hinzu und starte die Wiedergabe; der Kick landet auf dem Schlag, und der Bass rollt dazwischen. Stell Seq auf aus und verbinde ein Riff mit dem Eingang, um dem Roll eine bewegte Linie zu geben.

## Verwandte Organisms

Kick, Acid, Riff, DNA
