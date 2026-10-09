# Filter

Ein röhrengetriebener State-Variable-Filter in der Tradition des Club-Filters im Rack-Format.

Eine Drive-Stufe speist einen resonanten Filter, dessen Tasten HI, BAND und LO sich kombinieren, statt sich auszuschließen: HI mit LO ist ein Notch, den du für Phasing durchfahren kannst, alle drei zusammen sind ein Allpass, den Resonance dramatisch macht, und ohne Auswahl geht das Signal sauber durch. Ein Hüllkurvenfolger und ein Dreieck- oder Rechteck-Sweep können beide den Cutoff schieben, oder die Hüllkurve kann stattdessen in den Drive geschickt werden. Jeder Schalter schaltet weich, sodass du die Tasten beim Spielen ohne Klicks bedienen kannst. Setz ihn per Kabel hinter ein Rhizome, ein Drums-Organism oder einen ganzen Mixer, und verkable einen LFO oder einen Follower mit Frequency für mehr Formen, als die eingebauten Sweeps hergeben.

## Parameter

**EnvFollow** Wie weit die Lautheit des Eingangs den Cutoff öffnet. Null lässt den Filter statisch.

**EnvDecay** Fast oder Slow als Release der Hüllkurve, sobald der Eingang abfällt.

**Drive** Die Röhrenstufe. Sie fügt erst Wärme hinzu, dann weiches Clipping, dann hartes Clipping, je weiter du sie aufdrehst, mit Ausgleichsgain, damit der Pegel gleich bleibt.

**EnvToDrive** Schickt die Hüllkurve statt in den Cutoff in Drive, sodass laute Passagen stärker verzerren.

**LfoSpeed** Sweep-Rate von 0.2 Hz bis 10 kHz. Über ein paar hundert Hertz wird der Sweep selbst zu einem Ton.

**LfoWave** Triangle gleitet zwischen den Enden des Sweeps. Square springt zwischen ihnen.

**LfoSync** Koppelt den Sweep an den Transport statt an LfoSpeed.

**LfoBeats** Die Länge eines synchronisierten Sweeps in Schlägen, also ist 4 ein Takt mit vier Schlägen.

**LfoDepth** Wie weit der Sweep den Cutoff schiebt, bis zu ein paar Oktaven in jede Richtung.

**Resonance** Von sanftem Abfall bis zur Synth-Spitze. Sie nimmt sich bei hohen Eingangspegeln und tiefem Cutoff von selbst zurück, damit volle Sweeps lautsprechersicher bleiben.

**HiPass** Fügt das Hochpassband zum Ausgang hinzu.

**BandPass** Fügt das Bandpassband zum Ausgang hinzu.

**LoPass** Fügt das Tiefpassband zum Ausgang hinzu.

**Frequency** Der Cutoff, 20 Hz bis 20 kHz. Der große Sweep.

**FilterOn** Schaltet den Filter ein oder aus. Aus lässt das Signal unverändert durch.

**Mono** Schaltet die Filter beider Kanäle vom linken Eingang aus in Reihe, für 24 dB Flankensteilheit und doppelte Resonanz, und speist beide Ausgänge.

**MixInvert** Gibt das Direktsignal minus das gefilterte Signal aus, für Auslöschungseffekte, die gut vor einem Delay oder Hall sitzen.

## Rezept

**Synchroner Sweep** LoPass an, Resonance 6, Drive 3, Frequency 400. LfoSync an, LfoBeats 4, LfoWave Triangle, LfoDepth 6. Schick ein Drums-Organism per Kabel hinein, und der Cutoff öffnet und schließt sich einmal pro Takt im Tempo des Transports.

## Verwandte Organisms

LFO, Gain, Follower
