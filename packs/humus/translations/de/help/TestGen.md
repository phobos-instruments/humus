# TestGen

Ein vorhersehbares Testsignal: ein Sinus auf einer Frequenz oder weißes, rosa oder braunes Rauschen.

Nimm es, um einen Signalweg zu prüfen, um Pegel einzustellen, bevor es Musik gibt, oder um die Form eines Filters für sich zu hören: Ein Sinus zeigt, was ein Filter mit einer Frequenz macht, und Rauschen enthält alle Frequenzen zugleich und zeigt, was er mit allen macht. Es hat keinen Eingang und schickt dasselbe Signal an beide Ausgänge, sodass ein Organism eine Stereokette speist. Ein reiner Sinus zeigt auf einer Pegelanzeige viel mehr an, als er klingt, also vertrau eher einem VuMeter als deinen Ohren.

## Parameter

**Amplitude** Ausgangspegel. Fang leise an; ein Sinus mit Vollaussteuerung in eine unbekannte Kette ist genau der Weg, auf dem Monitore kaputtgehen.

**Frequency** Tonhöhe des Sinus, 10 Hz bis 20 kHz. Wird ignoriert, solange eine Rausch-Wellenform gewählt ist.

**Waveform** Sine ist eine Frequenz und sonst nichts. White noise hat gleiche Energie pro Hertz, Pink noise gleiche Energie pro Oktave, und Brown noise fällt noch schneller ab, für ein tiefes Grollen.

## Rezept

**Filter-Check** Waveform White noise, Amplitude 0.3. Schick es in einen Filter, den Filter in ein Spectrum und weiter in ein SoundOut. Fahr die Frequency des Filters durch, und das Spectrum zeichnet die Kurve, die der Filter anwendet.

## Verwandte Organisms

Wave, Rhizome, VuMeter, Spectrum
