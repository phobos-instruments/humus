# Substrate

Ein sich entwickelnder Drone oder ein Pad aus bis zu 64 verstimmten Sägezahn-Schichten, gestapelt auf einer Intervallleiter.

Jede Schicht steigt um eine Sprosse von Interval über den Grundton und hat ihre eigene langsame Drift in Tonhöhe und Pegel; die Schichten wechseln sich im Stereofeld ab. Interval wird bei der Stimmung des Patches erfragt, sodass die Leiter unter einem Tuning-Organism die eigenen Stufen dieser Tonleiter hinaufgeht. Die Leiter beginnt nach acht Sprossen von vorn, sodass sich hohe Werte für Layers als driftende Unisono-Kopien auftürmen, und der Pegel bleibt gleich, wenn Schichten dazukommen. Presets bringen die Leitern Octaves, Fifths und Thirds mit. Spiel es von einer PianoRoll oder einem DNA aus, oder lass Drone an und fahr Cutoff von einem Metapad aus.

## Parameter

**Drone** Aus spielt per MIDI über Attack und Release. An klingt dauerhaft auf Note, immer als ein einziger Grundton, egal was Mode sagt.

**Note** Der Grundton, solange Drone an ist oder solange im Mono-Modus noch nichts gespielt wurde.

**Layers** Wie viele Schichten, 1 bis 64. Ab etwa 16 ist es kein Akkord aus Schichten mehr, sondern eine Textur.

**Interval** Wie viele Tonleiterstufen jede Sprosse hinaufsteigt. In gewöhnlicher Stimmung stapelt 12 Oktaven, 7 Quinten und 4 Terzen; alles dazwischen ist ein Klangteppich, den keine feste Leiter bietet.

**Spread** Drift-Tiefe in Cent, wie weit die Schichten auseinanderwandern.

**Motion** Wie stark die Drift Tonhöhe und Pegel bewegt. 0 friert den Klangteppich ein.

**Cutoff** Der Tiefpass des Klangteppichs. Automatisiere ihn für das Anschwellen des Pads.

**Attack** Wie lange der Klangteppich zum Öffnen braucht, in Millisekunden.

**Release** Wie lange er zum Ausklingen braucht, nachdem die Noten losgelassen werden.

**Level** Ausgangspegel.

**BendRange** Wie weit das Pitch-Bend-Rad bei vollem Ausschlag reicht, in Halbtönen. Zwei ist der übliche Standard; null ignoriert das Rad.

**Mode** Wie MIDI-Noten zu Stimmen werden. Mono ist ein Grundton, die letzte Note gewinnt. Para verteilt den Vorrat an Schichten auf alle gehaltenen Noten, sodass ein Dreiklang mit 12 Schichten jeder Note einen Stapel aus vier gibt, bei gleichen Kosten, egal wie viele Tasten gedrückt sind. Poly gibt jeder Note den vollen Stapel mit eigener Hüllkurve, acht Stimmen.

## Rezept

**Neu verteilendes Pad** Drone aus, Mode Para, Layers 24, Interval 7, Motion 0.6, Attack 1500, Release 3000, Cutoff 1200. Halte langsame Akkorde aus einer PianoRoll; die Schichten verteilen sich auf jede gehaltene Note und fließen zu dem zurück, was noch gehalten wird, wenn eine Note endet.

## Verwandte Organisms

Tuning, Rhizome, DNA
